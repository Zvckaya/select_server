#pragma once
#include<vector>

class PacketBuffer
{
public:
	PacketBuffer();
	PacketBuffer(int size);
	PacketBuffer(const PacketBuffer& other);
	~PacketBuffer();

	PacketBuffer& operator=(const PacketBuffer& other);
	void Clear();
	int GetBufferSize() const;
	int GetDataSize() const;
	char* GetBufferPtr();
	int GetReadPos() const;
	int GetRemainingReadSize() const;
	int MoveWritePos(int size);
	int MoveReadPos(int size);
	int PutData(const char* src, int size);
	int GetData(char* dest, int size);

	//체이닝을 위해서 & return 
	PacketBuffer& operator<<(bool value) { PutData((char*)&value, sizeof(value)); return *this; }
	PacketBuffer& operator<<(int8_t value) { PutData((char*)&value, sizeof(value)); return *this; }
	PacketBuffer& operator<<(uint8_t value) { PutData((char*)&value, sizeof(value)); return *this; }
	PacketBuffer& operator<<(int16_t value) { PutData((char*)&value, sizeof(value)); return *this; }
	PacketBuffer& operator<<(uint16_t value) { PutData((char*)&value, sizeof(value)); return *this; }
	PacketBuffer& operator<<(int32_t value) { PutData((char*)&value, sizeof(value)); return *this; }
	PacketBuffer& operator<<(uint32_t value) { PutData((char*)&value, sizeof(value)); return *this; }
	PacketBuffer& operator<<(int64_t value) { PutData((char*)&value, sizeof(value)); return *this; }
	PacketBuffer& operator<<(uint64_t value) { PutData((char*)&value, sizeof(value)); return *this; }
	PacketBuffer& operator<<(float value) { PutData((char*)&value, sizeof(value)); return *this; }
	PacketBuffer& operator<<(double value) { PutData((char*)&value, sizeof(value)); return *this; }

	PacketBuffer& operator>>(bool& value) { GetData((char*)&value, sizeof(value)); return *this; }
	PacketBuffer& operator>>(int8_t& value) { GetData((char*)&value, sizeof(value)); return *this; }
	PacketBuffer& operator>>(uint8_t& value) { GetData((char*)&value, sizeof(value)); return *this; }
	PacketBuffer& operator>>(int16_t& value) { GetData((char*)&value, sizeof(value)); return *this; }
	PacketBuffer& operator>>(uint16_t& value) { GetData((char*)&value, sizeof(value)); return *this; }
	PacketBuffer& operator>>(int32_t& value) { GetData((char*)&value, sizeof(value)); return *this; }
	PacketBuffer& operator>>(uint32_t& value) { GetData((char*)&value, sizeof(value)); return *this; }
	PacketBuffer& operator>>(int64_t& value) { GetData((char*)&value, sizeof(value)); return *this; }
	PacketBuffer& operator>>(uint64_t& value) { GetData((char*)&value, sizeof(value)); return *this; }
	PacketBuffer& operator>>(float& value) { GetData((char*)&value, sizeof(value)); return *this; }
	PacketBuffer& operator>>(double& value) { GetData((char*)&value, sizeof(value)); return *this; }


private:
	std::vector<char> _buffer;
	int _readPos;
	int _writePos;
};

