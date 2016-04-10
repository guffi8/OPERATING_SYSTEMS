
#ifndef CACHFILESYSTEMLOG_H_
#define CACHFILESYSTEMLOG_H_

#include <iostream>
#include <fstream>

using namespace std;

#define FILE_SYSTEM_LOG "/.filesystem.log"
#define SUCCESS 0
#define FAIL -1


/**
 * The file system logger header, Responsible for documenting the
 * activities of the file system.
 */
class CacheFileSystemLog {

public:
	CacheFileSystemLog(char const * rootPath);
	~CacheFileSystemLog();
	void writeFileSystemFunc(char const * funcName);
	void writeLFUinfo(string fileName, const int blockNum, const int blockTimeInUsed);

private:
	ofstream logFile;
	int fileExist(char const * rootPath);

};


#endif /* CACHFILESYSTEMLOG_H_ */
