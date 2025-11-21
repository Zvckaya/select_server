// Packet.cpp
#include "Packet.h"
#include "Server.h"
#include "Session.h"
#include "NetConfig.h"

// ================= PacketIDAssign =================

void PacketIDAssign::FromRaw(const RawPacket16& raw)
{
    id = raw.b;
}

RawPacket16 PacketIDAssign::ToRaw() const
{
    RawPacket16 raw{};
    raw.type = static_cast<int32_t>(PacketType::IDAssign);
    raw.a = id;
    raw.b = 0;
    raw.c = 0;
    return raw;
}

void PacketIDAssign::Handle(Server&, Session&)
{
    // 서버는 클라이언트로부터 IDAssign을 받지 않음.
    // 필요하다면 로그 정도만 남기게 구현 가능.
}


// ================= PacketStarCreate =================

void PacketStarCreate::FromRaw(const RawPacket16& raw)
{
    id = raw.b;
    x = raw.c; // 원 코드: type, id, x, y였는데 Raw 구조 통합에 맞춰 사용
    // 여기서는 (type, id, x, y)를 (type, a, b, c)처럼 매핑하려면
    // 프로토콜에 맞게 조정 가능. 예: a=id, b=x, c=y 등.
    // 아래 ToRaw와 일관되게 맞춰야 함.
}

RawPacket16 PacketStarCreate::ToRaw() const
{
    RawPacket16 raw{};
    raw.type = static_cast<int32_t>(PacketType::StarCreate);
    raw.a = id;
    raw.b = x;
    raw.c = y;
    return raw;
}

void PacketStarCreate::Handle(Server&, Session&)
{
    // 클라이언트 → 서버에서는 사용 안 할 예정
    // 필요 시 확장
}


// ================= PacketStarDelete =================

void PacketStarDelete::FromRaw(const RawPacket16& raw)
{
    id = raw.a;
}

RawPacket16 PacketStarDelete::ToRaw() const
{
    RawPacket16 raw{};
    raw.type = static_cast<int32_t>(PacketType::StarDelete);
    raw.a = id;
    raw.b = 0;
    raw.c = 0;
    return raw;
}

void PacketStarDelete::Handle(Server&, Session&)
{
    // 클라이언트 → 서버에서는 사용 안 함 (서버가 주로 보내는 패킷)
}


// ================= PacketMove =================

void PacketMove::FromRaw(const RawPacket16& raw)
{
    id = raw.a;
    x = raw.b;
    y = raw.c;
}

RawPacket16 PacketMove::ToRaw() const
{
    RawPacket16 raw{};
    raw.type = static_cast<int32_t>(PacketType::Move);
    raw.a = id;
    raw.b = x;
    raw.c = y;
    return raw;
}

void PacketMove::Handle(Server& server, Session& session)
{
    // 서버 좌표 갱신
    session.x = x;
    session.y = y;

    // 나를 제외하고 이동 브로드캐스트
    RawPacket16 raw = ToRaw();
    server.Broadcast(raw, &session);
}


// ================= PacketFactory =================

std::unique_ptr<Packet> PacketFactory::CreateFromRaw(const RawPacket16& raw)
{
    PacketType type = static_cast<PacketType>(raw.type);

    switch (type)
    {
    case PacketType::IDAssign:
        return std::make_unique<PacketIDAssign>();
    case PacketType::StarCreate:
        return std::make_unique<PacketStarCreate>();
    case PacketType::StarDelete:
        return std::make_unique<PacketStarDelete>();
    case PacketType::Move:
        return std::make_unique<PacketMove>();
    default:
        return nullptr;
    }
}
