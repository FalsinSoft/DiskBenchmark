#include <vector>
#include "SystemFile.h"

using namespace std;

SystemFile::SystemFile() : m_hFile(INVALID_HANDLE_VALUE)
{
}

SystemFile::~SystemFile()
{
	close();
}

bool SystemFile::initialize(const string &fileName, bool directAccess, unsigned long long fileSize, unsigned char *block, unsigned long long blockSize)
{
	const auto nameLength = MultiByteToWideChar(CP_ACP, 0, fileName.c_str(), -1, NULL, 0);
	const auto blockNumber = (fileSize / blockSize);
	DWORD bytesWritten;
	wchar_t *name;
	HANDLE hFile;

	close();

	name = new wchar_t[nameLength];
	MultiByteToWideChar(CP_ACP, 0, fileName.c_str(), -1, name, nameLength);

	hFile = CreateFile(name,
					   GENERIC_WRITE,
					   NULL,
					   NULL,
					   CREATE_ALWAYS,
					   FILE_ATTRIBUTE_NORMAL,
					   NULL);
	if(hFile == INVALID_HANDLE_VALUE)
	{
		delete[] name;
		return false;
	}

	for(unsigned long long i = 0; i < blockNumber; i++)
	{
		if(WriteFile(hFile, block, blockSize, &bytesWritten, NULL) == FALSE || bytesWritten != blockSize)
		{
			delete[] name;
			return false;
		}
	}

	CloseHandle(hFile);
	
	m_fileFlags = (FILE_FLAG_POSIX_SEMANTICS | FILE_FLAG_OVERLAPPED);
	if(directAccess) m_fileFlags |= (FILE_FLAG_NO_BUFFERING | FILE_FLAG_WRITE_THROUGH);
	m_hFile = CreateFile(name,
						 GENERIC_READ | GENERIC_WRITE,
						 FILE_SHARE_READ | FILE_SHARE_WRITE,
						 NULL,
						 OPEN_EXISTING,
						 m_fileFlags,
						 NULL);
	if(m_hFile == INVALID_HANDLE_VALUE)
	{
		delete[] name;
		return false;
	}
	
	delete[] name;

	return true;
}

void SystemFile::close()
{
	if(m_hFile != INVALID_HANDLE_VALUE)
	{
		TCHAR filePath[MAX_PATH];

		GetFinalPathNameByHandle(m_hFile, filePath, MAX_PATH, VOLUME_NAME_DOS);
		CloseHandle(m_hFile);
		DeleteFile(filePath);

		m_hFile = INVALID_HANDLE_VALUE;
	}
}

SystemFile::FileHandle SystemFile::openFile()
{
	TCHAR filePath[MAX_PATH];
	FileHandle file;

	if(GetFinalPathNameByHandle(m_hFile, filePath, MAX_PATH, VOLUME_NAME_DOS) == 0)
	{
		throw runtime_error("GetFinalPathNameByHandle() return error " + GetLastError());
	}
	file = CreateFile(filePath,
					  GENERIC_READ | GENERIC_WRITE,
					  FILE_SHARE_READ | FILE_SHARE_WRITE,
					  NULL,
					  OPEN_EXISTING,
					  m_fileFlags,
					  NULL);
	if(file == INVALID_HANDLE_VALUE)
	{
		throw runtime_error("CreateFile() return error " + GetLastError());
	}

	return file;
}

void SystemFile::closeFile(FileHandle file)
{
	CloseHandle(file);
}

SystemFile::BlockHandle SystemFile::writeBlock(FileHandle file, unsigned long long offset, unsigned char *block, unsigned long long size)
{
	LPOVERLAPPED pOvl = new OVERLAPPED;

	ZeroMemory(pOvl, sizeof(OVERLAPPED));
	pOvl->Offset = (offset & 0xFFFFFFFF);
	pOvl->OffsetHigh = (offset >> 32);
	pOvl->hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	if(WriteFile(file, block, size, NULL, pOvl) == FALSE)
	{
		const auto error = GetLastError();

		if(error != ERROR_IO_PENDING)
		{
			throw runtime_error("WriteFile() return error " + error);
		}
	}

	return pOvl;
}

SystemFile::BlockHandle SystemFile::readBlock(FileHandle file, unsigned long long offset, unsigned char *block, unsigned long long size)
{
	LPOVERLAPPED pOvl = new OVERLAPPED;

	ZeroMemory(pOvl, sizeof(OVERLAPPED));
	pOvl->Offset = (offset & 0xFFFFFFFF);
	pOvl->OffsetHigh = (offset >> 32);
	pOvl->hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	if(ReadFile(file, block, size, NULL, pOvl) == FALSE)
	{
		const auto error = GetLastError();

		if(error != ERROR_IO_PENDING)
		{
			throw runtime_error("ReadFile() return error " + error);
		}
	}

	return pOvl;
}

bool SystemFile::checkReadWriteStatus(FileHandle file, BlockHandle block)
{
	DWORD bytesTransferred;
	BOOL result;

	result = GetOverlappedResult(file,
								 block,
								 &bytesTransferred,
								 FALSE);
	if(result == FALSE)
	{
		const auto error = GetLastError();

		if(error == ERROR_IO_INCOMPLETE)
		{
			return false;
		}

		throw runtime_error("GetOverlappedResult() return error " + error);
	}

	ResetEvent(block->hEvent);
	CloseHandle(block->hEvent);
	delete block;

	return true;
}

unsigned char* SystemFile::allocateAlignedMemory(unsigned long long size)
{
	return reinterpret_cast<unsigned char*>(VirtualAlloc(NULL, size, MEM_COMMIT, PAGE_READWRITE));
}

void SystemFile::freeAlignedMemory(unsigned char *ptr)
{
	VirtualFree(ptr, 0, MEM_RELEASE);
}
