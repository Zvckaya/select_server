// Server.h
#pragma once

#include <vector>
#include <memory>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <iostream>

#include "WinNetIncludes.h"
#include "NetConfig.h"
#include "PacketTypes.h"
#include "Session.h"

class Packet;
struct RawPacket16;

class Server
{
public:
    Server();
    ~Server();

    bool Initialize();
    void Tick();

    void SendTo(Session& session, const RawPacket16& raw);
    void Broadcast(const RawPacket16& raw, Session* exclude = nullptr);

private:
    SOCKET listenSocket = INVALID_SOCKET;
    std::vector<std::unique_ptr<Session>> sessions;
    int idCounter = 1;

    void NetworkProc();
    void AcceptProc();
    void RecvProc(Session& session);
    void ProcessPacket(Session& session, const char* packetData);
    void DisconnectSession(Session& session);
    void CleanupDeadSessions();
    void Monitor();

    //·Î±×¿ë
    void LogSend(const Session& to, const RawPacket16& raw);
    void LogBroadcast(const RawPacket16& raw, const Session* exclude);
    void LogRecv(const Session& from, const RawPacket16& raw);
};
