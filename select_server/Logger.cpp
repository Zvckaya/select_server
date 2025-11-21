#include "Logger.h"
#include <iostream>

void Logger::Start()
{
    bool expected = false;
    if (!running_.compare_exchange_strong(expected, true))
        return; // 이미 실행중임... 

    worker_ = std::thread(&Logger::ThreadFunc, this); //없으면 스레드 하나 팜.
}

void Logger::Stop()
{
    bool expected = true;
    if (!running_.compare_exchange_strong(expected, false))
        return; // running을 false로, 워커 스레드 정리 

    cv_.notify_all();

    if (worker_.joinable()) //정리 절차 진행
        worker_.join(); //실제 정리
}

void Logger::Log(const std::string& msg)
{
    {
        std::lock_guard<std::mutex> lock(mutex_); //이름은 Log지만 사실 push에 가까움
        queue_.push(msg); //로깅할내용 push, queue는 string의 queue
    }
    cv_.notify_one(); //lock을 대기하는 스레드가 있으면 깨워라.
}

void Logger::ThreadFunc() //실제 스레드 작업 
{
    std::unique_lock<std::mutex> lock(mutex_);

    while (running_ || !queue_.empty()) //스레드를 계속 돌릴지 종료할지 결정한다.
        //할일도 없고, stop이 종료되었으면 그냥 끝내기 
    {
        cv_.wait(lock, [this]() { 
            return !running_ || !queue_.empty(); //조건에 만족하지 않으면 그냥 block 상태임 cpu 사용 0 -> 누군가 notify_one 해야 wake
            });

        while (!queue_.empty()) 
        {
            std::string msg = std::move(queue_.front());
            queue_.pop();

            // 잠금 풀고 실제 출력진행
            lock.unlock();
            std::cout << msg << '\n'; // endl 쓰지 말고 '\n'만!
            lock.lock();
        }
    }
}
