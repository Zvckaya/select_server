#include "GameServer.h"
#include "Packet.h"
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
		Pkt_SC_CreateChar pkt;
		pkt.header.SetInfo(PacketType::SC_CREATE_MY_CHARACTER, sizeof(Pkt_SC_CreateChar) - sizeof(PacketHeader));
		pkt.id = newPlayer.id;
		pkt.x = newPlayer.x;
		pkt.y = newPlayer.y;
		pkt.direction = newPlayer.direction;
		pkt.hp = newPlayer.hp;

		SendPacket(session, &pkt, sizeof(pkt));
	}

	{
		Pkt_SC_CreateChar pkt;

		pkt.header.SetInfo(PacketType::SC_CREATE_OTHER_CHARACTER, sizeof(Pkt_SC_CreateChar) - sizeof(PacketHeader));

		pkt.id = newPlayer.id;
		pkt.x = newPlayer.x;
		pkt.y = newPlayer.y;
		pkt.direction = newPlayer.direction;
		pkt.hp = newPlayer.hp;


		// 나(session)를 제외하고 모두에게 전송
		BroadcastPacket(&pkt, sizeof(pkt), &session);
	}

	for (auto& pair : _players)
	{
		if (pair.first == session.id) continue; // 나는 이미 위에서 처리함

		Player& other = pair.second;

		Pkt_SC_CreateChar pkt;

		pkt.header.SetInfo(PacketType::SC_CREATE_OTHER_CHARACTER, sizeof(Pkt_SC_CreateChar) - sizeof(PacketHeader));

		pkt.id = other.id;
		pkt.x = other.x;
		pkt.y = other.y;
		pkt.direction = other.direction;
		pkt.hp = other.hp;


		SendPacket(session, &pkt, sizeof(pkt));
	}
}


void GameServer::OnDisconnection(Session& session)
{
	// 1. 목록에서 제거
	_players.erase(session.id);

	// 2. 다른 유저들에게 삭제 알림 (SC_DELETE_CHARACTER)
	Pkt_SC_DeleteChar pkt;
	pkt.header.SetInfo(PacketType::SC_DELETE_CHARACTER, sizeof(Pkt_SC_DeleteChar) - sizeof(PacketHeader));
	pkt.id = session.id;

	BroadcastPacket(&pkt, sizeof(pkt));
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
	// 1. 방향에 따른 좌표 이동
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

	//범위체크
	// Left
	if (p.x < RANGE_MOVE_LEFT) p.x = RANGE_MOVE_LEFT;

	// Right
	if (p.x > RANGE_MOVE_RIGHT) p.x = RANGE_MOVE_RIGHT;

	// Top (화면 좌표계에서 Y가 작을수록 위쪽)
	if (p.y < RANGE_MOVE_TOP) p.y = RANGE_MOVE_TOP;

	// Bottom
	if (p.y > RANGE_MOVE_BOTTOM) p.y = RANGE_MOVE_BOTTOM;

	//Logger::Instance().Log("id:" + std::to_string(p.id) + " x: " + std::to_string(p.x) + " p.y" + std::to_string(p.y));
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
	// 1. 공격자 찾기
	Player* attacker = GetPlayer(attackerId);
	if (attacker == nullptr) return;

	// 2. 공격 범위 및 데미지 설정
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

	// 3. 피격 판정 루프 (모든 유저 검사)
	// (최적화를 위해선 쿼드트리 등을 쓰지만, 60명이면 그냥 루프 돌아도 충분함)
	for (auto& pair : _players)
	{
		int victimId = pair.first;
		Player& victim = pair.second;

		// 나 자신은 공격 안 함
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

		// 4. 맞았다면?
		if (isHit)
		{
			// 체력 감소 (음수 방지)
			if (victim.hp > damage)
				victim.hp -= (uint8_t)damage;
			else
				victim.hp = 0;

			Pkt_SC_Damage dmgPkt;
			dmgPkt.header.SetInfo(PacketType::SC_DAMAGE, sizeof(Pkt_SC_Damage) - sizeof(PacketHeader));
			dmgPkt.attackId = attackerId;
			dmgPkt.damageId = victimId;
			dmgPkt.damageHp = victim.hp;

			BroadcastPacket(&dmgPkt, sizeof(dmgPkt));


			// 만약 죽었다면? (HP 0) -> DELETE 패킷 처리
			if (victim.hp == 0)
			{
				// 죽음 처리 로직 (나중에 부활 등 필요하면 여기서 처리)
				Pkt_SC_DeleteChar delPkt;
				delPkt.header.SetInfo(PacketType::SC_DELETE_CHARACTER, sizeof(Pkt_SC_DeleteChar) - sizeof(PacketHeader));
				delPkt.id = victimId;
				_players.erase(victimId);
				BroadcastPacket(&delPkt, sizeof(delPkt));
				
				//KickUser(victimId);

			}
		}
	}
}

void GameServer::KickUser(int id)
{
	if (_network) 
		_network->KickSession(id);
}