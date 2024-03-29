#pragma once

#include <functional>
#include <iostream>
#include <Windows.h>

class SystemFile
{
public:
	SystemFile(std::exception_ptr &exception);
	~SystemFile();

	using LogMsgFunction = std::function<void(const std::string &logMsg)>;

	struct FileHandle
	{
		HANDLE handle;
		HANDLE ioCompletionPort;
	};
	using BlockHandle = OVERLAPPED;

	void setLogMsgFunction(const LogMsgFunction &logMsgFunction);

	bool initialize(const std::string &fileName, bool directAccess, unsigned long long fileSize, unsigned char *block, unsigned long long blockSize, bool useExisting = false);
	void close(bool removeFile = true);

	FileHandle openFile(unsigned int taskNumber);
	void closeFile(FileHandle file);
	void writeBlock(FileHandle file, unsigned long long offset, unsigned char *data, unsigned long long size, BlockHandle *block);
	void readBlock(FileHandle file, unsigned long long offset, unsigned char *data, unsigned long long size, BlockHandle *block);
	BlockHandle* getCompletedBlock(FileHandle file);
	unsigned char* allocateAlignedMemory(unsigned long long size);
	void freeAlignedMemory(unsigned char *ptr);
	unsigned int getMemoryPageSize();

private:
	HANDLE m_hFile;
	DWORD m_fileFlags;
	LogMsgFunction m_logMsgFunction;
};