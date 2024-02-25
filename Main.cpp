#include "DiskBenchmark.h"
#include "CLI11/CLI.hpp"

using namespace std;

int main(int argc, char **argv)
{
	const auto calculateMBPerSec = [](unsigned long long totalBytes, unsigned long long msDuration)-> double
	{
		return (round(((static_cast<double>(totalBytes) / (1024.0 * 1024.0)) / (static_cast<double>(msDuration) / 1000.0)) * 10.0) / 10.0);
	};
	CLI::Option *optSeconds, *optIOType, *optRandom, *optThreadNumber, *optTaskNumber, *optUnalignedOffsets,
				*optFileName, *optFileSize, *optBlockSize, *optShowLog, *optReadPercentage;
	CLI::App app("DiskBenchmark");
	DiskBenchmark diskBenchmark;
	int seconds, threadNumber, taskNumber, readPercentage;
	DiskBenchmark::ThreadInfoList threadInfoList;
	unsigned long long totalBytesRead, totalBytesWrite, msDuration;
	long long fileSize, blockSize;
	DiskBenchmark::IOType ioType;
	string fileName, ioTypeParam;

	optSeconds = app.add_option("-s,--seconds", seconds, "Duration of test in seconds (optional)");
	optIOType = app.add_option("-i,--io_type", ioTypeParam, "I/O test type (r -> read, w -> write, rw -> read/write)");
	optReadPercentage = app.add_option("-p,--read_percentage", readPercentage, "Percentage of read blocks for read/write test");
	optRandom = app.add_flag("-r,--random", "Random read/write");
	optThreadNumber = app.add_option("-t,--thread", threadNumber, "Number of thread to use for the test");
	optTaskNumber = app.add_option("-o,--task", taskNumber, "Number of I/O operation per thread");
	optUnalignedOffsets = app.add_flag("-u,--unaligned", "Different starting offsets for each thread");
	optFileName = app.add_option("-n,--file_name", fileName, "Name of the file to use for test");
	optFileSize = app.add_option("-z,--file_size", fileSize, "Size of the file to use for test (in Mb)");
	optBlockSize = app.add_option("-b,--block_size", blockSize, "Size of the block to read/write (in Kb)");
	optShowLog = app.add_flag("-l,--log", "Show log messages");
	CLI11_PARSE(app, argc, argv);
	
	if(optIOType->count() == 0
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
	if(optShowLog->count() > 0) diskBenchmark.setLogMsgFunction([](const string& logMsg) { cout << logMsg << endl; });
	if(optThreadNumber->count() == 0) threadNumber = 1;
	if(optTaskNumber->count() == 0) taskNumber = 1;
	if(optSeconds->count() > 0 && seconds > 0)
	{
		diskBenchmark.setSecondsDuration(seconds);
	}
	if(optReadPercentage->count() > 0)
	{
		if(readPercentage < 0 || readPercentage > 100)
		{
			cerr << "Incorrect read percentage value" << endl;
			return 1;
		}
		diskBenchmark.setReadPercentage(static_cast<unsigned char>(readPercentage));
	}
	diskBenchmark.setRandomAccess((optRandom->count() > 0) ? true : false);
	diskBenchmark.setUnalignedOffsets((optUnalignedOffsets->count() > 0) ? true : false);
	fileSize *= (1024 * 1024);
	blockSize *= 1024;

	cout << "Start benchmark..." << endl << endl;
	totalBytesRead = totalBytesWrite = msDuration = 0;
	threadInfoList = diskBenchmark.executeTest(ioType, threadNumber, taskNumber, fileName, fileSize, blockSize);
	if(threadInfoList.size() > 0)
	{
		int threadCount = 1;

		for(const auto &threadInfo : threadInfoList)
		{
			cout << "Thread " << threadCount++ << endl;
			if(threadInfo.totalReadOperations == 0 && threadInfo.totalWriteOperations == 0)
			{
				cerr << "  Thread error occurred" << endl;
				continue;
			}
			if(threadInfo.totalReadOperations > 0)
			{
				cout << "  Read ops: " << threadInfo.totalReadOperations << " (" << ((threadInfo.totalReadOperations * blockSize) / 1024) << "KB)" << endl;
				totalBytesRead += (threadInfo.totalReadOperations * blockSize);
			}
			if(threadInfo.totalWriteOperations > 0)
			{
				cout << "  Write ops: " << threadInfo.totalWriteOperations << " (" << ((threadInfo.totalWriteOperations * blockSize) / 1024) << "KB)" << endl;
				totalBytesWrite += (threadInfo.totalWriteOperations * blockSize);
			}
			if(threadInfo.msDuration > msDuration) msDuration = threadInfo.msDuration;
		}
	}
	cout << endl << "Total test duration (ms): " << msDuration << endl;
	if(totalBytesRead > 0) cout << "Read MB/s " << fixed << setprecision(1) << calculateMBPerSec(totalBytesRead, msDuration) << endl;
	if(totalBytesWrite > 0) cout << "Write MB/s " << fixed << setprecision(1) << calculateMBPerSec(totalBytesWrite, msDuration) << endl;

	return 0;
}
