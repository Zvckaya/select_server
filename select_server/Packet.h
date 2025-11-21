#pragma once

#include "PacketTypes.h"

class Server;
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
    virtual void Handle(Server& server, Session& session) = 0;
};

// 개별 패킷 클래스들 -----------------------

class PacketIDAssign : public Packet
{
public:
    int id = 0;

    PacketType GetType() const override { return PacketType::IDAssign; }
    void FromRaw(const RawPacket16& raw) override;
    RawPacket16 ToRaw() const override;
    void Handle(Server& server, Session& session) override;
};

class PacketStarCreate : public Packet
{
public:
    int id = 0;
    int x = 0;
    int y = 0;

    PacketType GetType() const override { return PacketType::StarCreate; }
    void FromRaw(const RawPacket16& raw) override;
    RawPacket16 ToRaw() const override;
    void Handle(Server& server, Session& session) override;
};

class PacketStarDelete : public Packet
{
public:
    int id = 0;

    PacketType GetType() const override { return PacketType::StarDelete; }
    void FromRaw(const RawPacket16& raw) override;
    RawPacket16 ToRaw() const override;
    void Handle(Server& server, Session& session) override;
};

class PacketMove : public Packet
{
public:
    int id = 0;
    int x = 0;
    int y = 0;

    PacketType GetType() const override { return PacketType::Move; }
    void FromRaw(const RawPacket16& raw) override;
    RawPacket16 ToRaw() const override;
    void Handle(Server& server, Session& session) override;
};

// 팩토리 ----------------------------
#include <memory>

class PacketFactory
{
public:
    static std::unique_ptr<Packet> CreateFromRaw(const RawPacket16& raw);
};
