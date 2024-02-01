#include <vector>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include "SystemFile.h"

using namespace std;

SystemFile::SystemFile(exception_ptr &exception) : m_hFile(-1)
{
}

SystemFile::~SystemFile()
{
	close();
}

bool SystemFile::initialize(const string &fileName, bool directAccess, unsigned long long fileSize, unsigned char *block, unsigned long long blockSize)
{
	const auto blockNumber = (fileSize / blockSize);
	int hFile;

	close();

	hFile = open(fileName.c_str(), O_CREAT | O_WRONLY | O_TRUNC, S_IWUSR | S_IRUSR);
	if(hFile == -1)
	{
		cerr << "Unable to create file " << fileName << endl;
		return false;
	}
	for(unsigned long long i = 0; i < blockNumber; i++)
	{
		if(write(hFile, block, blockSize) != blockSize)
		{
			cerr << "Unable to write inside file " << fileName << endl;
			return false;
		}
	}
	fsync(hFile);
	::close(hFile);
	
	m_fileFlags = O_RDWR;
	if(directAccess) m_fileFlags |= O_DIRECT;
	m_hFile = open(fileName.c_str(), m_fileFlags);
	if(m_hFile == -1)
	{
		cerr << "Unable to open file " << fileName << endl;
		return false;
	}

	m_fileName = fileName;

	return true;
}

void SystemFile::close()
{
	if(m_hFile != -1)
	{
		::close(m_hFile);
		remove(m_fileName.c_str());

		m_hFile = -1;
	}
}

SystemFile::FileHandle SystemFile::openFile(unsigned int taskNumber)
{
	FileHandle file;

	file.handle = open(m_fileName.c_str(), m_fileFlags);
	if(file.handle == -1)
	{
		throw runtime_error(string("open() return error ") + strerror(errno));
	}
	memset(&file.context, 0, sizeof(file.context));
	if(io_setup(taskNumber, &file.context) != 0)
	{
		throw runtime_error("io_setup() error");
	}

	return file;
}

void SystemFile::closeFile(FileHandle file)
{
	io_destroy(file.context);
	::close(file.handle);
}

SystemFile::BlockHandle SystemFile::writeBlock(FileHandle file, unsigned long long offset, unsigned char *block, unsigned long long size)
{
	iocb *request = new iocb;

	io_prep_pwrite(request, file.handle, block, size, offset);
	if(io_submit(file.context, 1, &request) != 1)
	{
		throw runtime_error("io_submit() error");
	}

	return request;
}

SystemFile::BlockHandle SystemFile::readBlock(FileHandle file, unsigned long long offset, unsigned char *block, unsigned long long size)
{
	iocb *request = new iocb;

	io_prep_pread(request, file.handle, block, size, offset);
	if(io_submit(file.context, 1, &request) != 1)
	{
		throw runtime_error("io_submit() error");
	}

	return request;
}

SystemFile::BlockHandle SystemFile::getCompletedBlock(FileHandle file)
{
	io_event event;

	memset(&event, 0, sizeof(event));
	io_getevents(file.context, 0, 1, &event, NULL);
	if(event.obj != NULL);
	{
		iocb *request = event.obj;
		if(event.res < 0)
		{
			throw runtime_error("io_getevents() event error");
		}
		delete request;
		return request;
	}

	return nullptr;
}

unsigned char* SystemFile::allocateAlignedMemory(unsigned long long size)
{
	void *ptr = nullptr;
	posix_memalign(&ptr, 512, size);
	return reinterpret_cast<unsigned char*>(ptr);
}

void SystemFile::freeAlignedMemory(unsigned char *ptr)
{
	free(ptr);
}

unsigned int SystemFile::getMemoryPageSize()
{
	return getpagesize();
}