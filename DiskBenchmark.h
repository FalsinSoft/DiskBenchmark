#pragma once

#include <map>
#include <vector>
#include <future>
#include "SystemFile.h"

class DiskBenchmark
{
public:
	DiskBenchmark();
	~DiskBenchmark();

	struct ThreadInfo
	{
		unsigned long long totalBytesReadWrite = 0;
		unsigned int totalReadOperations = 0;
		unsigned int totalWriteOperations = 0;
	};

	std::vector<ThreadInfo> executeTest(unsigned int seconds, unsigned char readPercentange, bool randomAccess, unsigned int threadNumber, unsigned int taskNumber, const std::string &fileName, unsigned long long fileSize, unsigned long long blockSize);

private:
	std::unique_ptr<SystemFile> m_systemFile;

	void executeTasks(std::promise<ThreadInfo> promise, unsigned int seconds, unsigned char readPercentange, bool randomAccess, unsigned int taskNumber, unsigned long long fileSize, unsigned long long blockSize);
	void fillBlock(unsigned char *block, unsigned long long size) const;
	bool checkBlock(unsigned char *block, unsigned long long size) const;
	unsigned int crc32(unsigned char *buffer, unsigned long long size) const;
};