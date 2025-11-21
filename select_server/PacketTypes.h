#pragma once
#include <cstdint>

#pragma pack(push, 1)

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
