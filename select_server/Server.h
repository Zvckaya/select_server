// Server.h
#pragma once

#include <vector>
#include <memory>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <thread>

#include "WinNetIncludes.h"
#include "NetConfig.h"
#include "INetworkHandler.h"
#include "PacketTypes.h"
#include "Session.h"

class Packet;

class TcpServer
{
public:
    TcpServer();
    ~TcpServer();

    bool Initialize(INetworkHandler* hander);
    void Tick();
    void SendTo(Session& session, const char*data, int len);
    void Broadcast(const char* data, int len, Session* exclude =nullptr);

private:
    INetworkHandler* _hander = nullptr;
    SOCKET listenSocket = INVALID_SOCKET;
    std::vector<std::unique_ptr<Session>> sessions;
    struct linger _linger;
    

    int idCounter = 1;

    void NetworkProc();
    void AcceptProc();
    void RecvProc(Session& session);
    void SendProc(Session& session);
    void DisconnectSession(Session& session);
    void CleanupDeadSessions();
    

    //로그용(Logger 클래스를 호출하여 단순 로그를 찍음);
    /*void LogSend(const Session& to, const RawPacket16& raw); 
    void LogBroadcast(const RawPacket16& raw, const Session* exclude);
    void LogRecv(const Session& from, const RawPacket16& raw);*/
};
