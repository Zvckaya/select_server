
#include <iostream>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <ws2tcpip.h>
#include <winsock2.h>
#include <windows.h>
#include <vector>
#include <algorithm>
#include <string>

#pragma comment(lib, "ws2_32.lib")

const int PORT = 3000;
const int PACKET_SIZE = 16;
const int MAX_SESSIONS = 60; 

// --- 패킷 구조체 (클라이언트와 동일) ---
#pragma pack(push, 1)
struct PacketHeader {
    int type;
    int id;
};

struct PacketIDAssign { // Type 0
    int type; int id; int unused1; int unused2;
};

struct PacketStarCreate { // Type 1
    int type; int id; int x; int y;
};

struct PacketStarDelete { // Type 2
    int type; int id; int unused1; int unused2;
};

struct PacketMove { // Type 3
    int type; int id; int x; int y;
};
#pragma pack(pop)

// --- 세션 구조체 (Session + Player 통합) ---
struct Session
{
    SOCKET socket;
    int id;
    int x;
    int y;

    // 수신 버퍼 (TCP 스트림 처리용)
    char recvBuffer[4096];
    int recvBytes = 0;

    // 송신 버퍼 (간단한 구현을 위해 생략, 실제로는 필요)

    // 지연 삭제를 위한 플래그
    bool isDead = false;
};

// --- 전역 변수 ---
SOCKET g_listenSocket;
std::vector<Session*> g_sessions;
int g_idCounter = 1000; // ID 할당 기준점

// --- 함수 선언 ---
void InitializeNetwork();
void NetworkProc();
void AcceptProc();
void RecvProc(Session* session);
void PacketProc(Session* session, char* packetData);
void DisconnectSession(Session* session);
void CleanupDeadSessions(); // 지연 삭제 처리
void SendUnicast(Session* session, void* data, int size);
void SendBroadcast(Session* excludeSession, void* data, int size);
void Monitor();
void Cleanup();

// --- 메인 함수 ---
int main()
{
    InitializeNetwork();
    std::cout << "=== 별 서버(Select Model) 시작 (Port: " << PORT << ") ===" << std::endl;

    while (true)
    {
        NetworkProc();

        CleanupDeadSessions();

        Monitor();

        Sleep(16);
    }

    Cleanup();
    return 0;
}

// --- 네트워크 초기화 ---
void InitializeNetwork()
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) exit(1);

    g_listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (g_listenSocket == INVALID_SOCKET) exit(1);

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(g_listenSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
    {
        std::cerr << "Bind Error: " << WSAGetLastError() << std::endl;
        exit(1);
    }

    if (listen(g_listenSocket, SOMAXCONN) == SOCKET_ERROR)
    {
        std::cerr << "Listen Error: " << WSAGetLastError() << std::endl;
        exit(1);
    }

    // 리슨 소켓 논블로킹 설정W
    u_long mode = 1;
    ioctlsocket(g_listenSocket, FIONBIO, &mode);
}

// --- 네트워크 처리 (Select) ---
void NetworkProc()
{
    fd_set readSet;
    FD_ZERO(&readSet);

    // 1. 리슨 소켓 등록
    FD_SET(g_listenSocket, &readSet);

    // 2. 접속된 세션 소켓 등록
    for (Session* s : g_sessions)
    {
        if (!s->isDead) // 죽은 세션은 select에서 제외
            FD_SET(s->socket, &readSet);
    }

    // 3. Select (타임아웃 0: 폴링 방식, 게임 서버 특성상 대기 없이 진행)
    timeval timeout = { 0, 0 };
    int selectResult = select(0, &readSet, nullptr, nullptr, &timeout);

    if (selectResult == SOCKET_ERROR) return;

    // 4. Accept 처리
    if (FD_ISSET(g_listenSocket, &readSet))
    {
        AcceptProc();
    }

    // 5. Recv 처리
    for (Session* s : g_sessions)
    {
        if (!s->isDead && FD_ISSET(s->socket, &readSet))
        {
            RecvProc(s);
        }
    }
}

// --- 신규 접속 처리 (가장 중요: 동기화) ---
void AcceptProc()
{
    sockaddr_in clientAddr;
    int addrLen = sizeof(clientAddr);
    SOCKET clientSocket = accept(g_listenSocket, (SOCKADDR*)&clientAddr, &addrLen);

    if (clientSocket == INVALID_SOCKET) return;

    if (g_sessions.size() >= MAX_SESSIONS)
    {
        closesocket(clientSocket); // 접속 제한
        return;
    }

    // 논블로킹 설정
    u_long mode = 1;
    ioctlsocket(clientSocket, FIONBIO, &mode);

    // 세션 생성 및 ID 할당
    Session* newSession = new Session();
    newSession->socket = clientSocket;
    newSession->id = g_idCounter++;
    newSession->x = 0; // 초기 위치
    newSession->y = 0;

    std::cout << "[접속] ID: " << newSession->id << " (" << g_sessions.size() + 1 << "명)" << std::endl;

    // 1. 신규 유저에게 ID 할당 패킷 전송 (Unicast)
    PacketIDAssign idPacket = { (int)0, newSession->id, 0, 0 };
    SendUnicast(newSession, &idPacket, sizeof(idPacket));

    // 2. 신규 유저 정보를 기존 유저들에게 알림 (Broadcast) -> 별 생성(1)
    PacketStarCreate createMyStar = { (int)1, newSession->id, newSession->x, newSession->y };
    SendBroadcast(nullptr, &createMyStar, sizeof(createMyStar)); // 모두에게(자신 포함해도 됨, 클라 로직에 따름)

    // 3. 기존 유저들의 정보를 신규 유저에게 알림 (Unicast loop) -> 별 생성(1)
    for (Session* s : g_sessions)
    {
        if (!s->isDead)
        {
            PacketStarCreate createOtherStar = { (int)1, s->id, s->x, s->y };
            SendUnicast(newSession, &createOtherStar, sizeof(createOtherStar));
        }
    }

    // 리스트에 추가
    g_sessions.push_back(newSession);
}

// --- 데이터 수신 처리 ---
void RecvProc(Session* s)
{
    // 링버퍼 개념을 단순화한 선형 버퍼 + memmove 방식
    int bytesRecv = recv(s->socket, s->recvBuffer + s->recvBytes, sizeof(s->recvBuffer) - s->recvBytes, 0);

    if (bytesRecv == 0 || bytesRecv == SOCKET_ERROR)
    {
        // 연결 종료 또는 에러
        if (bytesRecv == SOCKET_ERROR && WSAGetLastError() == WSAEWOULDBLOCK) return;
        
        DisconnectSession(s);
        return;
    }

    s->recvBytes += bytesRecv;

    // 패킷 조립 (16바이트 단위)
    while (s->recvBytes >= PACKET_SIZE)
    {
        PacketProc(s, s->recvBuffer);

        // 처리된 패킷 제거 (앞으로 당기기)
        memmove(s->recvBuffer, s->recvBuffer + PACKET_SIZE, s->recvBytes - PACKET_SIZE);
        s->recvBytes -= PACKET_SIZE;
    }
}

// --- 패킷 로직 처리 ---
void PacketProc(Session* s, char* packetData)
{
    PacketHeader* header = reinterpret_cast<PacketHeader*>(packetData);

    // 서버는 클라이언트로부터 3번(Move) 패킷만 받음 (요구사항 기준)
    if (header->type == 3)
    {
        PacketMove* p = reinterpret_cast<PacketMove*>(packetData);

        // 1. 서버 측 좌표 업데이트 (검증 로직이 들어갈 곳)
        s->x = p->x;
        s->y = p->y;

        // 2. 다른 유저들에게 이동 브로드캐스팅
        // (요구사항: 나 자신에게는 보내지 않음)
        SendBroadcast(s, p, PACKET_SIZE);
    }
}

// --- 연결 종료 요청 (지연 삭제) ---
void DisconnectSession(Session* s)
{
    if (s->isDead) return;

    s->isDead = true; // 플래그 On
    std::cout << "[종료] ID: " << s->id << " 예약됨" << std::endl;

    // 별 삭제 패킷(2) 브로드캐스팅
    PacketStarDelete deletePacket = { (int)2, s->id, 0, 0 };
    SendBroadcast(s, &deletePacket, sizeof(deletePacket));
}

// --- 지연 삭제 수행 (프레임 마지막) ---
void CleanupDeadSessions()
{
    // 람다식을 이용해 isDead가 true인 세션을 찾아 제거
    auto it = std::remove_if(g_sessions.begin(), g_sessions.end(), [](Session* s) {
        if (s->isDead)
        {
            closesocket(s->socket);
            delete s;
            return true;
        }
        return false;
        });

    g_sessions.erase(it, g_sessions.end());
}

// --- 패킷 전송 (Unicast) ---
void SendUnicast(Session* session, void* data, int size)
{
    if (session->isDead) return;

    // 간단한 구현을 위해 send 바로 호출 (실제로는 송신 버퍼에 넣어야 함)
    send(session->socket, (char*)data, size, 0);
}

// --- 패킷 전송 (Broadcast) ---
void SendBroadcast(Session* excludeSession, void* data, int size)
{
    for (Session* s : g_sessions)
    {
        if (s != excludeSession && !s->isDead)
        {
            SendUnicast(s, data, size);
        }
    }
}

// --- 모니터링 ---
void Monitor()
{
    static DWORD lastCheckTime = 0;
    DWORD curTime = GetTickCount();

    if (curTime - lastCheckTime > 1000) // 1초마다 갱신
    {
        lastCheckTime = curTime;

        // 콘솔 타이틀에 정보 표시 (간단한 모니터링)
        char title[256];
        sprintf_s(title, "Star Server - Users: %d | Last ID: %d", (int)g_sessions.size(), g_idCounter);
        SetConsoleTitleA(title);
    }
}

// --- 뒷정리 ---
void Cleanup()
{
    for (Session* s : g_sessions)
    {
        closesocket(s->socket);
        delete s;
    }
    WSACleanup();
}