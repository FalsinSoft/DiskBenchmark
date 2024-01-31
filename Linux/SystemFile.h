#pragma once

#include <map>
#include <iostream>
#include <fcntl.h>
#include <aio.h>

class SystemFile
{
	typedef aiocb *paiocb;

public:
	SystemFile(std::exception_ptr &exception);
	~SystemFile();

	using FileHandle = int;
	using BlockHandle = paiocb;

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
	int m_hFile;
	int m_fileFlags;
	std::string m_fileName;
};