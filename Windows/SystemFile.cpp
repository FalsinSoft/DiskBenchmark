#include <vector>
#include "SystemFile.h"

using namespace std;

SystemFile::SystemFile(exception_ptr &exception) : m_hFile(INVALID_HANDLE_VALUE)
{
}

SystemFile::~SystemFile()
{
	close();
}

bool SystemFile::initialize(const string &fileName, bool directAccess, unsigned long long fileSize, unsigned char *block, unsigned long long blockSize)
{
	const auto blockNumber = (fileSize / blockSize);
	vector<TCHAR> name;
	DWORD bytesWritten;
	HANDLE hFile;

	close();

	name.resize(MultiByteToWideChar(CP_ACP, 0, fileName.c_str(), -1, NULL, 0));
	MultiByteToWideChar(CP_ACP, 0, fileName.c_str(), -1, name.data(), name.size());
	
	hFile = CreateFile(name.data(),
					   GENERIC_WRITE,
					   NULL,
					   NULL,
					   CREATE_ALWAYS,
					   FILE_ATTRIBUTE_NORMAL,
					   NULL);
	if(hFile == INVALID_HANDLE_VALUE)
	{
		return false;
	}
	for(unsigned long long i = 0; i < blockNumber; i++)
	{
		if(WriteFile(hFile, block, static_cast<DWORD>(blockSize), &bytesWritten, NULL) == FALSE || bytesWritten != blockSize)
		{
			return false;
		}
	}
	FlushFileBuffers(hFile);
	CloseHandle(hFile);
	
	m_fileFlags = FILE_FLAG_OVERLAPPED;
	if(directAccess) m_fileFlags |= (FILE_FLAG_NO_BUFFERING | FILE_FLAG_WRITE_THROUGH);
	m_hFile = CreateFile(name.data(),
						 GENERIC_READ | GENERIC_WRITE,
						 FILE_SHARE_READ | FILE_SHARE_WRITE,
						 NULL,
						 OPEN_EXISTING,
						 m_fileFlags,
						 NULL);
	if(m_hFile == INVALID_HANDLE_VALUE)
	{
		DeleteFile(name.data());
		return false;
	}

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

SystemFile::FileHandle SystemFile::openFile(unsigned int taskNumber)
{
	TCHAR filePath[MAX_PATH];
	FileHandle file;

	if(GetFinalPathNameByHandle(m_hFile, filePath, MAX_PATH, VOLUME_NAME_DOS) == 0)
	{
		throw runtime_error("GetFinalPathNameByHandle() return error " + GetLastError());
	}
	file.handle = CreateFile(filePath,
							 GENERIC_READ | GENERIC_WRITE,
							 FILE_SHARE_READ | FILE_SHARE_WRITE,
							 NULL,
							 OPEN_EXISTING,
							 m_fileFlags,
							 NULL);
	if(file.handle == INVALID_HANDLE_VALUE)
	{
		throw runtime_error("CreateFile() return error " + GetLastError());
	}
	file.ioCompletionPort = CreateIoCompletionPort(file.handle, NULL, 0, 0);
	if(file.ioCompletionPort == INVALID_HANDLE_VALUE)
	{
		throw runtime_error("CreateIoCompletionPort() return error " + GetLastError());
	}

	return file;
}

void SystemFile::closeFile(FileHandle file)
{
	CloseHandle(file.ioCompletionPort);
	CloseHandle(file.handle);
}

SystemFile::BlockHandle SystemFile::writeBlock(FileHandle file, unsigned long long offset, unsigned char *block, unsigned long long size)
{
	LPOVERLAPPED pOvl = new OVERLAPPED;

	ZeroMemory(pOvl, sizeof(OVERLAPPED));
	pOvl->Offset = (offset & 0xFFFFFFFF);
	pOvl->OffsetHigh = (offset >> 32);
	if(WriteFile(file.handle, block, static_cast<DWORD>(size), NULL, pOvl) == FALSE)
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
	if(ReadFile(file.handle, block, static_cast<DWORD>(size), NULL, pOvl) == FALSE)
	{
		const auto error = GetLastError();

		if(error != ERROR_IO_PENDING)
		{
			throw runtime_error("ReadFile() return error " + error);
		}
	}

	return pOvl;
}

SystemFile::BlockHandle SystemFile::getCompletedBlock(FileHandle file)
{
	ULONG_PTR completionKey = 0;
	DWORD bytesTransferred;
	LPOVERLAPPED pOvl;
	BOOL result;

	result = GetQueuedCompletionStatus(file.ioCompletionPort,
									   &bytesTransferred,
									   &completionKey,
									   &pOvl,
									   0);
	if(result == TRUE)
	{
		delete pOvl;
		return pOvl;
	}
	else if(pOvl != NULL)
	{
		throw runtime_error("GetQueuedCompletionStatus() block operation failed");
	}

	return nullptr;
}

unsigned char* SystemFile::allocateAlignedMemory(unsigned long long size)
{
	return reinterpret_cast<unsigned char*>(VirtualAlloc(NULL, size, MEM_COMMIT, PAGE_READWRITE));
}

void SystemFile::freeAlignedMemory(unsigned char *ptr)
{
	VirtualFree(ptr, 0, MEM_RELEASE);
}

unsigned int SystemFile::getMemoryPageSize()
{
	SYSTEM_INFO systemInfo;
	GetSystemInfo(&systemInfo);
	return systemInfo.dwPageSize;
}
