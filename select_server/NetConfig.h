#pragma once

//포트
inline constexpr int PORT = 5000;

//버퍼 사이즈
inline constexpr int MAX_BUFFER = 1024 * 10;
inline constexpr int MAX_SESSIONS = 60;

//게임 상수 
inline constexpr int MOVE_UNIT_X = 3;
inline constexpr int MOVE_UNIT_Y = 2;

//이동가능 범위
inline constexpr int RANGE_MOVE_TOP = 50;
inline constexpr int RANGE_MOVE_LEFT = 10;
inline constexpr int RANGE_MOVE_RIGHT = 630;
inline constexpr int RANGE_MOVE_BOTTOM = 470;

// 공격 범위
inline constexpr int ATTACK1_RANGE_X = 80;
inline constexpr int ATTACK1_RANGE_Y = 10;