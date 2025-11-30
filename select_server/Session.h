// Session.h
#pragma once

#include "WinNetIncludes.h"
#include "CRingBuffer.h"

struct Session
{
    SOCKET socket = INVALID_SOCKET; //소켓과 세션은 반드시 분리해야한다.

    CRingBuffer recvBuffer;
    CRingBuffer sendBuffer;

    bool isDead = false;

};
