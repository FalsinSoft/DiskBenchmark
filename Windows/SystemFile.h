#pragma once

#include <map>
#include <iostream>
#include <Windows.h>

class SystemFile
{
public:
	SystemFile();
	~SystemFile();

	using FileHandle = HANDLE;
	using BlockHandle = LPOVERLAPPED;

	bool initialize(const std::string &fileName, bool directAccess, unsigned long long fileSize, unsigned char *block, unsigned long long blockSize);
	void close();

	FileHandle openFile();
	void closeFile(FileHandle file);
	BlockHandle writeBlock(FileHandle file, unsigned long long offset, unsigned char *block, unsigned long long size);
	BlockHandle readBlock(FileHandle file, unsigned long long offset, unsigned char *block, unsigned long long size);
	bool checkReadWriteStatus(FileHandle file, BlockHandle block);
	unsigned char* allocateAlignedMemory(unsigned long long size);
	void freeAlignedMemory(unsigned char *ptr);
	unsigned int getMemoryPageSize();

private:
	HANDLE m_hFile;
	DWORD m_fileFlags;
};