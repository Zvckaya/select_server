#pragma once
#include "PacketTypes.h"

class GameServer;
struct Session;

// 패킷 기반 클래스
class Packet
{
public:
    virtual ~Packet() = default;
    
    //패킷 타입 변환
    virtual PacketType GetType() const = 0;
    
    // 패킷 처리 (게임 서버에서 로직 구현)
    virtual void Handle(GameServer& server, Session& session) = 0;

    
};

// 개별 패킷 클래스들 -----------------------

//이동 시작
class PacketMoveStart : public Packet
{
public:
    // 실제 데이터 구조체를 멤버로 가짐
    Pkt_CS_MoveAttack data;

    PacketType GetType() const override { return PacketType::CS_MOVE_START; }

    void Handle(GameServer& server, Session& session) override;
};

class PacketMoveStop : public Packet
{
public:
    Pkt_CS_MoveAttack data;

    PacketType GetType() const override { return PacketType::CS_MOVE_STOP; }

    void Handle(GameServer& server, Session& session) override;
};


class PacketAttack1 : public Packet
{
public:
    Pkt_CS_MoveAttack data;

    PacketType GetType() const override { return PacketType::CS_ATTACK1; }

    void Handle(GameServer& server, Session& session) override;
};

class PacketAttack2 : public Packet
{
public:
    Pkt_CS_MoveAttack data;

    PacketType GetType() const override { return PacketType::CS_ATTACK2; }

    void Handle(GameServer& server, Session& session) override;
};

class PacketAttack3 : public Packet
{
public:
    Pkt_CS_MoveAttack data;

    PacketType GetType() const override { return PacketType::CS_ATTACK3; }

    void Handle(GameServer& server, Session& session) override;
};


// 팩토리 ----------------------------
#include <memory>
class PacketFactory
{
public:
    static std::unique_ptr<Packet> CreatePacket(PacketHeader* header);
};
