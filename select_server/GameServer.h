#pragma once
#include "INetworkHandler.h"
#include "PacketTypes.h" // 패킷 구조체 정의
#include <map>

// 전방 선언
class TcpServer;
struct Session;

//플레이어 
struct Player {
    int32_t id;
    int16_t x;
    int16_t y;
    uint8_t direction; 
    uint8_t hp;

    bool isMoving = false;
};

class GameServer : public INetworkHandler
{
public:
    GameServer();
    virtual ~GameServer() = default;

    //네트워크 라이브러리 연결 
    void SetNetwork(TcpServer* server) { _network = server; }

    // 네트워크 인터페이스 
    virtual void OnConnection(Session& session) override;
    virtual void OnDisconnection(Session& session) override;
    virtual void OnRecv(Session& session, const char* packetData, int len) override;

    //송신관련 
    void SendPacket(Session& session, void* ptr, int size);
    void BroadcastPacket(void* ptr, int size, Session* exclude = nullptr);

    // 플레이어 찾기
    Player* GetPlayer(int id);

    //프레임 로직
    void Update();
    void UpdatePlayers();
    void MovePlayer(Player& player);
    void HandleAttack(int attackerId, int attackType);

private:
    TcpServer* _network = nullptr;
    std::map<int, Player> _players; // 접속한 유저 목록
};