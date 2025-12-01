#pragma once
#include "Session.h"

class INetworkHandler
{

public:

	virtual ~INetworkHandler() = default;

	virtual void OnConnection(Session& session) = 0;

	virtual void OnDisconnection(Session& session) = 0;

	virtual void OnRecv(Session& session, const char* packetData, int len) = 0;

};