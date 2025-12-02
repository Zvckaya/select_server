#include "GameServer.h"
#include "Packet.h"
#include "Server.h"
#include "NetConfig.h"
#include "Logger.h"

#include <iostream>
#include <iomanip>
#include <sstream>

// 패킷 내용을 낱낱이 까보는 함수
void InspectCreateCharPacket(const Pkt_SC_CreateChar& pkt)
{
	std::stringstream ss;
	ss << "\n========== [패킷 검사기] SC_CREATE_MY_CHARACTER ==========\n";

	// 1. 값 확인 (논리적 데이터)
	ss << "[Header] Code: 0x" << std::hex << (int)pkt.header.byCode
		<< " | Size: " << std::dec << (int)pkt.header.bySize
		<< " | Type: " << (int)pkt.header.byType << "\n";

	ss << "[Body]   ID: " << pkt.id << "\n"
		<< "         Dir: " << (int)pkt.direction << "\n"
		<< "         X: " << pkt.x << "  Y: " << pkt.y << "\n"
		<< "         HP: " << (int)pkt.hp << "\n";

	// 2. 헥사 덤프 (실제 메모리 배열 - 가장 중요!)
	// 여기서 00이나 CC 같은 이상한 패딩 바이트가 끼어있는지 봐야 함
	ss << "[HexDump] (Total " << sizeof(pkt) << " bytes)\n";

	const unsigned char* ptr = (const unsigned char*)&pkt;
	for (int i = 0; i < sizeof(pkt); ++i)
	{
		// 1바이트씩 16진수로 출력
		ss << std::hex << std::setw(2) << std::setfill('0') << (int)ptr[i] << " ";
	}
	ss << "\n======================================================\n";

	// 콘솔에 출력 (혹은 Logger::Instance().Log(ss.str()) 사용)
	std::cout << ss.str() << std::endl;
}

GameServer::GameServer()
{
	//생성자...
}

void GameServer::OnConnection(Session& session)
{
	Player newPlayer;
	newPlayer.id = session.id;
	newPlayer.x = 100;
	newPlayer.y = 100;
	newPlayer.direction = dfPACKET_MOVE_DIR_LL;
	newPlayer.hp = 100;

	_players[session.id] = newPlayer;

	{
		Pkt_SC_CreateChar pkt;
		pkt.header.SetInfo(PacketType::SC_CREATE_MY_CHARACTER, sizeof(Pkt_SC_CreateChar) - sizeof(PacketHeader));
		pkt.id = newPlayer.id;
		pkt.x = newPlayer.x;
		pkt.y = newPlayer.y;
		pkt.direction = newPlayer.direction;
		pkt.hp = newPlayer.hp;

		InspectCreateCharPacket(pkt);
		SendPacket(session, &pkt, sizeof(pkt));
	}

	{
		Pkt_SC_CreateChar pkt;

		pkt.header.SetInfo(PacketType::SC_CREATE_OTHER_CHARACTER, sizeof(Pkt_SC_CreateChar) - sizeof(PacketHeader));

		pkt.id = newPlayer.id;
		pkt.x = newPlayer.x;
		pkt.y = newPlayer.y;
		pkt.direction = newPlayer.direction;
		pkt.hp = newPlayer.hp;


		InspectCreateCharPacket(pkt);
		// 나(session)를 제외하고 모두에게 전송
		BroadcastPacket(&pkt, sizeof(pkt), &session);
	}

	for (auto& pair : _players)
	{
		if (pair.first == session.id) continue; // 나는 이미 위에서 처리함

		Player& other = pair.second;

		Pkt_SC_CreateChar pkt;

		pkt.header.SetInfo(PacketType::SC_CREATE_OTHER_CHARACTER, sizeof(Pkt_SC_CreateChar) - sizeof(PacketHeader));

		pkt.id = other.id;
		pkt.x = other.x;
		pkt.y = other.y;
		pkt.direction = other.direction;
		pkt.hp = other.hp;


		InspectCreateCharPacket(pkt);
		SendPacket(session, &pkt, sizeof(pkt));
	}
}


void GameServer::OnDisconnection(Session& session)
{
	// 1. 목록에서 제거
	_players.erase(session.id);

	// 2. 다른 유저들에게 삭제 알림 (SC_DELETE_CHARACTER)
	Pkt_SC_DeleteChar pkt;
	pkt.header.byCode = PACKET_CODE;
	pkt.header.bySize = sizeof(Pkt_SC_DeleteChar);
	pkt.header.byType = (uint8_t)PacketType::SC_DELETE_CHARACTER;
	pkt.id = session.id;

	BroadcastPacket(&pkt, sizeof(pkt), &session);
}

void GameServer::OnRecv(Session& session, const char* packetData, int len)
{
	auto packet = PacketFactory::CreatePacket((PacketHeader*)packetData);

	if (packet)
	{
		packet->Handle(*this, session);
	}
	else
	{
		return; 
	}
}

void GameServer::SendPacket(Session& session, void* ptr, int size)
{
	if (_network)
		_network->SendTo(session, (char*)ptr, size);
}

void GameServer::BroadcastPacket(void* ptr, int size, Session* exclude)
{
	if (_network)
		_network->Broadcast((char*)ptr, size, exclude);
}


void GameServer::UpdatePlayers()
{
	for (auto& pair : _players)
	{
		Player& p = pair.second;
		if (p.isMoving)
		{
			MovePlayer(p);
		}
	}
}

void GameServer::MovePlayer(Player& p)
{
	// 1. 방향에 따른 좌표 이동
	switch (p.direction)
	{
	case dfPACKET_MOVE_DIR_LL: // 좌
		p.x -= MOVE_UNIT_X;
		break;
	case dfPACKET_MOVE_DIR_LU: // 좌상
		p.x -= MOVE_UNIT_X;
		p.y -= MOVE_UNIT_Y;
		break;
	case dfPACKET_MOVE_DIR_UU: // 상
		p.y -= MOVE_UNIT_Y;
		break;
	case dfPACKET_MOVE_DIR_RU: // 우상
		p.x += MOVE_UNIT_X;
		p.y -= MOVE_UNIT_Y;
		break;
	case dfPACKET_MOVE_DIR_RR: // 우
		p.x += MOVE_UNIT_X;
		break;
	case dfPACKET_MOVE_DIR_RD: // 우하
		p.x += MOVE_UNIT_X;
		p.y += MOVE_UNIT_Y;
		break;
	case dfPACKET_MOVE_DIR_DD: // 하
		p.y += MOVE_UNIT_Y;
		break;
	case dfPACKET_MOVE_DIR_LD: // 좌하
		p.x -= MOVE_UNIT_X;
		p.y += MOVE_UNIT_Y;
		break;
	}

	//범위체크
	// Left
	if (p.x < RANGE_MOVE_LEFT) p.x = RANGE_MOVE_LEFT;

	// Right
	if (p.x > RANGE_MOVE_RIGHT) p.x = RANGE_MOVE_RIGHT;

	// Top (화면 좌표계에서 Y가 작을수록 위쪽)
	if (p.y < RANGE_MOVE_TOP) p.y = RANGE_MOVE_TOP;

	// Bottom
	if (p.y > RANGE_MOVE_BOTTOM) p.y = RANGE_MOVE_BOTTOM;

	Logger::Instance().Log("id:" + std::to_string(p.id) + " x: " + std::to_string(p.x) + " p.y" + std::to_string(p.y));
}

Player* GameServer::GetPlayer(int id)
{
	auto it = _players.find(id);
	if (it != _players.end())
	{
		return &it->second;
	}
	return nullptr;

}
void GameServer::Update()
{
	UpdatePlayers();
}