#pragma once

#include "PacketTypes.h"

class GameServer;
struct Session;

// 패킷 기반 클래스
class Packet
{
public:
    virtual ~Packet() = default;
    virtual PacketType GetType() const = 0;
    virtual void FromRaw(const RawPacket16& raw) = 0;
    virtual RawPacket16 ToRaw() const = 0;

    // 패킷 처리 (서버 입장)
    virtual void Handle(GameServer& server, Session& session) = 0;
};

// 개별 패킷 클래스들 -----------------------

// 팩토리 ----------------------------
#include <memory>
class PacketFactory
{
public:
    static std::unique_ptr<Packet> CreateFromRaw(const RawPacket16& raw);
};
