#include "DiskBenchmark.h"
#include "CLI11/CLI.hpp"

using namespace std;

int main(int argc, char **argv)
{
	CLI::Option *optSeconds, *optIOType, *optRandom, *optThreadNumber, *optTaskNumber, *optUnalignedOffsets, *optFileName, *optFileSize, *optBlockSize, *optShowLog;
	CLI::App app("DiskBenchmark");
	DiskBenchmark diskBenchmark;
	int seconds, threadNumber, taskNumber;
	vector<DiskBenchmark::ThreadInfo> threadInfoList;
	unsigned long long totalBytesRead, totalBytesWrite;
	long long fileSize, blockSize;
	DiskBenchmark::IOType ioType;
	string fileName, ioTypeParam;
	bool randomAccess, unalignedOffsets;

	optSeconds = app.add_option("-s,--seconds", seconds, "Duration of test in seconds");
	optIOType = app.add_option("-i,--io_type", ioTypeParam, "I/O test type (r -> read, w -> write, rw -> read/write)");
	optRandom = app.add_flag("-r,--random", "Random read/write");
	optThreadNumber = app.add_option("-t,--thread", threadNumber, "Number of thread to use for the test");
	optTaskNumber = app.add_option("-o,--task", taskNumber, "Number of I/O operation per thread");
	optUnalignedOffsets = app.add_flag("-u,--unaligned", "Different starting offsets for each thread");
	optFileName = app.add_option("-n,--file_name", fileName, "Name of the file to use for test");
	optFileSize = app.add_option("-z,--file_size", fileSize, "Size of the file to use for test (in Mb)");
	optBlockSize = app.add_option("-b,--block_size", blockSize, "Size of the block to read/write (in Kb)");
	optShowLog = app.add_flag("-l,--log", "Show log messages");
	CLI11_PARSE(app, argc, argv);
	
	if(optSeconds->count() == 0 || seconds == 0
	|| optIOType->count() == 0
	|| optThreadNumber->count() == 0 || threadNumber == 0
	|| optTaskNumber->count() == 0 || taskNumber == 0
	|| optFileName->count() == 0
	|| optFileSize->count() == 0 || fileSize == 0
	|| optBlockSize->count() == 0 || blockSize == 0)
	{
		cerr << "Invalid or missing param (use -h for help)" << endl;
		return 1;
	}
	if(ioTypeParam == "r")
		ioType = DiskBenchmark::IOType::Read;
	else if(ioTypeParam == "w")
		ioType = DiskBenchmark::IOType::Write;
	else if(ioTypeParam == "rw")
		ioType = DiskBenchmark::IOType::ReadWrite;
	else
	{
		cerr << "Invalid I/O type test param (use -h for help)" << endl;
		return 1;
	}
	randomAccess = (optRandom->count() > 0) ? true : false;
	unalignedOffsets = (optUnalignedOffsets->count() > 0) ? true : false;
	fileSize *= (1024 * 1024);
	blockSize *= 1024;

	if(optShowLog->count() > 0) diskBenchmark.setLogMsgFunction([](const string &logMsg) { cout << logMsg << endl; });

	cout << "Start benchmark..." << endl;
	totalBytesRead = totalBytesWrite = 0;
	threadInfoList = diskBenchmark.executeTest(seconds, ioType, randomAccess, threadNumber, taskNumber, unalignedOffsets, fileName, fileSize, blockSize);
	if(threadInfoList.size() > 0)
	{
		for(const auto &threadInfo : threadInfoList)
		{
			if(threadInfo.totalReadOperations == 0 && threadInfo.totalWriteOperations == 0)
			{
				cerr << "Thread error occurred" << endl;
				continue;
			}
			if(threadInfo.totalReadOperations > 0)
			{
				cout << "Read ops: " << threadInfo.totalReadOperations << " (" << static_cast<double>((threadInfo.totalReadOperations * blockSize) / 1024) << "KB)" << endl;
				totalBytesRead += (threadInfo.totalReadOperations * blockSize);
			}
			if(threadInfo.totalWriteOperations > 0)
			{
				cout << "Write ops: " << threadInfo.totalWriteOperations << " (" << static_cast<double>((threadInfo.totalWriteOperations * blockSize) / 1024) << "KB)" << endl;
				totalBytesWrite += (threadInfo.totalWriteOperations * blockSize);
			}
		}
	}
	if(totalBytesRead > 0) cout << "Read MB/s " << (static_cast<double>(totalBytesRead / (1024 * 1024)) / static_cast<double>(seconds)) << endl;
	if(totalBytesWrite > 0) cout << "Write MB/s " << (static_cast<double>(totalBytesWrite / (1024 * 1024)) / static_cast<double>(seconds)) << endl;

	return 0;
}
