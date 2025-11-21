# 1. 개요 (Overview)

이 프로젝트는 WinSock + Non-blocking + select 기반
단일 스레드 TCP 게임 서버

서버는 16바이트 고정 규격의 RawPacket을 사용하여
유저 접속/위치 이동/별 생성·삭제 이벤트를 핸들링
추가적으로 별도 Logger 스레드를 사용하여
표준 출력으로 인해 select 루프가 지연되는 문제 해결

핵심 요소

- WinSock2 기반 TCP 서버

- Non-blocking 소켓 + select 이벤트 처리

- 고정 크기 패킷(16 bytes)

- PacketFactory 기반 자동 패킷 분기

- 세션(Session) 구조체 기반 유저 상태 관리

- 지연 삭제(CleanupDeadSessions) 기반 안전한 세션 종료

- Logger 전용 스레드로 cout 병목 해결

# 2. 목차 (Table of Contents)

1 서버 실행 흐름 (main.cpp)

2 Server 구조

3 Session 구조

4 Packet 구조 & 처리 흐름

5 Logger 스레드

6 지연 삭제(Delayed Delete)

# 3. 내용

3.1 서버 실행 흐름 (main.cpp)

(코드: 

main

)

프로그램 시작 시 Logger::Start()로 로깅 전용 스레드 생성

Server 생성 및 Initialize()로 WinSock·listen 소켓 설정

이후 무한 루프에서 server.Tick()을 16ms 주기로 호출

Tick() 내부에서 NetworkProc() + CleanupDeadSessions() 실행

server loop는 싱글 스레드, 로그만 별도 스레드

3.2 Server 구조

(코드: 

Server

)

Server는 다음 기능을 담당한다:

✔ WinSock 초기화 & Non-blocking listen 소켓

WSAStartup

socket(AF_INET, SOCK_STREAM)

bind, listen

ioctlsocket(FIONBIO)로 논블로킹 처리

✔ select 기반 이벤트 처리

NetworkProc()에서 다음을 수행:

fd_set 초기화

listenSocket 포함

모든 살아있는 session 소켓 추가

select 호출

listenSocket readable → AcceptProc()

session readable → RecvProc()

✔ RecvProc()의 버퍼 처리

(코드: 

Server

)

recv로 받아서 session.recvBuffer에 누적

16바이트(PACKET_SIZE) 이상이면 패킷 1개씩 처리

처리 후 memmove로 남은 데이터 앞으로 이동

패킷 단편화(fragmentation) 안전하게 처리

✔ 패킷 처리 흐름

ProcessPacket():

RawPacket16 → PacketFactory → Packet 객체 생성 → FromRaw → Handle


광범위한 switch-case 대신 확장 가능한 구조.

✔ Broadcast & SendTo

모든 session 순회

exclude 옵션 제공

RawPacket16 단위 그대로 송출

3.3 Session 구조

(코드: 

Session

)

Session은 다음 데이터를 가진다:

socket 핸들

id (서버에서 자동 증가)

위치(x, y)

recvBuffer[4096], recvBytes

isDead (지연 삭제 플래그)

설계 포인트

소켓과 세션 분리

recvBytes + memmove로 버퍼 누적 구조

세션 제거는 즉시 하지 않고 isDead 플래그 후 Cleanup 단계에서 처리

3.4 Packet 구조 & 처리

(코드: 

Packet

)

RawPacket16 (16 bytes)

(코드: 

PacketTypes

)

int type; 
int a;
int b;
int c;


IDAssign, StarCreate, StarDelete, Move 패킷이 존재하며 모두 이 구조를 사용.

PacketFactory

RawPacket16의 type 값만 보고 자동으로 패킷 객체 생성.

각 패킷의 역할

IDAssign: 새 접속 클라이언트에게 고유 ID 전달

StarCreate: 별(캐릭터) 생성

StarDelete: 연결 종료 시 별 삭제

Move: 좌표 이동 브로드캐스트

핸들러 예시(Move):

session.x = x;
session.y = y;
server.Broadcast(raw, &session);

3.5 Logger 스레드

(코드: 

Logger

, 

Logger

)

서버 스레드에서 cout을 직접 호출하면
select 루프가 끊기는 심각한 병목 발생함.

이를 해결하기 위해:

Log(msg) → mutex + queue push

worker thread → queue pop → cout 출력

condition_variable로 대기 및 wake-up

running_ atomic<bool>로 안전한 종료

이 구조는 실제 대규모 서버에서도 사용되는 정석 구조.

3.6 지연 삭제(Delayed Delete)

(코드: 

Server

)

Session 삭제는 즉시 수행하지 않음.

과정:

DisconnectSession → isDead = true

Tick()에서 CleanupDeadSessions 호출

remove_if로 죽은 세션을 뒤로 모음

소켓 close

vector erase로 실제 삭제

장점:

재귀적 호출 위험 없음

dangling 포인터 접근 차단

select 루프와 충돌 없음
