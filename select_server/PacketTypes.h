#pragma once
#include <cstdint>

constexpr uint8_t PACKET_CODE = 0x89;

#pragma pack(push, 1) 
enum class PacketType : uint8_t
{
	// Server -> Client
	SC_CREATE_MY_CHARACTER = 0,
	SC_CREATE_OTHER_CHARACTER = 1,
	SC_DELETE_CHARACTER = 2,

	// Move (CS: Client->Server, SC: Server->Client)
	CS_MOVE_START = 10,
	SC_MOVE_START = 11,
	CS_MOVE_STOP = 12,
	SC_MOVE_STOP = 13,

	// Attack
	CS_ATTACK1 = 20,
	SC_ATTACK1 = 21,
	CS_ATTACK2 = 22,
	SC_ATTACK2 = 23,
	CS_ATTACK3 = 24,
	SC_ATTACK3 = 25,

	// Damage & Sync
	SC_DAMAGE = 30,
	CS_SYNC = 250, // 사용 안함이라 되어있지만 정의는 둠
	SC_SYNC = 251,
};


//패킷 구조체 선언

//패킷 헤더(3바이트 고정)
struct PacketHeader
{
	uint8_t byCode; //0x89아니면 컷
	uint8_t bySize; //전체 사이즈
	uint8_t byType;
};


//Client -> Server

//공격
struct Pkt_CS_MoveAttack
{
	PacketHeader header;
	uint8_t  direction;
	int16_t  x;
	int16_t  y;
};

//명세에 있음. 뭔지는 모름
struct Pkt_CS_Sync
{
	PacketHeader header;
	int16_t x;
	int16_t y;
};

//Server->Client

// 내 캐릭터 생성 / 타인 캐릭터 생성 (구조 동일)
struct Pkt_SC_CreateChar
{
	PacketHeader header;
	int32_t id;
	uint8_t direction;
	int16_t x;
	int16_t y;
	uint8_t hp;
};

// 캐릭터 삭제
struct Pkt_SC_DeleteChar
{
	PacketHeader header;
	int32_t id;
};

// 이동 시작/정지, 공격 알림 (구조 동일: ID + Dir + X + Y)
struct Pkt_SC_MoveAttack
{
	PacketHeader header;
	int32_t id;
	uint8_t direction;
	int16_t x;
	int16_t y;
};

// 데미지 알림
struct Pkt_SC_Damage
{
	PacketHeader header;
	int32_t attackId; // 때린 놈
	int32_t damageId; // 맞은 놈
	uint8_t damageHp; // 남은 체력
};

// 서버측 위치 동기화
struct Pkt_SC_Sync
{
	PacketHeader header;
	int32_t id;
	int16_t x;
	int16_t y;
};

#pragma pack(pop)

//inline const char* PacketTypeToString(PacketType type)
//{
//    switch (type)
//    {
//    case PacketType::None:
//        return;
//
//    }
//}
