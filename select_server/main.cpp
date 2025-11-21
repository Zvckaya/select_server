
#include <iostream>
#include "Server.h"
#include "NetConfig.h"

int main()
{
    Server server;
    if (!server.Initialize())
    {
        std::cerr << "서버 초기화 실패\n";
        return 1;
    }

    std::cout << "=== 별 서버(Select Model, OOP Packet) 시작 (Port: " << PORT << ") ===\n";

    while (true)
    {
        server.Tick();
        Sleep(16);
    }

    return 0;
}
