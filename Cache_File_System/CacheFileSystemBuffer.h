
#ifndef CACHEFILESYSTEMBUFFER_H_
#define CACHEFILESYSTEMBUFFER_H_

#include <iostream>
#include <vector>
#include <map>
#include <fuse.h>

#define SUCCESS 0
#define FAIL -1

using namespace std;

typedef struct block {
	int blockNum;
	char* data;
	size_t dataSize;
	int accessNum;
} block;

typedef struct block_info {
	string fileName;
	int blockNum;
	int accessNum;
} block_info;

/**
 * Cache buffer is Responsible for communications between
 * the cache blocks and memory
 */
class CacheFileSystemBuffer {

public:

	CacheFileSystemBuffer(int numberOfBlocks, int blockSize);
	~CacheFileSystemBuffer();

	int getDataFormCache(char* fullPath, char* buf, size_t size, off_t offset, struct fuse_file_info *fi);
	void getBlockInfo(vector<block_info*>* info);

	void updateFileNameInCache(char* oldPath, char* NewPath, int isDir);
private:

	unsigned int numberOfBlocks;
	unsigned int blockSize;
	map<string, vector<block*> > nameToBlocks;
	vector<block*> cache;

	int isFileInCache(char* fileName);
	void importFromMemToCache(char* fullPath, int blockNum, struct fuse_file_info *fi);
	void importFromCache(char* fullPath, char* buf, size_t size, off_t offset, int blockNum);
	void insertToCache(block* toInsert, char* fullPath);
	void removeBlockFromCache();
	void freeBlock(block* toFree);
	int actualBlockNums(size_t size, off_t offset, struct fuse_file_info *fi);
};


#endif /* CACHEFILESYSTEMBUFFER_H_ */
