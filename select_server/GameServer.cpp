#include "GameServer.h"
#include "Packet.h"
#include "PacketBuffer.h"
#include "Server.h"
#include "NetConfig.h"
#include "Logger.h"

#include <iostream>
#include <iomanip>
#include <sstream>
#include <random>

GameServer::GameServer()
{
	//생성자...
}

void GameServer::OnConnection(Session& session)
{
	Player newPlayer;
	newPlayer.id = session.id;
	newPlayer.x = RANGE_MOVE_LEFT + (rand() % (RANGE_MOVE_RIGHT - RANGE_MOVE_LEFT + 1));
	newPlayer.y = RANGE_MOVE_TOP + (rand() % (RANGE_MOVE_BOTTOM - RANGE_MOVE_TOP + 1));
	newPlayer.direction = dfPACKET_MOVE_DIR_LL;
	newPlayer.hp = 100;

	_players[session.id] = newPlayer;

	{
		PacketBuffer pkt;
		pkt.MoveWritePos(sizeof(PacketHeader));

		pkt << newPlayer.id << newPlayer.direction << newPlayer.x << newPlayer.y << newPlayer.hp;

		PacketHeader* header = (PacketHeader*)pkt.GetBufferPtr();
		header->SetInfo(PacketType::SC_CREATE_MY_CHARACTER, pkt.GetDataSize() - sizeof(PacketHeader));

		SendPacket(session, pkt.GetBufferPtr(), pkt.GetDataSize());
	}

	{
		PacketBuffer pkt;
		pkt.MoveWritePos(sizeof(PacketHeader));

		// 구조 동일: id, direction, x, y, hp
		pkt << newPlayer.id << newPlayer.direction << newPlayer.x << newPlayer.y << newPlayer.hp;

		PacketHeader* header = (PacketHeader*)pkt.GetBufferPtr();
		header->SetInfo(PacketType::SC_CREATE_OTHER_CHARACTER, pkt.GetDataSize() - sizeof(PacketHeader));

		// 나(session)를 제외하고 모두에게 전송
		BroadcastPacket(pkt.GetBufferPtr(), pkt.GetDataSize(), &session);
	}

	for (auto& pair : _players)
	{
		if (pair.first == session.id) continue; // 나는 이미 위에서 처리함

		Player& other = pair.second;

		PacketBuffer pkt;
		pkt.MoveWritePos(sizeof(PacketHeader));

		pkt << other.id << other.direction << other.x << other.y << other.hp;

		PacketHeader* header = (PacketHeader*)pkt.GetBufferPtr();
		header->SetInfo(PacketType::SC_CREATE_OTHER_CHARACTER, pkt.GetDataSize() - sizeof(PacketHeader));

		SendPacket(session, pkt.GetBufferPtr(), pkt.GetDataSize());
	}
}


void GameServer::OnDisconnection(Session& session)
{
	_players.erase(session.id);

	// SC_DELETE_CHARACTER 패킷 전송
	PacketBuffer pkt;
	pkt.MoveWritePos(sizeof(PacketHeader));

	pkt << session.id; // 삭제할 ID

	PacketHeader* header = (PacketHeader*)pkt.GetBufferPtr();
	header->SetInfo(PacketType::SC_DELETE_CHARACTER, pkt.GetDataSize() - sizeof(PacketHeader));

	BroadcastPacket(pkt.GetBufferPtr(), pkt.GetDataSize());
}

void GameServer::OnRecv(Session& session, const char* packetData, int len)
{
	auto packet = PacketFactory::CreatePacket((PacketHeader*)packetData);

	if (packet)
	{
		packet->Handle(*this, session);
	}
	else
	{
		return; 
	}
}

void GameServer::SendPacket(Session& session, void* ptr, int size)
{
	if (_network)
		_network->SendTo(session, (char*)ptr, size);
}

void GameServer::BroadcastPacket(void* ptr, int size, Session* exclude)
{
	if (_network)
		_network->Broadcast((char*)ptr, size, exclude);
}




void GameServer::UpdatePlayers()
{
	for (auto& pair : _players)
	{
		Player& p = pair.second;
		if (p.isMoving)
		{
			MovePlayer(p);
		}
	}
}

void GameServer::MovePlayer(Player& p)
{
	switch (p.direction)
	{
	case dfPACKET_MOVE_DIR_LL: // 좌
		p.x -= MOVE_UNIT_X;
		break;
	case dfPACKET_MOVE_DIR_LU: // 좌상
		p.x -= MOVE_UNIT_X;
		p.y -= MOVE_UNIT_Y;
		break;
	case dfPACKET_MOVE_DIR_UU: // 상
		p.y -= MOVE_UNIT_Y;
		break;
	case dfPACKET_MOVE_DIR_RU: // 우상
		p.x += MOVE_UNIT_X;
		p.y -= MOVE_UNIT_Y;
		break;
	case dfPACKET_MOVE_DIR_RR: // 우
		p.x += MOVE_UNIT_X;
		break;
	case dfPACKET_MOVE_DIR_RD: // 우하
		p.x += MOVE_UNIT_X;
		p.y += MOVE_UNIT_Y;
		break;
	case dfPACKET_MOVE_DIR_DD: // 하
		p.y += MOVE_UNIT_Y;
		break;
	case dfPACKET_MOVE_DIR_LD: // 좌하
		p.x -= MOVE_UNIT_X;
		p.y += MOVE_UNIT_Y;
		break;
	}

	if (p.x < RANGE_MOVE_LEFT) p.x = RANGE_MOVE_LEFT;

	if (p.x > RANGE_MOVE_RIGHT) p.x = RANGE_MOVE_RIGHT;

	if (p.y < RANGE_MOVE_TOP) p.y = RANGE_MOVE_TOP;

	if (p.y > RANGE_MOVE_BOTTOM) p.y = RANGE_MOVE_BOTTOM;

}

Player* GameServer::GetPlayer(int id)
{
	auto it = _players.find(id);
	if (it != _players.end())
	{
		return &it->second;
	}
	return nullptr;

}
void GameServer::Update()
{
	UpdatePlayers();
}

void GameServer::HandleAttack(int attackerId, int attackType)
{
	Player* attacker = GetPlayer(attackerId);
	if (attacker == nullptr) return;
	
	int rangeX = 0;
	int rangeY = 0;
	int damage = 0;

	switch (attackType)
	{
	case 1:
		rangeX = dfATTACK1_RANGE_X; // 80
		rangeY = dfATTACK1_RANGE_Y; // 10
		damage = 10; // 주먹 데미지
		break;
	case 2:
		rangeX = dfATTACK2_RANGE_X; // 90
		rangeY = dfATTACK2_RANGE_Y; // 10
		damage = 20; // 큰주먹
		break;
	case 3:
		rangeX = dfATTACK3_RANGE_X; // 100
		rangeY = dfATTACK1_RANGE_Y; // 20
		damage = 30; // 발차기
		break;
	default:
		return;
	}

	for (auto& pair : _players)
	{
		int victimId = pair.first;
		Player& victim = pair.second;

		if (victimId == attackerId) continue;
		// 이미 죽은 사람은 안 때림
		if (victim.hp <= 0) continue;

		// 공격자의 Y를 기준으로 위아래 RangeY 안에 있어야 함
		if (abs(attacker->y - victim.y) > rangeY)
			continue; // 높이가 안 맞음 (빗나감)

		//범위체크
		bool isHit = false;

		// 공격자가 왼쪽(LL)을 보고 있을 때: (Attacker.X - Range) ~ (Attacker.X)
		if (attacker->direction == dfPACKET_MOVE_DIR_LL)
		{
			if (victim.x >= (attacker->x - rangeX) && victim.x <= attacker->x)
			{
				isHit = true;
			}
		}
		// 공격자가 오른쪽(RR)을 보고 있을 때: (Attacker.X) ~ (Attacker.X + Range)
		else
		{
			// 대각선이나 위/아래를 보고 있어도 보통 오른쪽 판정으로 퉁치거나, 
			// 정확히 하려면 8방향 다 따져야 하지만 일단 RR기준으로 처리
			if (victim.x >= attacker->x && victim.x <= (attacker->x + rangeX))
			{
				isHit = true;
			}
		}

		if (isHit)
		{
	
			// 체력 감소 (음수 방지)
			if (victim.hp > damage)
				victim.hp -= (uint8_t)damage;
			else
				victim.hp = 0;

			// [수정] 데미지 패킷 (SC_DAMAGE) - PacketBuffer 사용
			PacketBuffer dmgPkt;
			dmgPkt.MoveWritePos(sizeof(PacketHeader));

			// 구조: AttackerID -> DamageID -> DamageHP
			dmgPkt << attackerId << victimId << victim.hp;

			PacketHeader* header = (PacketHeader*)dmgPkt.GetBufferPtr();
			header->SetInfo(PacketType::SC_DAMAGE, dmgPkt.GetDataSize() - sizeof(PacketHeader));

			BroadcastPacket(dmgPkt.GetBufferPtr(), dmgPkt.GetDataSize());


			// 만약 죽었다면? (HP 0) -> DELETE 패킷 처리
			if (victim.hp == 0)
			{
				PacketBuffer delPkt;
				delPkt.MoveWritePos(sizeof(PacketHeader));
				delPkt << victimId;

				PacketHeader* delHeader = (PacketHeader*)delPkt.GetBufferPtr();
				delHeader->SetInfo(PacketType::SC_DELETE_CHARACTER, delPkt.GetDataSize() - sizeof(PacketHeader));

				_players.erase(victimId);
				BroadcastPacket(delPkt.GetBufferPtr(), delPkt.GetDataSize());

				KickUser(victimId);
		
			}
		}
	}
}

void GameServer::KickUser(int id)
{
	if (_network) 
		_network->KickSession(id);
}