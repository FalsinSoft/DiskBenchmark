#pragma once

#include <map>
#include <iostream>
#include "libaio.h"

class SystemFile
{
public:
	SystemFile(std::exception_ptr &exception);
	~SystemFile();

	struct FileHandle
	{
		int handle;
		io_context_t context;
	};
	using BlockHandle = iocb;

	bool initialize(const std::string &fileName, bool directAccess, unsigned long long fileSize, unsigned char *block, unsigned long long blockSize);
	void close();

	FileHandle openFile(unsigned int taskNumber);
	void closeFile(FileHandle file);
	void writeBlock(FileHandle file, unsigned long long offset, unsigned char *data, unsigned long long size, BlockHandle *block);
	void readBlock(FileHandle file, unsigned long long offset, unsigned char *data, unsigned long long size, BlockHandle *block);
	BlockHandle* getCompletedBlock(FileHandle file);
	unsigned char* allocateAlignedMemory(unsigned long long size);
	void freeAlignedMemory(unsigned char *ptr);
	unsigned int getMemoryPageSize();

private:
	int m_hFile;
	int m_fileFlags;
	std::string m_fileName;
};