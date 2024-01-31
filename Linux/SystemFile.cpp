#include <vector>
#include <unistd.h>
#include <string.h>
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

SystemFile::FileHandle SystemFile::openFile()
{
	FileHandle file;

	file = open(m_fileName.c_str(), m_fileFlags);
	if(file == -1)
	{
		throw runtime_error(string("open() return error ") + strerror(errno));
	}

	return file;
}

void SystemFile::closeFile(FileHandle file)
{
	::close(file);
}

SystemFile::BlockHandle SystemFile::writeBlock(FileHandle file, unsigned long long offset, unsigned char *block, unsigned long long size)
{
	aiocb *aio = new aiocb;

	memset(aio, 0, sizeof(aiocb));
	aio->aio_fildes = file;
	aio->aio_offset = offset;
	aio->aio_buf = block;
	aio->aio_nbytes = size;
	aio->aio_lio_opcode = LIO_NOP;
	aio->aio_sigevent.sigev_notify = SIGEV_NONE;

	if(aio_write(aio) == -1)
	{
		throw runtime_error(string("aio_write() return error ") + strerror(errno));
	}

	return aio;
}

SystemFile::BlockHandle SystemFile::readBlock(FileHandle file, unsigned long long offset, unsigned char *block, unsigned long long size)
{
	aiocb *aio = new aiocb;

	memset(aio, 0, sizeof(aiocb));
	aio->aio_fildes = file;
	aio->aio_offset = offset;
	aio->aio_buf = block;
	aio->aio_nbytes = size;
	aio->aio_lio_opcode = LIO_NOP;
	aio->aio_sigevent.sigev_notify = SIGEV_NONE;

	if(aio_read(aio) == -1)
	{
		throw runtime_error(string("aio_write() return error ") + strerror(errno));
	}

	return aio;
}

bool SystemFile::checkReadWriteStatus(FileHandle file, BlockHandle block)
{
	int result;
	
	result = aio_error(block);
	if(result != 0)
	{
		if(result == EINPROGRESS)
		{
			return false;
		}

		throw runtime_error(string("aio_error() return error ") + strerror(errno));
	}
	if(aio_return(block) < block->aio_nbytes)
	{
		throw runtime_error(string("aio_return() return error ") + strerror(errno));
	}
	delete block;

	return true;
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