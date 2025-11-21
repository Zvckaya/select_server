

#  WinSock 기반 Non-Blocking 게임 서버 아키텍처 정리

단일 스레드 + select + Logger Thread로 구성한 미니 서버 엔진

## 1. 개요 (Overview)

이번 프로젝트는 **WinSock + Non-blocking 소켓 + select 기반**으로 구성한
가볍고 빠른 **단일 스레드 TCP 게임 서버**다.

16바이트로 고정된 RawPacket 구조를 사용해
유저의 접속, 이동, 별 생성·삭제 같은 최소한의 게임 이벤트를 처리하며,
main 스레드는 오로지 네트워크 I/O & 서버 로직만 담당하도록 설계했다.

특히, 서버 구조가 select 기반으로 빠르게 돌아가기 때문에
**표준 출력(cout)**이 병목을 일으켰고, 이를 해결하기 위해
**Logger 전용 스레드**를 별도로 운영하도록 구성한 것이 특징이다.

프로젝트를 구성하는 핵심 기술 요소는 다음과 같다:

* WinSock2 기반 TCP 서버
* Non-blocking 소켓 + select를 활용한 단일 스레드 이벤트 처리
* 16-byte RawPacket 기반의 고정 크기 프로토콜
* PacketFactory를 통한 타입 자동 분기
* 세션(Session) 단위의 유저 관리
* CleanupDeadSessions()를 활용한 안전한 지연 삭제
* Logger 전용 스레드를 활용한 표준 출력 병목 해결

전체적인 구조는 IOCP 같은 고성능 서버로 가기 전,
**네트워크 서버의 기본기를 학습하기 위한 최적의 형태**라고 할 수 있다.

---

## 2. 목차 (Table of Contents)

1. 서버 실행 흐름 (main.cpp)
2. Server 구조
3. Session 구조
4. Packet 구조 & 처리 흐름
5. Logger 스레드
6. 지연 삭제(Delayed Delete)

---

## 3. 내용

---

## 3.1 서버 실행 흐름 (main.cpp)

프로그램은 다음 순서로 동작한다:

1. **Logger 스레드 기동**

   * 서버 메인 루프와 분리된 출력 전용 스레드 생성

2. **Server 생성 및 초기화**

   * WinSock 초기화
   * listen 소켓 생성
   * 포트 바인딩 & 리슨
   * 필요 옵션 설정 (논블로킹 등)

3. **메인 루프 실행**

   * 16ms 주기(약 60FPS)로 Tick() 실행
   * Tick 내부에서

     * NetworkProc() (네트워크 이벤트 처리)
     * CleanupDeadSessions() (지연 삭제) 수행

main 스레드는 오로지 select와 간단한 게임 로직만 실행하기 때문에
전체 구조가 단순하고 관리하기 쉽다.

---

## 3.2 Server 구조

Server는 프로젝트의 핵심이며 다음과 같은 역할을 담당한다.

### ✔ 1) WinSock 초기화 & Non-blocking listen 소켓 준비

* WSAStartup
* socket(AF_INET, SOCK_STREAM)
* bind & listen
* ioctlsocket(FIONBIO)로 논블로킹 설정

이 과정을 통해 “아무것도 기다리지 않는”
즉, CPU를 점유하지 않는 입력 감지가 가능해진다.

---

### ✔ 2) select 기반 이벤트 처리

select 이벤트 루프는 다음 단계로 구성된다:

1. **fd_set 초기화**
2. **listenSocket 등록**
3. **모든 살아있는 session 소켓 등록**
4. **select 호출**

select 결과:

* listenSocket readable → **AcceptProc()**
* session readable → **RecvProc()**

이 구조는 단일 스레드이지만
다중 클라이언트의 동시 접속과 데이터를 안정적으로 처리할 수 있다.

---

### ✔ 3) RecvProc(): 버퍼 기반 안전한 패킷 처리

recv는 항상 패킷 단위(16바이트)로 오지 않는다.
따라서 Session 구조체 내부의 recvBuffer에서 누적 처리한다.

* recv → recvBytes 누적
* recvBytes ≥ 16이면 1패킷 처리
* 처리 후 남은 바이트는 memmove로 앞으로 이동
* 단편화(fragmentation) 대응 가능

이는 네트워크 프로그래밍의 핵심 개념이다.

---

### ✔ 4) 패킷 고수준 처리 흐름

패킷 처리는 다음 구조를 따른다:

```
RawPacket16
    ↓
PacketFactory::CreateFromRaw()
    ↓
Packet 객체 생성
    ↓
FromRaw()로 역직렬화
    ↓
Handle()에서 서버 로직 처리
```

이 방식은 switch-case를 최소화하고,
새로운 패킷 추가도 패킷 클래스만 만들면 되는 확장성 있는 구조다.

---

### ✔ 5) Broadcast & SendTo

* RawPacket16 그대로 송출
* exclude 인자를 통해 특정 클라이언트만 제외 가능
* 모든 네트워크 출력은 Logger 기록과 함께 수행

---

## 3.3 Session 구조

Session은 “클라이언트 1명”을 표현하는 최소 단위이며 다음 정보를 갖는다:

* 클라이언트 socket
* 고유 id
* x, y 위치
* recvBuffer(4096)
* recvBytes
* isDead (삭제 플래그)

### 설계 포인트

1. **소켓과 세션의 분리**
2. recvBuffer로 누적 처리 → 패킷 경계 문제 해결
3. isDead 기반으로 삭제 예약 → 즉시 삭제 X

이는 select 기반 서버에서 가장 안정적인 방식이다.

---

## 3.4 Packet 구조 & 처리

### ✔ RawPacket16 (16 bytes)

고정 길이 패킷:

```
type (int)
a (int)
b (int)
c (int)
```

단순하지만 성능이 매우 좋은 구조다.

---

### ✔ Packet 종류

* IDAssign
* StarCreate
* StarDelete
* Move

모두 RawPacket16을 기반으로 직렬화/역직렬화가 가능하며,
PacketFactory에서 자동으로 변환된다.

---

### ✔ Move 핸들링 예시

```cpp
session.x = x;
session.y = y;
server.Broadcast(raw, &session);
```

한 유저가 이동하면,
다른 모든 클라이언트에게 이동 정보를 브로드캐스트한다.

---

## 3.5 Logger 스레드

select 루프는 매우 빠르게 실행되기 때문에
cout 같은 동기 I/O는 심각한 병목을 발생시킨다.

이를 해결하기 위해 Logger는 다음 구성으로 만들어졌다:

* Log(msg) → mutex + queue push
* worker thread → queue pop + cout 출력
* condition_variable 기반 대기/깨우기
* atomic<bool>로 안전한 종료 지원

이 구조는 실제로 대규모 서버에서도 사용되는 안정적인 패턴이다.

---

## 3.6 지연 삭제 (Delayed Delete)

세션 삭제는 즉시 하면 안 된다.
왜냐하면 select 루프나 다른 함수에서 해당 Session을 참조 중일 수 있기 때문이다.

따라서 이 프로젝트는 다음 흐름을 사용한다:

1. DisconnectSession()
   → isDead = true
2. Tick() → CleanupDeadSessions()
3. remove_if로 dead session 뒤로 이동
4. 소켓 close
5. vector.erase로 실제 삭제

이 과정을 통해

* dangling 포인터 방지
* 재귀적 삭제 문제 해결
* select 루프와 충돌 없음

이라는 안정적인 구조를 달성했다.

---

