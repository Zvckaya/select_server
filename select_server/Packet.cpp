#include "Packet.h"
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
		player->x = data.x;
		player->y = data.y;
		player->direction = data.direction;
		player->isMoving = true; //플레이어 이동
	}

	Pkt_SC_MoveAttack sendPkt;

	sendPkt.header.SetInfo(PacketType::SC_MOVE_START, sizeof(Pkt_SC_MoveAttack) - sizeof(PacketHeader));
	sendPkt.id = session.id;
	sendPkt.x = data.x;
	sendPkt.y = data.y;
	sendPkt.direction = data.direction;

	server.BroadcastPacket((char*)&sendPkt, sizeof(sendPkt), &session);

}


void PacketMoveStop::Handle(GameServer& server, Session& session)
{
	auto* player = server.GetPlayer(session.id);
	if (player)
	{
		if (abs(player->x - data.x) < dfERROR_RANGE && abs(player->y - data.y) < dfERROR_RANGE)
		{
			player->x = data.x; // 클라가 보낸 최종 좌표로 보정	
			player->y = data.y;
			player->direction = data.direction;
			player->isMoving = false;

			Pkt_SC_MoveAttack sendPkt;
			sendPkt.header.SetInfo(PacketType::SC_MOVE_STOP, sizeof(Pkt_SC_MoveAttack) - sizeof(PacketHeader));
			sendPkt.id = session.id;
			sendPkt.x = data.x;
			sendPkt.y = data.y;
			sendPkt.direction = data.direction;

			server.BroadcastPacket((char*)&sendPkt, sizeof(sendPkt), &session);
		}
		else {
			Logger::Instance().Log("[핵 의심 사용자] User " + std::to_string(session.id) +
				"  X:" + std::to_string(abs(player->x - data.x)) +
				" Y:" + std::to_string(abs(player->y - data.y)));
			session.isDead = true;
		}
		
	}

	
}

void PacketAttack1::Handle(GameServer& server, Session& session)
{
	//// 1. 공격 방향/위치 업데이트 (필요 시)
	//server.UpdatePlayer(session.id, data.x, data.y, data.direction);

	//// 2. 다른 유저들에게 공격 모션 알림 (SC_ATTACK1)
	//Pkt_SC_MoveAttack sendPkt;
	//sendPkt.header.byCode = PACKET_CODE;
	//sendPkt.header.bySize = sizeof(Pkt_SC_MoveAttack);
	//sendPkt.header.byType = (uint8_t)PacketType::SC_ATTACK1;

	//sendPkt.id = session.id;
	//sendPkt.x = data.x;
	//sendPkt.y = data.y;
	//sendPkt.direction = data.direction;

	//server.BroadcastPacket((char*)&sendPkt, sizeof(sendPkt), &session);

	// TODO: 여기서 충돌 판정(Damage) 로직을 추가할 수 있음
	// server.ProcessAttack(session.id, ...);
}

std::unique_ptr<Packet> PacketFactory::CreatePacket(PacketHeader* header)
{
	if (header->byCode != PACKET_CODE)
		return nullptr;

	PacketType type = static_cast<PacketType>(header->byType);

	switch (type)
	{
	case PacketType::CS_MOVE_START:
	{
		auto pkt = std::make_unique<PacketMoveStart>();
		memcpy(&pkt->data, header, sizeof(Pkt_CS_MoveAttack));
		return pkt;
	}

	case PacketType::CS_MOVE_STOP:
	{
		auto pkt = std::make_unique<PacketMoveStop>();
		memcpy(&pkt->data, header, sizeof(Pkt_CS_MoveAttack));
		return pkt;
	}

	case PacketType::CS_ATTACK1:
	{
		auto pkt = std::make_unique<PacketMoveStart>();
		memcpy(&pkt->data, header, sizeof(Pkt_CS_MoveAttack));
		return pkt;
	}

	default:
		return nullptr;
	}

}