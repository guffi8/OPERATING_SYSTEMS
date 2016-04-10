#include "CacheFileSystemLog.h"
#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <errno.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string>
#include <cstring>
#include <fstream>
#include <ctime>
#include <string>
#include <cstring>

using namespace std;

/**
 * construct an object that dealing with the .fileSystem.log file
 */
CacheFileSystemLog::CacheFileSystemLog(char const * rootPath)
{

	char path[PATH_MAX];
	strcpy(path, rootPath);
	strncat(path, FILE_SYSTEM_LOG, PATH_MAX);

	// if the logger file not exist
	if(fileExist(rootPath) != SUCCESS) {
		logFile.open(path);
	} else {
		logFile.open(path, std::ios_base::app);
	}

}

/**
 * the de-structor, close the stream if it was open.
 */
CacheFileSystemLog::~CacheFileSystemLog()
{
	if(logFile.is_open())
	{
		logFile.close();
	}
}

/**
 * getting an file path and return 0 if the file already exist. -1 otherwise.
 */
int CacheFileSystemLog::fileExist(char const * rootPath)
{

	char path[PATH_MAX];
	strcpy(path, rootPath);
	strncat(path, FILE_SYSTEM_LOG, PATH_MAX);

	ifstream isfile(path);
	if(isfile)
	{
		return SUCCESS;
	}

	return FAIL;
}

/**
 * writing to the stream the time stamp (UNIX-TIME) and the given function name.
 */
void CacheFileSystemLog::writeFileSystemFunc(char const * funcName)
{
	time_t res = time(nullptr);
	logFile << res << " " << funcName << endl;
}

/**
 * writing to the stream the LFU information of the file fileName.
 */
void CacheFileSystemLog::writeLFUinfo(string fileName, const int blockNum,
		const int blockTimeInUsed)
{
	logFile << fileName << " " << blockNum << " " << blockTimeInUsed << endl;
}

