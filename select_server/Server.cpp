#include "Server.h"
#include "Packet.h"    // 다형성 패킷 처리
#include <iostream>
#include <algorithm>
#include <cstring>     // std::memcpy, std::memmove

Server::Server() = default;

Server::~Server()
{
    for (auto& s : sessions)
    {
        if (s->socket != INVALID_SOCKET)
            closesocket(s->socket);
    }
    if (listenSocket != INVALID_SOCKET)
        closesocket(listenSocket);

    WSACleanup();
}

bool Server::Initialize()
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        return false;

    listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket == INVALID_SOCKET)
        return false;

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(listenSocket, reinterpret_cast<SOCKADDR*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR)
    {
        std::cerr << "Bind Error: " << WSAGetLastError() << std::endl;
        return false;
    }

    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR)
    {
        std::cerr << "Listen Error: " << WSAGetLastError() << std::endl;
        return false;
    }

    u_long mode = 1;
    ioctlsocket(listenSocket, FIONBIO, &mode);

    return true;
}

void Server::Tick()
{
    NetworkProc();
    CleanupDeadSessions();
    Monitor();
}

void Server::NetworkProc()
{
    fd_set readSet;
    FD_ZERO(&readSet);

    FD_SET(listenSocket, &readSet);

    for (auto& s : sessions)
    {
        if (!s->isDead)
            FD_SET(s->socket, &readSet);
    }

    timeval timeout = { 0, 0 };
    int result = select(0, &readSet, nullptr, nullptr, &timeout);
    if (result == SOCKET_ERROR)
        return;

    if (FD_ISSET(listenSocket, &readSet))
    {
        AcceptProc();
    }

    for (auto& sp : sessions)
    {
        Session& s = *sp;
        if (!s.isDead && FD_ISSET(s.socket, &readSet))
        {
            RecvProc(s);
        }
    }
}

void Server::AcceptProc()
{
    sockaddr_in clientAddr;
    int addrLen = sizeof(clientAddr);
    SOCKET clientSocket = accept(listenSocket, reinterpret_cast<SOCKADDR*>(&clientAddr), &addrLen);

    if (clientSocket == INVALID_SOCKET)
        return;

    if (sessions.size() >= MAX_SESSIONS)
    {
        closesocket(clientSocket);
        return;
    }

    u_long mode = 1;
    ioctlsocket(clientSocket, FIONBIO, &mode);

    auto newSession = std::make_unique<Session>();
    newSession->socket = clientSocket;
    newSession->id = idCounter++;
    newSession->x = 0;
    newSession->y = 0;

    Session* newSessionPtr = newSession.get();

    std::cout << "[접속] ID: " << newSessionPtr->id << " (" << sessions.size() + 1 << "명)" << std::endl;

    // 1. ID 할당 패킷 (Unicast)
    {
        PacketIDAssign pkt;
        pkt.id = newSessionPtr->id;
        RawPacket16 raw = pkt.ToRaw();
        SendTo(*newSessionPtr, raw);
    }

    // 2. 내 별 생성 패킷 (나 + 기존 유저들)
    {
        PacketStarCreate createMy;
        createMy.id = newSessionPtr->id;
        createMy.x = newSessionPtr->x;
        createMy.y = newSessionPtr->y;
        RawPacket16 raw = createMy.ToRaw();

        // 나에게
        SendTo(*newSessionPtr, raw);
        // 기존 유저들에게
        Broadcast(raw, newSessionPtr);
    }

    // 3. 기존 유저들의 별 정보를 신규 유저에게
    for (auto& sp : sessions)
    {
        Session& s = *sp;
        if (!s.isDead)
        {
            PacketStarCreate createOther;
            createOther.id = s.id;
            createOther.x = s.x;
            createOther.y = s.y;
            RawPacket16 raw = createOther.ToRaw();
            SendTo(*newSessionPtr, raw);
        }
    }

    sessions.push_back(std::move(newSession));
}

void Server::RecvProc(Session& s)
{
    int bytesRecv = recv(s.socket, s.recvBuffer + s.recvBytes,
        sizeof(s.recvBuffer) - s.recvBytes, 0);

    if (bytesRecv == 0 || bytesRecv == SOCKET_ERROR)
    {
        if (bytesRecv == SOCKET_ERROR && WSAGetLastError() == WSAEWOULDBLOCK)
            return;

        DisconnectSession(s);
        return;
    }

    s.recvBytes += bytesRecv;

    while (s.recvBytes >= PACKET_SIZE)
    {
        ProcessPacket(s, s.recvBuffer);

        std::memmove(s.recvBuffer,
            s.recvBuffer + PACKET_SIZE,
            s.recvBytes - PACKET_SIZE);
        s.recvBytes -= PACKET_SIZE;
    }
}

void Server::ProcessPacket(Session& session, const char* packetData)
{
    RawPacket16 raw{};
    static_assert(sizeof(RawPacket16) == PACKET_SIZE, "PACKET_SIZE must be 16");
    std::memcpy(&raw, packetData, PACKET_SIZE);

    auto packet = PacketFactory::CreateFromRaw(raw);
    if (!packet)
        return;

    packet->FromRaw(raw);
    packet->Handle(*this, session);
}

void Server::DisconnectSession(Session& s)
{
    if (s.isDead) return;

    s.isDead = true;
    std::cout << "[종료] ID: " << s.id << " 예약됨" << std::endl;

    PacketStarDelete del;
    del.id = s.id;
    RawPacket16 raw = del.ToRaw();
    Broadcast(raw, &s);
}

void Server::CleanupDeadSessions()
{
    auto it = std::remove_if(sessions.begin(), sessions.end(),
        [](const std::unique_ptr<Session>& sp)
        {
            return sp->isDead;
        });

    for (auto itr = it; itr != sessions.end(); ++itr)
    {
        if ((*itr)->socket != INVALID_SOCKET)
            closesocket((*itr)->socket);
    }

    sessions.erase(it, sessions.end());
}

void Server::Monitor()
{
    static DWORD lastCheckTime = 0;
    DWORD curTime = GetTickCount();

    if (curTime - lastCheckTime > 1000)
    {
        lastCheckTime = curTime;

        char title[256];
        sprintf_s(title, "Star Server - Users: %d | Last ID: %d",
            static_cast<int>(sessions.size()), idCounter);
        SetConsoleTitleA(title);
    }
}

void Server::SendTo(Session& session, const RawPacket16& raw)
{
    if (session.isDead) return;
    send(session.socket, reinterpret_cast<const char*>(&raw), PACKET_SIZE, 0);
}

void Server::Broadcast(const RawPacket16& raw, Session* exclude)
{
    for (auto& sp : sessions)
    {
        Session& s = *sp;
        if (&s != exclude && !s.isDead)
        {
            SendTo(s, raw);
        }
    }
}
