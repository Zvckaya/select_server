#include "Server.h"
#include "INetworkHandler.h"
#include "Packet.h" 
#include "Logger.h"
#include <iostream>
#include <algorithm>
#include <cstring>  

TcpServer::TcpServer() = default;

TcpServer::~TcpServer()
{
	for (auto& s : sessions)
	{
		if (s->socket != INVALID_SOCKET)
		{
			setsockopt(s->socket, SOL_SOCKET, SO_LINGER, (char*)&_linger, sizeof(_linger)); //4way 하지 않게 바로 정리
			closesocket(s->socket);
		}
	}

	if (listenSocket != INVALID_SOCKET)
		closesocket(listenSocket);

	WSACleanup();
}

bool TcpServer::Initialize(INetworkHandler* hander)
{
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		return false;

	listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listenSocket == INVALID_SOCKET)
		return false;

	_hander = hander;

	_linger.l_onoff = 1; //링거 구조체 초기화
	_linger.l_linger = 0;


	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(PORT);
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY); //모든 NIC 카드 i/o 감지

	if (bind(listenSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) //바인딩
	{
		std::cerr << "Bind Error: " << WSAGetLastError() << std::endl;
		return false;
	}

	if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) //리스닝 
	{
		std::cerr << "Listen Error: " << WSAGetLastError() << std::endl;
		return false;
	}

	u_long mode = 1;
	ioctlsocket(listenSocket, FIONBIO, &mode); //소켓을 non-blocking으로 전환

	return true; //여기까지 도달해야 listen 소켓 바인딩+리스닝 시작
}

void TcpServer::Tick()
{
	NetworkProc(); //네트워크 처리.
	CleanupDeadSessions(); //지연 삭제구현.
}

void TcpServer::NetworkProc() //네트워크 i/o 처리
{
	fd_set readSet; //readSet 구성
	fd_set writeSet; //writeSet 

	FD_ZERO(&readSet);
	FD_ZERO(&writeSet);
	//매 while마다 fd_set을 초기화해서 사용해야한다!
	//fd_set은 select 호출후 변형되는 구조체 이기때문에.
	//기존 기록이 남아 있음. -> recv 하려는데 send한게 없음-> 사고발생

	FD_SET(listenSocket, &readSet); //리슨 소켓부터 추가 

	for (auto& s : sessions) //세션들을 돔
	{

		if (!s->isDead) // 살아있으면 
		{
			FD_SET(s->socket, &readSet); //readSet에 추가 읽기 감지 

			if (s->sendBuffer.GetUseSize() > 0)
			{
				FD_SET(s->socket, &writeSet);
			}
		}
	}

	timeval timeout = { 0, 0 };
	int result = select(0, &readSet, nullptr, nullptr, &timeout); //대기 없이 select한다.
	if (result == SOCKET_ERROR)
		return;

	if (FD_ISSET(listenSocket, &readSet)) //listen 소켓이 readSet에 있는지 확인, 즉 새 connect가 있는가?
	{
		AcceptProc(); //accept절차 진행
	}

	for (auto& sp : sessions) //그후 모든 세션을 순회하며 
	{
		Session& s = *sp;
		if (!s.isDead && FD_ISSET(s.socket, &readSet)) //안죽었고, readSet에 해당 소켓이 포함되어 있는가? 
		{
			RecvProc(s);//포함되어 있으면 recv를 진행한다.
		}
	}

	for (auto& sp : sessions)
	{
		Session& s = *sp; //sp 는 이터레이터 
		if (!s.isDead && FD_ISSET(s.socket, &writeSet))
		{
			SendProc(s);
		}

	}
}

void TcpServer::AcceptProc() //accept 처리
{
	sockaddr_in clientAddr;
	int addrLen = sizeof(clientAddr);
	SOCKET clientSocket = accept(listenSocket, (SOCKADDR*)&clientAddr, &addrLen); //accept를 한후, 해당 소켓 정보를 받아옴

	if (clientSocket == INVALID_SOCKET)
		return;

	if (sessions.size() >= MAX_SESSIONS)  //MAX_SESSIONS세션의 사이즈는 60임
	{
		setsockopt(clientSocket, SOL_SOCKET, SO_LINGER, (char*)&_linger, sizeof(_linger)); //4way 하지 않게 바로 정리
		closesocket(clientSocket); //해당 소켓을 닫음. NP 정리
		return;
	}

	u_long mode = 1;
	ioctlsocket(clientSocket, FIONBIO, &mode); //비동기 소켓으로 설정

	auto newSession = std::make_unique<Session>(); //유니크 포인터로 생성
	newSession->socket = clientSocket;
	newSession->id = idCounter++;


	Session* newSessionPtr = newSession.get(); //세선 정보를 가져옴 

	Logger::Instance().Log("[접속] ID: " + std::to_string(newSessionPtr->id) + " (" + std::to_string(sessions.size() + 1) + "명)");


	if (_hander)
	{
		Logger::Instance().Log("핸들러 호출");

		_hander->OnConnection(*newSessionPtr);
	}

	sessions.push_back(std::move(newSession)); //성공적으로 id할당 및 별 생성했으면 세션을 추가한다.
	//session은 unique_ptr<Session>의 vector임

}

void TcpServer::RecvProc(Session& s)
{
	//직접 쓸 수 있는 공간 확인
	int directSize = s.recvBuffer.DirectEnqueueSize();

	if (directSize <= 0)
	{
		return;//
	}

	int recvBytes = recv(s.socket, s.recvBuffer.GetRearBufferPtr(), directSize, 0);

	if (recvBytes == SOCKET_ERROR)
	{
		if (WSAGetLastError() != WSAEWOULDBLOCK) // wouldblock은 당장 줄 데이터가 없으니 다시 확인해라~
		{
			DisconnectSession(s);
		}
		return;
	}

	if (recvBytes == 0) { //0이면 ret임 
		DisconnectSession(s);
		return;
	}

	s.recvBuffer.MoveRear(recvBytes); //둘다 아니면 rear 이동(복사는 잘 되었다 가정)

	while (true)
	{
		if (s.recvBuffer.GetUseSize() < sizeof(PacketHeader)) //헤더 크기는 왔나?
			break; //다음턴에 

		PacketHeader header;
		s.recvBuffer.Peek((char*)&header, sizeof(PacketHeader));


		if (header.IsValid() == false)
		{
			DisconnectSession(s); //조작된 패킷
			break;
		}

		int totalSize = header.GetTotalSize();

		if (s.recvBuffer.GetUseSize() < totalSize) //패킷 전제가 도착했나?
		{
			break;
		}

		std::vector<char> packetData(totalSize);
		s.recvBuffer.Dequeue(packetData.data(), totalSize);

		if (_hander)
		{
			_hander->OnRecv(s, packetData.data(), totalSize);
		}
	}

}

void TcpServer::SendProc(Session& s)
{
	int directSize = s.sendBuffer.DirectDequeueSize();
	if (directSize <= 0)
		return;

	
	int byteSent = send(s.socket, s.sendBuffer.GetFrontBufferPtr(), directSize, 0); //실제 send 수행 

	if (byteSent == SOCKET_ERROR)
	{
		if (WSAGetLastError() != WSAEWOULDBLOCK)
		{
			DisconnectSession(s);
		}
		return;
	}

	if (byteSent > 0) {
		s.sendBuffer.MoveFront(byteSent);
	}
}



void TcpServer::DisconnectSession(Session& s)
{
	if (s.isDead) return; //isDead가 변경되어 있으면 return 

	s.isDead = true;
	Logger::Instance().Log("[종료] ID: " + std::to_string(s.id) + " 예약됨\n"); //실제 종료는 아님

	if (_hander)
	{
		_hander->OnDisconnection(s);
	}
}

void TcpServer::CleanupDeadSessions()
{
	auto it = std::remove_if(sessions.begin(), sessions.end(),
		[](const std::unique_ptr<Session>& sp)
		{
			return sp->isDead;
		});
	//removeif+람다를 이용하여 세션들을 순회, 만약, 삭제예약(isDead)이 활성화 된 요소들을 뒤로 모아준다.
	//remove_if는 실제 삭제는 아님. 반환형은 삭제되지 않는 구간의 마지막 다음 위치를 반환함!
	//즉 이터레이터를 받아. 전체 순회가 아니라. 삭제 시작 위치에서 끝까지 돌 수 있다.

	for (auto itr = it; itr != sessions.end(); ++itr)
	{
		if ((*itr)->socket != INVALID_SOCKET) //만약 INVALID면 뭔가 문제가 있는것.
			closesocket((*itr)->socket); //소켓 핸들을 정리해준다. 
	}

	sessions.erase(it, sessions.end()); //실제 session, 즉 서버 인메모리에서는 지금 삭제 
	//delay 삭제를 통해 재귀를 방지하고, 댕글링 포인터 진입 자체를 막아준다 
	//lock을 걸지 않은 이유는 main 서버 스레드는(select가 도는) 짜피 싱글이라 경합이 발생하지 않는다..
}



void TcpServer::SendTo(Session& session, const char* data, int len ) //Raw한 패킷으로 변환해서 전송
{
	if (session.isDead) return; //만약 죽었으면 보내지 않음

	//LogSend(session, raw); //로깅

	
	if (session.sendBuffer.GetFreeSize() >= len)
	{
		session.sendBuffer.Enqueue(data, len);
		
	}
	else //send buffer가 찼다는건 상대 recv버퍼가 문제가 있다는것 , 비정상 종료 
	{
		DisconnectSession(session);
	}

}

void TcpServer::Broadcast(const char* data,int len, Session* exclude)
{
	// LogBroadcast(raw, exclude); //로깅 

	for (auto& sp : sessions) //세션들을 순회하며 
	{
		Session& s = *sp; //unique_ptr의 이터레이터라 뺴는게 날듯..
		if (&s != exclude && !s.isDead) //안죽었고 예외대상이 아니면 
		{
			SendTo(s, data,len);
		}
	}
}
//
//void TcpServer::LogSend(const Session& to, const RawPacket16& raw)
//{
//	PacketType type = static_cast<PacketType>(raw.type);
//	std::string msg = "[SEND]  To ID=" + std::to_string(to.id) +
//		" Type=" + PacketTypeToString(type) +
//		" (type=" + std::to_string(raw.type) +
//		", a=" + std::to_string(raw.a) +
//		", b=" + std::to_string(raw.b) +
//		", c=" + std::to_string(raw.c) + ")";
//	Logger::Instance().Log(msg);
//}
//
//void TcpServer::LogBroadcast(const RawPacket16& raw, const Session* exclude)
//{
//	PacketType type = static_cast<PacketType>(raw.type);
//	std::string msg = "[BCAST] Type=" + std::string(PacketTypeToString(type)) +
//		" (type=" + std::to_string(raw.type) +
//		", a=" + std::to_string(raw.a) +
//		", b=" + std::to_string(raw.b) +
//		", c=" + std::to_string(raw.c) + ")";
//	if (exclude)
//		msg += " (exclude ID=" + std::to_string(exclude->id) + ")";
//	Logger::Instance().Log(msg);
//}
//
//void TcpServer::LogRecv(const Session& from, const RawPacket16& raw)
//{
//	PacketType type = static_cast<PacketType>(raw.type);
//	std::string msg = "[RECV]  From ID=" + std::to_string(from.id) +
//		" Type=" + PacketTypeToString(type) +
//		" (type=" + std::to_string(raw.type) +
//		", a=" + std::to_string(raw.a) +
//		", b=" + std::to_string(raw.b) +
//		", c=" + std::to_string(raw.c) + ")";
//	Logger::Instance().Log(msg);
//}
