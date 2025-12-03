#pragma once
#include "PacketTypes.h"
#include "PacketBuffer.h"

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

    //직렬화
    virtual bool Decode(PacketBuffer& reader) { return true; }
    virtual void Encode(PacketBuffer& writer){}
    
};

// 개별 패킷 클래스들 -----------------------

//이동 시작
class PacketMoveStart : public Packet
{
public:
   
    uint8_t direction;
    int16_t x;
    int16_t y;

    PacketType GetType() const override { return PacketType::CS_MOVE_START; }

    void Handle(GameServer& server, Session& session) override;

    bool Decode(PacketBuffer& reader) override
    {
        reader >> direction >> x >> y;
        return true;
    }
    void Encode(PacketBuffer& writer) override
    {
        writer << direction << x << y;
    }
};

class PacketMoveStop : public Packet
{
public:
    uint8_t  direction;
    int16_t  x;
    int16_t  y;

    PacketType GetType() const override { return PacketType::CS_MOVE_STOP; }

    void Handle(GameServer& server, Session& session) override;

    bool Decode(PacketBuffer& reader) override
    {
        reader >> direction >> x >> y;
        return true;
    }
    void Encode(PacketBuffer& writer) override
    {
        writer << direction << x << y;
    }
};


class PacketAttack1 : public Packet
{
public:
  
    uint8_t  direction;
    int16_t  x;
    int16_t  y;

    PacketType GetType() const override { return PacketType::CS_ATTACK1; }

    void Handle(GameServer& server, Session& session) override;

    bool Decode(PacketBuffer& reader) override
    {
        reader >> direction >> x >> y;
        return true;
    }
    void Encode(PacketBuffer& writer) override
    {
        writer << direction << x << y;
    }
};

class PacketAttack2 : public Packet
{
public:
    uint8_t  direction;
    int16_t  x;
    int16_t  y;


    PacketType GetType() const override { return PacketType::CS_ATTACK2; }

    void Handle(GameServer& server, Session& session) override;

    bool Decode(PacketBuffer& reader) override
    {
        reader >> direction >> x >> y;
        return true;
    }
    void Encode(PacketBuffer& writer) override
    {
        writer << direction << x << y;
    }
};

class PacketAttack3 : public Packet
{
public:
    uint8_t  direction;
    int16_t  x;
    int16_t  y;

    PacketType GetType() const override { return PacketType::CS_ATTACK3; }

    void Handle(GameServer& server, Session& session) override;

    bool Decode(PacketBuffer& reader) override
    {
        reader >> direction >> x >> y;
        return true;
    }
    void Encode(PacketBuffer& writer) override
    {
        writer << direction << x << y;
    }
};


// 팩토리 ----------------------------
#include <memory>
class PacketFactory
{
public:
    static std::unique_ptr<Packet> CreatePacket(PacketHeader* header);
};
