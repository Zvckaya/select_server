# C++ High-Performance Single-Threaded Game Server

Windows Socket API (Winsock)와 select 모델을 기반으로 밑바닥부터 구현한 비동기 싱글 스레드 TCP 게임 서버입니다.
Boost.Asio 같은 상용 네트워크 라이브러리를 사용하지 않고, 네트워크 코어와 게임 콘텐츠 로직을 분리하는 아키텍처를 직접 설계하여, 고성능 네트워크 프로그래밍의 핵심 원리를 구현하는 데 집중했습니다.

# ⚙️ Tech Stack

Language: C++17

Platform: Windows (Winsock2)

Network Model: Select-based Non-blocking I/O

Architecture: Single Threaded Event Loop + Async Logger Thread

Tools: Visual Studio 2019+

# 🎯 Project Goals & Architecture Philosophy

이 프로젝트는 단순한 에코 서버를 넘어서, 실제 MMORPG 서버에서 사용하는 것과 유사한 계층형 아키텍처(Layered Architecture)와 메모리/버퍼 최적화 기법을 직접 구현하는 것을 목표로 했습니다.

## 1. Network Engine과 Game Content Logic의 완벽한 분리 (Decoupling)

설계 의도

네트워크 엔진(TcpServer)은 패킷의 의미나 게임 규칙을 전혀 모르게 만들고, 게임 로직(GameServer)은 가능한 한 Session ID / Player ID 중심으로 상태를 관리하도록 설계했습니다.

구현 방식

인터페이스 기반 통신: 네트워크 계층은 INetworkHandler 인터페이스만 알고 있으며, 접속/종료/패킷 수신 시 OnConnection, OnDisconnection, OnRecv 콜백만 호출합니다.

추상화된 데이터 전달: 네트워크 계층은 "바이트 스트림(char*)을 Session에게서 받았다"는 사실만 전달하며, 그 바이트가 어떤 패킷 타입인지, 어떤 게임 명령인지는 전혀 관여하지 않습니다.

로직의 독립성: GameServer는 Session.id를 키로 _players 맵을 관리하며, 실제 게임 로직(스폰, 이동, 공격, 피격, 브로드캐스트)을 전담합니다.

결과

이를 통해, 네트워크 계층을 IOCP나 다른 모델로 교체하더라도 INetworkHandler 구현만 맞추면 게임 로직은 그대로 재사용될 수 있는 유연한 구조를 확보했습니다.

## 2. Zero-Copy를 지향하는 버퍼 관리

문제 인식

일반적인 소켓 프로그래밍 패턴(recv() → 임시 버퍼 → memcpy → 링버퍼 → 패킷 조립)은 패킷 파싱 전에 불필요한 메모리 복사(memcpy)를 여러 번 유발합니다.

해결: Direct Access RingBuffer

CRingBuffer에 링버퍼 내부 메모리에 직접 접근할 수 있는 인터페이스를 설계했습니다.

char* GetRearBufferPtr(): 쓰기 위치 포인터 반환

int DirectEnqueueSize(): 끊기지 않고 쓸 수 있는 연속된 공간 크기 반환

void MoveRear(int size): 포인터 이동

구현 코드 예시:

// 임시 버퍼 없이 링버퍼 메모리에 직접 recv 수행
int directSize = s.recvBuffer.DirectEnqueueSize();
int recvBytes = recv(s.socket, s.recvBuffer.GetRearBufferPtr(), directSize, 0);
if (recvBytes > 0) s.recvBuffer.MoveRear(recvBytes);


결과

커널에서 사용자 영역으로 넘어온 데이터를 임시 버퍼 없이 바로 링버퍼에 꽂아넣는 구조를 만들어, 메모리 복사 비용을 획기적으로 줄였습니다. 송신(send) 또한 GetFrontBufferPtr()를 통해 링버퍼에서 직접 데이터를 가져갑니다.

## 3. 직렬화 버퍼 (Serialization Buffer) 도입

초기 접근 및 문제점

초기에는 C-Style 구조체 캐스팅(reinterpret_cast)을 사용했으나, 컴파일러/플랫폼 간 패딩(Padding) 문제와 구조체 변경 시 유연성이 떨어지는 문제를 겪었습니다.

개선: PacketBuffer

std::vector<char> 기반의 직렬화 버퍼 클래스를 직접 구현했습니다.

operator<< / operator>>를 오버로딩하여, Stream 기반의 안전한 직렬화를 지원합니다.

// 직렬화 (Serialize)
PacketBuffer buf;
buf << (uint8_t)direction << (int16_t)x << (int16_t)y;

// 역직렬화 (Deserialize)
uint8_t dir;
int16_t x, y;
buf >> dir >> x >> y;


결과

구조체 패킹 문제에서 자유로워졌으며, 이기종 클라이언트와의 통신 안정성을 확보했습니다. 또한, memcpy 사용을 배제하여 버퍼 오버플로우 등의 메모리 오류를 예방했습니다.

## 4. 패킷 팩토리(Packet Factory)와 자동화된 핸들링

Data Flow

Raw Data 수신 → PacketHeader 파싱 → PacketFactory::CreatePacket → Packet::Decode → Packet::Handle

특징

다형성 활용: 모든 패킷은 Packet 추상 클래스를 상속받으며, Handle 메서드 안에서만 실제 게임 로직에 접근합니다.

스위치문 제거: GameServer.cpp에 거대한 switch-case 문이 생기는 것을 방지하고, 객체지향적 다형성을 활용해 패킷 처리 로직을 각 패킷 클래스로 캡슐화했습니다.

결과

새로운 패킷 타입을 추가할 때 enum 정의, Packet 클래스 구현, Factory 등록만으로 확장이 가능해졌으며, 네트워크 코드와 로직 코드가 섞이지 않는 깔끔한 구조를 유지했습니다.

🏗️ System Architecture

Layered Architecture

Network Layer: Winsock + select 기반 소켓 I/O. 각 클라이언트는 Session 객체(Socket + RingBuffer)로 관리됩니다.

Interface Layer: TcpServer가 CRingBuffer를 사용해 패킷 경계를 식별하고, 완성된 패킷 단위만 INetworkHandler로 전달합니다.

Protocol Layer: PacketHeader, PacketType, PacketBuffer로 프로토콜을 정의하고, PacketFactory가 Raw Data를 C++ 객체로 역직렬화합니다.

Logic Layer: GameServer가 플레이어 관리 및 게임 로직(이동, 전투)을 수행하고, 결과를 다시 패킷으로 직렬화하여 브로드캐스트합니다.

Runtime Flow

Initialization: main.cpp에서 Logger 스레드와 서버 객체를 초기화하고, bind/listen을 수행합니다.

Game Loop:

while (true) {
    server.Tick();      // 네트워크 I/O (select, accept, recv, send)
    gameLogic.Update(); // 게임 로직 (이동 등)
    // 20ms 프레임 동기화
}


Connection Life-cycle: AcceptProc -> OnConnection -> DisconnectSession (지연 삭제 예약) -> CleanupDeadSessions (실제 해제)

# 🛠️ Technical Challenges & Troubleshooting

## 1. Select 모델의 Busy Waiting (CPU 100% 점유) 문제

문제: 초기 구현 시 모든 세션 소켓을 무조건 writeSet에 넣고 select()를 호출했습니다. 대부분의 소켓은 항상 "쓰기 가능" 상태이므로 select가 즉시 리턴되어 CPU 100% Busy Waiting 현상이 발생했습니다.

해결: 송신 버퍼(SendBuffer)에 실제로 보낼 데이터가 존재하는 세션만 writeSet에 등록하도록 로직을 변경하여 불필요한 호출을 줄이고 CPU 사용량을 최적화했습니다.

## 2. 표준 출력(std::cout)에 의한 병목 현상

문제: select 기반 싱글 스레드 서버는 메인 루프가 매우 빠르게 돌아야 하는데, 디버깅을 위해 사용한 std::cout의 블로킹으로 인해 네트워크 처리 속도가 급격히 저하되었습니다.

해결: 비동기 로거(Async Logger)를 구현했습니다. 메인 스레드는 로그 메시지를 Thread-safe Queue에 Push만 하고 즉시 리턴하며, 별도의 로거 스레드가 condition_variable로 깨어나 쌓인 로그를 한 번에 출력합니다. 이를 통해 로깅 부하를 네트워크 루프에서 완전히 분리했습니다.

## 3. 세션 종료 시 Dangling Iterator 문제

문제: 게임 로직 루프(for (auto& s : sessions)) 도중 접속 종료를 감지하고 바로 erase를 호출하면 이터레이터가 무효화되어 크래시가 발생했습니다.

해결: 지연 삭제(Lazy Disconnection) 패턴을 적용했습니다. 즉시 삭제하는 대신 Session::isDead = true 플래그만 세팅하고, 프레임의 가장 마지막 단계인 CleanupDeadSessions에서 일괄적으로 실제 소켓 종료 및 메모리 해제를 수행하여 안전성을 확보했습니다.

📂 Source Code Structure

Core Network

Server.cpp: select 기반 I/O 멀티플렉싱 엔진 (TcpServer)

Session.h: 소켓 + 송수신 CRingBuffer를 관리하는 세션 객체

CRingBuffer.cpp: Direct Access가 가능한 고성능 링버퍼 구현

Protocol & Utils

PacketBuffer.cpp: 안전한 직렬화/역직렬화 헬퍼

Packet.cpp: 각 패킷 클래스 정의 및 PacketFactory

Logger.cpp: 스레드 안전 비동기 로거

NetConfig.h: 서버 설정 및 게임 상수

Game Content

GameServer.cpp: 플레이어 관리, 이동/전투/피격 로직, 패킷 핸들링

INetworkHandler.h: 네트워크 엔진과 로직을 연결하는 인터페이스

🚀 How to Build & Run

Requirements: Windows, Visual Studio 2019 이상 (C++17 지원)

Open Server.sln.

Set Configuration to Release / x64.

Build Solution (Ctrl + Shift + B).

Run Server.exe.
