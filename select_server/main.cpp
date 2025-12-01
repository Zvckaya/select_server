#include <iostream>
#include <chrono>
#include <thread>
#include "Logger.h"
#include "GameServer.h"
#include "Server.h"
#include "NetConfig.h"

int main()
{
    Logger::Instance().Start(); // 로깅 스레드 시작

    
    TcpServer server;
    GameServer gameLogic;
    gameLogic.SetNetwork(&server);
    if (!server.Initialize(&gameLogic))
    {
        Logger::Instance().Log("서버 초기화 실패"); //모든 로깅은 로거를 쓴다. 
        Logger::Instance().Stop();
        return 1;
    }

    Logger::Instance().Log("=== 서버 시작 "+std::to_string(PORT) + "번 포트 ===");

    using namespace std::chrono;
    const auto frameDuration = milliseconds(16);

    while (true)
    {
        auto frameStart = steady_clock::now(); //로직 시작 시간

        server.Tick(); //로직 프레임 

        auto frameEnd = steady_clock::now(); //로직 끝난시간
        auto elapsed = frameEnd - frameStart; //총 걸린시간 

        if (elapsed < frameDuration) //16보다 적게 걸렸을때만 sleep
            std::this_thread::sleep_for(frameDuration - elapsed);
    }

    return 0;
}
