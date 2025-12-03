
#include "Server.h"
#include "GameServer.h"
#include "NetConfig.h"
#include "Logger.h"
#include <thread>
#include <chrono>
#include <mmsystem.h> 
#pragma comment(lib, "winmm.lib")

int main()
{
	timeBeginPeriod(1); //해상도 수정
	Logger::Instance().Start();

	TcpServer server;
	GameServer gameLogic;
	

	gameLogic.SetNetwork(&server);

	if (!server.Initialize(&gameLogic))
	{
		Logger::Instance().Log("Server init 실패");
		return -1;
	}

	Logger::Instance().Log("Tcp 서버 시작");

	using namespace std::chrono;
	auto frameDuration = milliseconds(20); //50 fps


	while (true)
	{
		auto start = steady_clock::now();
		server.Tick();
		gameLogic.Update();
		
		auto end = steady_clock::now();

		auto elapsed = duration_cast<milliseconds>(end - start);
		if (elapsed < frameDuration)
		{
			std::this_thread::sleep_for(frameDuration - elapsed);
		}

	}

	timeEndPeriod(1);
	return 0;
}