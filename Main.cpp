#include "DiskBenchmark.h"
#include "CLI11/CLI.hpp"

using namespace std;

int main(int argc, char **argv)
{
	CLI::Option *optSeconds, *optIOType, *optRandom, *optThreadNumber, *optTaskNumber, *optFileName, *optFileSize, *optBlockSize, *optShowLog;
	CLI::App app("DiskBenchmark");
	DiskBenchmark diskBenchmark;
	int seconds, threadNumber, taskNumber;
	vector<DiskBenchmark::ThreadInfo> threadInfoList;
	unsigned long long totalBytesReadWrite = 0;
	long long fileSize, blockSize;
	DiskBenchmark::IOType ioType;
	string fileName, ioTypeParam;
	bool randomAccess;

	optSeconds = app.add_option("-s,--seconds", seconds, "Duration of test in seconds");
	optIOType = app.add_option("-i,--io_type", ioTypeParam, "I/O test type (r -> read, w -> write, rw -> read/write)");
	optRandom = app.add_flag("-r,--random", "Random read/write");
	optThreadNumber = app.add_option("-t,--thread", threadNumber, "Number of thread to use for the test");
	optTaskNumber = app.add_option("-o,--task", taskNumber, "Number of I/O operation per thread");
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
	fileSize *= (1024 * 1024);
	blockSize *= 1024;

	if(optShowLog->count() > 0) diskBenchmark.setLogMsgFunction([](const string &logMsg) { cout << logMsg << endl; });

	cout << "Start benchmark..." << endl;
	threadInfoList = diskBenchmark.executeTest(seconds, ioType, randomAccess, threadNumber, taskNumber, fileName, fileSize, blockSize);
	if(threadInfoList.size() > 0)
	{
		for(const auto &threadInfo : threadInfoList)
		{
			cout << "Read ops: " << threadInfo.totalReadOperations << " - "
				 << "Write ops: " << threadInfo.totalWriteOperations << " - "
				 << "Total read/write (KB): " << static_cast<double>(threadInfo.totalBytesReadWrite / 1024)
				 << endl;

			if(threadInfo.totalBytesReadWrite > 0)
				totalBytesReadWrite += threadInfo.totalBytesReadWrite;
			else
				cerr << "Thread error occurred" << endl;
		}
	}
	if(totalBytesReadWrite > 0)
		cout << "MB/s " << (static_cast<double>(totalBytesReadWrite / (1024 * 1024)) / static_cast<double>(seconds)) << endl;
	else
		cerr << "Operation failed" << endl;

	return 0;
}
