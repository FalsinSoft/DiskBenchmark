#include "DiskBenchmark.h"
#include "CLI11/CLI.hpp"

using namespace std;

int main(int argc, char **argv)
{
	CLI::Option *optSeconds, *optReadPercentage, *optRandom, *optThreadNumber, *optTaskNumber, *optFileName, *optFileSize, *optBlockSize;
	CLI::App app("DiskBenchmark");
	DiskBenchmark diskBenchmark;
	int seconds, readPercentage, threadNumber, taskNumber;
	vector<DiskBenchmark::ThreadInfo> threadInfoList;
	unsigned long long totalBytesReadWrite = 0;
	long long fileSize, blockSize;
	string fileName;
	bool randomAccess;

	optSeconds = app.add_option("-s,--seconds", seconds, "Duration of test in seconds");
	optReadPercentage = app.add_option("-p,--read_percentage", readPercentage, "Percentage of read operations (remaining operations are write)");
	optRandom = app.add_flag("-r,--random", "Random read/write");
	optThreadNumber = app.add_option("-t,--thread", threadNumber, "Number of thread to use for the test");
	optTaskNumber = app.add_option("-o,--task", taskNumber, "Number of I/O operation per thread");
	optFileName = app.add_option("-n,--file_name", fileName, "Name of the file to use for test");
	optFileSize = app.add_option("-z,--file_size", fileSize, "Size of the file to use for test (in Mb)");
	optBlockSize = app.add_option("-b,--block_size", blockSize, "Size of the block to read/write (in Kb)");
	CLI11_PARSE(app, argc, argv);

	if(optSeconds->count() == 0 || seconds == 0
	|| optReadPercentage->count() == 0 || readPercentage > 100
	|| optThreadNumber->count() == 0 || threadNumber == 0
	|| optTaskNumber->count() == 0 || taskNumber == 0
	|| optFileName->count() == 0
	|| optFileSize->count() == 0 || fileSize == 0
	|| optBlockSize->count() == 0 || blockSize == 0)
	{
		cerr << "Invalid or missing param (use -h for help)" << endl;
		return 1;
	}
	randomAccess = (optRandom->count() > 0) ? true : false;
	fileSize *= (1024 * 1024);
	blockSize *= 1024;

	cout << "Start benchmark..." << endl;
	threadInfoList = diskBenchmark.executeTest(seconds, readPercentage, randomAccess, threadNumber, taskNumber, fileName, fileSize, blockSize);
	if(threadInfoList.size() > 0)
	{
		for(const auto &threadInfo : threadInfoList)
		{
			cout << "Read ops: " << threadInfo.totalReadOperations << " - Write ops: " << threadInfo.totalWriteOperations << endl;

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
