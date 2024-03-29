#include <algorithm>
#include <chrono>
#include <random>
#include <thread>
#include "DiskBenchmark.h"
#include "SystemFile.h"

using namespace std;

DiskBenchmark::DiskBenchmark() : m_systemFile(new SystemFile(m_exception)),
								 m_logMsgFunction([](const string &logMsg){}),
								 m_unalignedOffsets(false),
								 m_randomAccess(false),
								 m_readPercentage(50),
								 m_secondsDuration(0),
								 m_crcBlock(false),
								 m_useExistingFile(false)
{
}

DiskBenchmark::~DiskBenchmark()
{
}

void DiskBenchmark::setLogMsgFunction(const LogMsgFunction &logMsgFunction)
{
	m_systemFile->setLogMsgFunction(logMsgFunction);
	m_logMsgFunction = logMsgFunction;
}

void DiskBenchmark::setUnalignedOffsets(bool unalignedOffsets)
{
	m_unalignedOffsets = unalignedOffsets;
}

void DiskBenchmark::setRandomAccess(bool randomAccess)
{
	m_randomAccess = randomAccess;
}

void DiskBenchmark::setReadPercentage(unsigned char readPercentage)
{
	if(readPercentage <= 100) m_readPercentage = readPercentage;
}

void DiskBenchmark::setWritePercentage(unsigned char writePercentage)
{
	if(writePercentage <= 100) m_readPercentage = (100 - writePercentage);
}

void DiskBenchmark::setSecondsDuration(unsigned int seconds)
{
	m_secondsDuration = seconds;
}

void DiskBenchmark::setCrcBlockCheck(bool crcBlock)
{
	m_crcBlock = crcBlock;
}

void DiskBenchmark::setUseExistingFile(bool useExistingFile)
{
	m_useExistingFile = useExistingFile;
}

DiskBenchmark::ThreadInfoList DiskBenchmark::executeTest(IOType ioType, unsigned int threadNumber, unsigned int taskNumber, const std::string &fileName, unsigned long long fileSize, unsigned long long blockSize)
{
	const auto offsets = calculateOffsets(fileSize, blockSize, ioType, m_readPercentage, m_randomAccess);
	const auto pageSize = m_systemFile->getMemoryPageSize();
	ThreadInfoList threadInfoList;
	unsigned char *block;
	bool result;

	if(blockSize == 0 || blockSize % pageSize)
	{
		cerr << "Block size must be " << pageSize << " bytes aligned" << endl;
		return threadInfoList;
	}
	if(threadNumber == 0 || taskNumber == 0)
	{
		cerr << "Invalid thread or task number" << endl;
		return threadInfoList;
	}

	m_logMsgFunction("Initialization...");
	block = new unsigned char[blockSize];
	fillBlock(block, blockSize, m_crcBlock);
	result = m_systemFile->initialize(fileName, true, fileSize, block, blockSize, m_useExistingFile);
	delete[] block;

	if(result == false)
	{
		cerr << "Initialization failed!" << endl;
		return threadInfoList;
	}

	m_logMsgFunction("Start test threads");
	try
	{
		if(threadNumber > 1)
		{
			struct ThreadData
			{
				future<ThreadInfo> status;
				thread instance;
			};
			vector<ThreadData> threads(threadNumber);
			unsigned int startOffsetIndex = 0;

			for(auto& thread : threads)
			{
				promise<ThreadInfo> promise;
				thread.status = promise.get_future();
				thread.instance = std::thread(&DiskBenchmark::executeTasksThread, this, move(promise), taskNumber, blockSize, startOffsetIndex, ref(offsets));
				if(m_unalignedOffsets) startOffsetIndex += (offsets.size() / threads.size());
			}

			while(!threads.empty())
			{
				auto thread = threads.begin();

				while(thread != threads.end())
				{
					if(thread->status.wait_for(std::chrono::milliseconds(0)) == future_status::ready)
					{
						if(thread->instance.joinable()) thread->instance.join();
						threadInfoList.push_back(thread->status.get());
						thread = threads.erase(thread);
					}
					else
					{
						++thread;
					}
				}

				if(m_exception) rethrow_exception(m_exception);
			}
		}
		else
		{
			threadInfoList.push_back(executeTasks(taskNumber, blockSize, 0, offsets));
			if(m_exception) rethrow_exception(m_exception);
		}
	}
	catch(runtime_error &e)
	{
		cerr << "Taks error: " << e.what() << endl;
		threadInfoList.clear();
	}
	
	m_systemFile->close(!m_useExistingFile);

	return threadInfoList;
}

DiskBenchmark::ThreadInfo DiskBenchmark::executeTasks(unsigned int taskNumber, unsigned long long blockSize, unsigned int startOffsetIndex, const OffsetDataList& offsets)
{
	struct TaskData
	{
		enum class State
		{
			Null,
			Read,
			Write
		};
		State state = State::Null;
		SystemFile::BlockHandle block;
		unsigned char *buffer = nullptr;
	};
	chrono::time_point<chrono::steady_clock> startTime;
	SystemFile::BlockHandle *completedBlock;
	int activeTasksCounter, offsetIndex, blocksCounter;
	vector<TaskData> tasks(taskNumber);
	SystemFile::FileHandle file;
	unsigned char *buffer;
	ThreadInfo threadInfo;
	bool running;

	m_logMsgFunction("Execute task thread started");
	buffer = m_systemFile->allocateAlignedMemory(blockSize * taskNumber);
	if(m_crcBlock == false) fillBlock(buffer, blockSize * taskNumber, false);
	for(unsigned int i = 0; i < taskNumber; i++) tasks[i].buffer = &buffer[blockSize * i];
	try
	{
		file = m_systemFile->openFile(taskNumber);

		running = true;
		blocksCounter = 0;
		activeTasksCounter = 0;
		offsetIndex = startOffsetIndex;
		startTime = chrono::steady_clock::now();
		do
		{
			if(running == true && m_secondsDuration > 0)
			{
				running = (chrono::duration_cast<chrono::seconds>(chrono::steady_clock::now() - startTime).count() < m_secondsDuration) ? true : false;
			}

			if(running == true && activeTasksCounter < taskNumber)
			{
				for(auto &task : tasks)
				{
					if(task.state == TaskData::State::Null)
					{
						const auto offset = offsets.at(offsetIndex++);

						task.state = offset.read ? TaskData::State::Read : TaskData::State::Write;
						if(task.state == TaskData::State::Read)
						{
							m_systemFile->readBlock(file, offset.address, task.buffer, blockSize, &task.block);
						}
						else
						{
							if(m_crcBlock) fillBlock(task.buffer, blockSize, true);
							m_systemFile->writeBlock(file, offset.address, task.buffer, blockSize, &task.block);
						}
						activeTasksCounter++;
						if(offsetIndex >= offsets.size()) offsetIndex = 0;
						
						if(m_secondsDuration == 0 && ++blocksCounter >= offsets.size())
						{
							running = false;
							break;
						}
					}
				}
			}
			
			while((completedBlock = m_systemFile->getCompletedBlock(file)) != nullptr)
			{
				for(auto &task : tasks)
				{
					if(&task.block == completedBlock)
					{
						if(m_crcBlock == true && task.state == TaskData::State::Read)
						{
							if(!checkCrcBlock(task.buffer, blockSize)) throw runtime_error("Read block crc failed");
						}

						if(task.state == TaskData::State::Read)
							threadInfo.totalReadOperations++;
						else
							threadInfo.totalWriteOperations++;

						task.state = TaskData::State::Null;
						activeTasksCounter--;
						break;
					}
				}
			}
		} while(running == true || activeTasksCounter > 0);
		threadInfo.msDuration = chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - startTime).count();

		m_systemFile->closeFile(file);
	}
	catch(...)
	{
		threadInfo.totalReadOperations = threadInfo.totalWriteOperations = 0;
		m_exception = current_exception();
	}
	m_systemFile->freeAlignedMemory(buffer);
	m_logMsgFunction("Execute task thread finished");

	return threadInfo;
}

void DiskBenchmark::executeTasksThread(promise<ThreadInfo> promise, unsigned int taskNumber, unsigned long long blockSize, unsigned int startOffsetIndex, const OffsetDataList& offsets)
{
	promise.set_value(executeTasks(taskNumber, blockSize, startOffsetIndex, offsets));
}

DiskBenchmark::OffsetDataList DiskBenchmark::calculateOffsets(unsigned long long fileSize, unsigned long long blockSize, IOType ioType, unsigned char readPercentage, bool randomAccess) const
{
	const unsigned long long maxBlocksNumber = (fileSize / blockSize);
	const unsigned long long maxReadBlocksNumber = ((maxBlocksNumber * readPercentage) / 100);
	OffsetDataList offsets;
	random_device randomDev;
	default_random_engine randomEngine(randomDev());

	for(unsigned long long i = 0; i < maxBlocksNumber; i++)
	{
		OffsetData offset;

		offset.address = (blockSize * i);
		switch(ioType)
		{
			case IOType::Read:
				offset.read = true;
				break;
			case IOType::Write:
				offset.read = false;
				break;
			case IOType::ReadWrite:
				offset.read = (i < maxReadBlocksNumber) ? true : false;
				break;
		}
		offsets.push_back(offset);
	}
	if(randomAccess)
	{
		shuffle(offsets.begin(), offsets.end(), randomEngine);
	}

	return offsets;
}

void DiskBenchmark::fillBlock(unsigned char *block, unsigned long long size, bool crc) const
{
	if(crc == true && size > 4)
	{
		generate_n(block, size - 4, rand);
		*(reinterpret_cast<unsigned int*>(&block[size - 4])) = crc32(block, size - 4);
	}
	else
	{
		generate_n(block, size, rand);
	}
}

bool DiskBenchmark::checkCrcBlock(unsigned char *block, unsigned long long size) const
{
	unsigned int crc1, crc2;

	if(size < 4)
	{
		return false;
	}

	crc1 = *(reinterpret_cast<unsigned int*>(&block[size - 4]));
	crc2 = crc32(block, size - 4);

	return (crc1 == crc2) ? true : false;
}

unsigned int DiskBenchmark::crc32(unsigned char *buffer, unsigned long long size) const
{
	const unsigned int table[] = {
		0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
		0xe963a535, 0x9e6495a3,	0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
		0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
		0xf3b97148, 0x84be41de,	0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
		0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec,	0x14015c4f, 0x63066cd9,
		0xfa0f3d63, 0x8d080df5,	0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
		0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,	0x35b5a8fa, 0x42b2986c,
		0xdbbbc9d6, 0xacbcf940,	0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
		0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
		0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
		0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,	0x76dc4190, 0x01db7106,
		0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
		0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
		0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
		0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
		0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
		0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
		0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
		0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
		0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
		0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
		0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
		0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
		0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
		0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
		0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
		0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
		0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
		0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
		0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
		0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
		0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
		0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
		0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
		0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
		0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
		0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
		0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
		0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
		0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
		0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
		0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
		0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
	};
	unsigned int crc = 0xFFFFFFFF;

	for(unsigned long long i = 0; i < size; ++i)
	{
		crc = (table[(crc ^ buffer[i]) & 0xFF] ^ (crc >> 8));
	}

	return (crc ^ 0xFFFFFFFF);
}
