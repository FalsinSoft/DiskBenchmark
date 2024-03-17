#pragma once

#include <map>
#include <vector>
#include <future>

class SystemFile;

class DiskBenchmark
{
	struct OffsetData
	{
		unsigned long long address = 0;
		bool read = true;
	};
	using OffsetDataList = std::vector<OffsetData>;

public:
	DiskBenchmark();
	~DiskBenchmark();

	using LogMsgFunction = std::function<void(const std::string &logMsg)>;

	enum class IOType
	{
		Read = 0,
		Write,
		ReadWrite
	};
	struct ThreadInfo
	{
		unsigned long long msDuration = 0;
		unsigned int totalReadOperations = 0;
		unsigned int totalWriteOperations = 0;
	};
	using ThreadInfoList = std::vector<ThreadInfo>;

	ThreadInfoList executeTest(IOType ioType, unsigned int threadNumber, unsigned int taskNumber, const std::string &fileName, unsigned long long fileSize, unsigned long long blockSize);
	void setLogMsgFunction(const LogMsgFunction &logMsgFunction);
	void setUnalignedOffsets(bool unalignedOffsets);
	void setRandomAccess(bool randomAccess);
	void setReadPercentage(unsigned char readPercentage);
	void setWritePercentage(unsigned char writePercentage);
	void setSecondsDuration(unsigned int seconds);
	void setCrcBlockCheck(bool crcBlock);
	void setUseExistingFile(bool useExistingFile);

private:
	std::unique_ptr<SystemFile> m_systemFile;
	std::exception_ptr m_exception;
	LogMsgFunction m_logMsgFunction;
	bool m_unalignedOffsets, m_randomAccess, m_crcBlock;
	unsigned char m_readPercentage;
	unsigned int m_secondsDuration;
	bool m_useExistingFile;

	ThreadInfo executeTasks(unsigned int taskNumber, unsigned long long blockSize, unsigned int startOffsetIndex, const OffsetDataList& offsets);
	void executeTasksThread(std::promise<ThreadInfo> promise, unsigned int taskNumber, unsigned long long blockSize, unsigned int startOffsetIndex, const OffsetDataList &offsets);
	OffsetDataList calculateOffsets(unsigned long long fileSize, unsigned long long blockSize, IOType ioType, unsigned char readPercentage, bool randomAccess) const;
	void fillBlock(unsigned char *block, unsigned long long size, bool crc) const;
	bool checkCrcBlock(unsigned char *block, unsigned long long size) const;
	unsigned int crc32(unsigned char *buffer, unsigned long long size) const;
};