#include "GameServer.h"
#include "Packet.h"

void GameServer::OnConnection(Session& session)  {
    Player newPlayer;
    newPlayer.x = 0;
    newPlayer.y = 0;
    _players[session.id] = newPlayer;

    if (_network)
    {
        {
            //패킷의 id, 즉 무슨 패킷 인지는 enum으로 send에서 봄
            PacketIDAssign pkt;
            pkt.id = session.id;
            RawPacket16 raw = pkt.ToRaw();

            _network->SendTo(session, raw); //sendto는 특정 대상에게 전송하는 함수임
            //먼저 id를 할당한다. 클라는 이 id를 이용하여 로직통신

        }
        // 내 별 생성 패킷 (나 + 기존 유저들)
        {
            PacketStarCreate createMy;
            createMy.id = session.id;
            createMy.x = newPlayer.x;
            createMy.y = newPlayer.y;
            RawPacket16 raw = createMy.ToRaw();


            //먼저 나에게 별 생성 하라 전송 
            _network->SendTo(session, raw);
            // 기존 유저들에게 별 생성 
            _network->Broadcast(raw, &session);
        }
        // 기존 유저들의 별 정보를 신규 유저에게 전송 (이때 부하가 큼 신규유저는)
        for (auto& pair : _players) {
            int otherId = pair.first;
            Player& otherPlayer = pair.second;

            if (otherId == session.id)
                continue; //나는 제외

            PacketStarCreate createOther;
            createOther.id = otherId;
            createOther.x = otherPlayer.x;
            createOther.y = otherPlayer.y;

            RawPacket16 raw = createOther.ToRaw();

            _network->SendTo(session,raw);
        }
    }
    

}

void GameServer::OnDisconnection(Session& session)  {
    _players.erase(session.id);

    PacketStarDelete del;
    del.id = session.id;
    RawPacket16 raw = del.ToRaw();

    BroadcastPacket(raw, &session);
}

void GameServer::OnRecv(Session& session, const char* packetData, int len)  {
    RawPacket16 raw;
    memcpy(&raw, packetData, len);

    auto packet = PacketFactory::CreateFromRaw(raw);

    if (packet)
    {
        packet->FromRaw(raw);

        packet->Handle(*this, session);
    }
}


void GameServer::SendPacket(Session& session, const RawPacket16& raw)
{
    if (_network) _network->SendTo(session, raw);
}

void GameServer::BroadcastPacket(const RawPacket16& raw, Session* exclude)
{
    if (_network) _network->Broadcast(raw, exclude);
}

void GameServer::UpdatePlayerPosition(int id, int x, int y)
{
    if (_players.find(id) != _players.end())
    {
        _players[id].x = x;
        _players[id].y = y;
    }
}