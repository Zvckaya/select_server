#include "Packet.h"
#include "PacketBuffer.h"
#include "GameServer.h"
#include "Server.h"
#include "Session.h"
#include "NetConfig.h"
#include "Logger.h"


void PacketMoveStart::Handle(GameServer& server, Session& session)
{
	auto* player = server.GetPlayer(session.id);

	if (player)
	{
		player->x = this->x;
		player->y = this->y;
		player->direction = this->direction;
		player->isMoving = true; //플레이어 이동
	}

	PacketBuffer sendBuf;
	sendBuf.MoveWritePos(sizeof(PacketHeader));

	sendBuf << session.id << this->direction << this->x << this->y;

	PacketHeader* header = (PacketHeader*)sendBuf.GetBufferPtr();
	header->SetInfo(PacketType::SC_MOVE_START, sendBuf.GetDataSize() - sizeof(PacketHeader));
	server.BroadcastPacket(sendBuf.GetBufferPtr(), sendBuf.GetDataSize(), &session);

}


void PacketMoveStop::Handle(GameServer& server, Session& session)
{
	auto* player = server.GetPlayer(session.id);
	if (player)
	{
		// [변경] 멤버변수 사용 (x, y, direction)
		if (abs(player->x - this->x) < dfERROR_RANGE && abs(player->y - this->y) < dfERROR_RANGE)
		{
			player->x = this->x;
			player->y = this->y;
			player->direction = this->direction;
			player->isMoving = false;

			// [송신] PacketBuffer 사용
			PacketBuffer sendBuf;
			sendBuf.MoveWritePos(sizeof(PacketHeader));

			sendBuf << session.id << this->direction << this->x << this->y;

			PacketHeader* header = (PacketHeader*)sendBuf.GetBufferPtr();
			header->SetInfo(PacketType::SC_MOVE_STOP, sendBuf.GetDataSize() - sizeof(PacketHeader));

			server.BroadcastPacket(sendBuf.GetBufferPtr(), sendBuf.GetDataSize(), &session);
		}
		else
		{
			Logger::Instance().Log("[핵 의심 사용자] User " + std::to_string(session.id) +
				"  X:" + std::to_string(abs(player->x - this->x)) +
				" Y:" + std::to_string(abs(player->y - this->y)));
			session.isDead = true;
		}
	}


}

void PacketAttack1::Handle(GameServer& server, Session& session)
{
	auto* player = server.GetPlayer(session.id);

	if (player)
	{
		player->x = this->x;
		player->y = this->y;
		player->direction = this->direction;

		// [송신] 공격 모션 브로드캐스팅
		PacketBuffer sendBuf;
		sendBuf.MoveWritePos(sizeof(PacketHeader));

		sendBuf << session.id << this->direction << this->x << this->y;

		PacketHeader* header = (PacketHeader*)sendBuf.GetBufferPtr();
		header->SetInfo(PacketType::SC_ATTACK1, sendBuf.GetDataSize() - sizeof(PacketHeader));

		server.BroadcastPacket(sendBuf.GetBufferPtr(), sendBuf.GetDataSize(), &session);

		// 피격 판정
		server.HandleAttack(session.id, 1);
	}

}


void PacketAttack2::Handle(GameServer& server, Session& session)
{
	auto* player = server.GetPlayer(session.id);

	if (player)
	{
		player->x = this->x;
		player->y = this->y;
		player->direction = this->direction;

		// [송신] 공격 모션 브로드캐스팅
		PacketBuffer sendBuf;
		sendBuf.MoveWritePos(sizeof(PacketHeader));

		sendBuf << session.id << this->direction << this->x << this->y;

		PacketHeader* header = (PacketHeader*)sendBuf.GetBufferPtr();
		header->SetInfo(PacketType::SC_ATTACK2, sendBuf.GetDataSize() - sizeof(PacketHeader));

		server.BroadcastPacket(sendBuf.GetBufferPtr(), sendBuf.GetDataSize(), &session);

		// 피격 판정
		server.HandleAttack(session.id, 2);
	}
	
}

void PacketAttack3::Handle(GameServer& server, Session& session)
{
	auto* player = server.GetPlayer(session.id);

	if (player)
	{
		player->x = this->x;
		player->y = this->y;
		player->direction = this->direction;

		// [송신] 공격 모션 브로드캐스팅
		PacketBuffer sendBuf;
		sendBuf.MoveWritePos(sizeof(PacketHeader));

		sendBuf << session.id << this->direction << this->x << this->y;

		PacketHeader* header = (PacketHeader*)sendBuf.GetBufferPtr();
		header->SetInfo(PacketType::SC_ATTACK3, sendBuf.GetDataSize() - sizeof(PacketHeader));

		server.BroadcastPacket(sendBuf.GetBufferPtr(), sendBuf.GetDataSize(), &session);

		// 피격 판정
		server.HandleAttack(session.id, 3);
	}
}




std::unique_ptr<Packet> PacketFactory::CreatePacket(PacketHeader* header)
{
	if (header->byCode != PACKET_CODE)
		return nullptr;

	PacketType type = static_cast<PacketType>(header->byType);

	char* payloadPtr = (char*)header + sizeof(PacketHeader); //진짜 데이터 위치 
	int bodySize = header->bySize; //바디 사이즈 

	PacketBuffer reader;
	reader.PutData(payloadPtr, bodySize); //3바이트 뒤부터 바디사이즈만큼 읽어 ~

	std::unique_ptr<Packet> pkt = nullptr;


	switch (type)
	{
	case PacketType::CS_MOVE_START:
		pkt = std::make_unique<PacketMoveStart>();
		break;

	case PacketType::CS_MOVE_STOP:
		pkt = std::make_unique<PacketMoveStop>();
		break;

	case PacketType::CS_ATTACK1:
		pkt = std::make_unique<PacketAttack1>();
		break;

	case PacketType::CS_ATTACK2:
		pkt = std::make_unique<PacketAttack2>();
		break;

	case PacketType::CS_ATTACK3:
		pkt = std::make_unique<PacketAttack3>();
		break;

	default:
		return nullptr;
	}

	if (pkt)
	{
		// reader >> 변수1 >> 변수2 ... 실행됨
		if (pkt->Decode(reader) == false) //깡으로 decoe 

			return nullptr;
	}

	return pkt;

}