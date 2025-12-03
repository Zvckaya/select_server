#include "PacketBuffer.h"
#include <cstring>
#include <algorithm>
#include <cassert>

PacketBuffer::PacketBuffer() :_readPos(0), _writePos(0)
{
	_buffer.reserve(1024);
}

PacketBuffer::~PacketBuffer() 
{
}
PacketBuffer::PacketBuffer(int size) :_readPos(0), _writePos(0)
{
	_buffer.reserve(size);

}
PacketBuffer::PacketBuffer(const PacketBuffer& other)
{
	*this = other;
}


PacketBuffer& PacketBuffer::operator=(const PacketBuffer& other)
{
	if (this != &other)
	{
		_buffer = other._buffer;
		_readPos = other._readPos;
		_writePos = other._writePos;
	}
	return *this;
}

void PacketBuffer::Clear()
{
	_writePos = 0;
	_readPos = 0;
	_buffer.clear();
}

int PacketBuffer::GetBufferSize() const
{
	return static_cast<int>(_buffer.capacity());
}

int PacketBuffer::GetDataSize() const
{
	return _writePos;
}

char* PacketBuffer::GetBufferPtr()
{
	return _buffer.data();
}

int PacketBuffer::GetReadPos() const
{
	return _readPos;
}

int PacketBuffer::GetRemainingReadSize() const
{
	return _writePos - _readPos;
}

int PacketBuffer::MoveWritePos(int size)
{
	if (size <= 0) return _writePos;

	if (_writePos + size > static_cast<int>(_buffer.size()))
	{
		_buffer.resize(_writePos + size);
	}
	_writePos += size;
	return _writePos;

}

int PacketBuffer::MoveReadPos(int size)
{
	if (size <= 0) return _readPos;

	if (_readPos + size > _writePos)
	{
		_readPos = _writePos;
	}
	else {
		_readPos += size;
	}

	return _readPos;
}

int PacketBuffer::PutData(const char* src, int size)
{
	if (size <= 0) return 0;

	if (_writePos + size > static_cast<int>(_buffer.size()))
	{
		_buffer.resize(_writePos + size);
	}
	memcpy(&_buffer[_writePos], src, size);
	_writePos += size;

	return size;
}

int PacketBuffer::GetData(char* dest,int size)
{
	if (size <= 0) return 0;

	if (_readPos + size > _writePos)
	{
		return 0;
	}
	memcpy(dest, &_buffer[_readPos], size);
	_readPos += size;
	return size;
}