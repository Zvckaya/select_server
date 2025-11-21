// Session.h
#pragma once

#include "WinNetIncludes.h"

struct Session
{
    SOCKET socket = INVALID_SOCKET; //소켓과 세션은 반드시 분리해야한다.
    
    int id = 0;
    int x = 0;
    int y = 0;

    char recvBuffer[4096];
    int recvBytes = 0;

    bool isDead = false;
};
