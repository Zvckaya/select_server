
#include "Packet.h"
#include "Server.h"
#include "Session.h"
#include "NetConfig.h"


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

void PacketIDAssign::Handle(GameServer&, Session&)
{
    //사실 PacketIDAssign이 서버로 올일은 없음
}



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

void PacketStarCreate::Handle(GameServer&, Session&)
{
    //사실 이것도 사용할 일은 없음
}



void PacketStarDelete::FromRaw(const RawPacket16& raw)
{
    id = raw.a; //시작 + 4부터 그냥 읽는다 
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

void PacketStarDelete::Handle(GameServer&, Session&)
{
    // 이것도 쓸일은 없음
}


void PacketMove::FromRaw(const RawPacket16& raw)
{

    id = raw.a; // +4
    x = raw.b; // +8
    y = raw.c; //+12
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

void PacketMove::Handle(GameServer& server, Session& session)
{
    server.UpdatePlayerPosition(id, x, y);

    RawPacket16 raw = ToRaw();

    server.BroadcastPacket(raw, &session);

}



std::unique_ptr<Packet> PacketFactory::CreateFromRaw(const RawPacket16& raw) //raw 패킷을 받아서 자동 캐스팅 및 RAII 객체로 반환
{
    PacketType type = static_cast<PacketType>(raw.type); //단순하게 static_cast 진행

    switch (type)
    {
        //패킷은 RAII 객체로 만들어 반환한다.
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
