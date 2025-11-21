//select는 싱글 스레드 기반인데, printf나 cout을 쓰면 표준 출력 스트림에 글을 쓰게됨
//네트워크 i/o및 간단한 서버 로직이라 처리가 빠른데. 표준 출력 스트림을 쓰니 말도안되는 지연이 발생
//로그를 실시간으로 보기 위해서 logger를 생성하여 출력만을 위한 스레드를 만들었다. 

#pragma once

#include <string>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <atomic>

class Logger
{
public:
    static Logger& Instance()
    {
        static Logger instance;
        return instance;
    }

    void Start();
    void Stop();

    void Log(const std::string& msg);

private:
    Logger() = default;
    ~Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    void ThreadFunc(); 

private:
    std::thread worker_;
    std::mutex mutex_;
    std::condition_variable cv_;
    std::queue<std::string> queue_;
    std::atomic<bool> running_{ false };
};
