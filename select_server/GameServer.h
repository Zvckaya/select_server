#pragma once
#include "INetworkHandler.h"
#include "Session.h"
#include "Server.h"
#include <map>


struct Player {
	int x, y;
};

class  GameServer : public INetworkHandler
{
public:

	void SetNetwork(TcpServer* server) { _network = server; }
	virtual void OnConnection(Session& session) override;
    virtual void OnDisconnection(Session& session) override;
    virtual void OnRecv(Session& session, const char* packetData, int len) override;

    void SendPacket(Session& session, const RawPacket16& raw);
    void BroadcastPacket(const RawPacket16& raw, Session* exclude = nullptr);
    // 플레이어 이동 처리 (맵 데이터 갱신용)
    void UpdatePlayerPosition(int id, int x, int y);
private:
	std::map<int, Player> _players;
	TcpServer* _network = nullptr;
};
