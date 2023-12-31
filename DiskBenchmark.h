#pragma once

#include <map>
#include <vector>
#include <future>
#include "SystemFile.h"

class DiskBenchmark
{
	struct OffsetData
	{
		unsigned long long address = 0;
		bool read = true;
	};

public:
	DiskBenchmark();
	~DiskBenchmark();

	enum class IOType
	{
		Read = 0,
		Write,
		ReadWrite
	};
	struct ThreadInfo
	{
		unsigned long long totalBytesReadWrite = 0;
		unsigned int totalReadOperations = 0;
		unsigned int totalWriteOperations = 0;
	};

	std::vector<ThreadInfo> executeTest(unsigned int seconds, IOType ioType, bool randomAccess, unsigned int threadNumber, unsigned int taskNumber, const std::string &fileName, unsigned long long fileSize, unsigned long long blockSize);

private:
	std::unique_ptr<SystemFile> m_systemFile;

	void executeTasks(std::promise<ThreadInfo> promise, unsigned int seconds, unsigned int taskNumber, unsigned long long blockSize, unsigned int startOffsetindex, const std::vector<OffsetData> &offsets);
	std::vector<OffsetData> calculateOffsets(unsigned long long fileSize, unsigned long long blockSize, IOType ioType, bool randomAccess) const;
	void fillBlock(unsigned char *block, unsigned long long size) const;
	bool checkBlock(unsigned char *block, unsigned long long size) const;
	unsigned int crc32(unsigned char *buffer, unsigned long long size) const;
};