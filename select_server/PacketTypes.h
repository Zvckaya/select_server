#pragma once
#include <cstdint>

#pragma pack(push, 1) //padding이 발생하지 않게 선언
//사실 모두 4바이트라 패딩이 발생하지 않지만 일단 패킷 구조체를 선언할때는 하는게 좋다.

enum class PacketType : int32_t
{
    IDAssign = 0,
    StarCreate = 1,
    StarDelete = 2,
    Move = 3,
};

struct RawPacket16
{
    int32_t type;
    int32_t a;
    int32_t b;
    int32_t c;
};

#pragma pack(pop)

inline const char* PacketTypeToString(PacketType type)
{
    switch (type)
    {
    case PacketType::IDAssign:   return "IDAssign";
    case PacketType::StarCreate: return "StarCreate";
    case PacketType::StarDelete: return "StarDelete";
    case PacketType::Move:       return "Move";
    default:                     return "Unknown";
    }
}
