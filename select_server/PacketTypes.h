#pragma once
#include <cstdint>

#pragma pack(push, 1) 
enum class PacketType : int32_t
{
    None=0,
};


struct RawPacket16
{
    int32_t type;
    int32_t a;
    int32_t b;
    int32_t c;
};
//패킷 구조체 선언

#pragma pack(pop)

inline const char* PacketTypeToString(PacketType type)
{
    switch (type)
    {
    case PacketType::None:
        return;

    }
}
