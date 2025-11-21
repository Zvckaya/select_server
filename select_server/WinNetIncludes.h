// WinNetIncludes.h
#pragma once

// 여기가 "윈도우/윈속 헤더의 단일 진입점" 이라고 생각하면 됨.

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

// Winsock2는 windows.h 보다 먼저!
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")
