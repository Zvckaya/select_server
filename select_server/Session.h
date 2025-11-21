// Session.h
#pragma once

#include "WinNetIncludes.h"

struct Session
{
    SOCKET socket = INVALID_SOCKET;
    int id = 0;
    int x = 0;
    int y = 0;

    char recvBuffer[4096];
    int recvBytes = 0;

    bool isDead = false;
};
