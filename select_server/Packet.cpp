
#include "Packet.h"
#include "Server.h"
#include "Session.h"
#include "NetConfig.h"




std::unique_ptr<Packet> PacketFactory::CreateFromRaw(const RawPacket16& raw) //raw 패킷을 받아서 자동 캐스팅 및 RAII 객체로 반환
{
    PacketType type = static_cast<PacketType>(raw.type); //단순하게 static_cast 진행

    switch (type)
    {
    case PacketType::None:
        return;
    default:
        return nullptr;
    }
}
