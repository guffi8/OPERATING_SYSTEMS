#include "CacheFileSystemBuffer.h"

#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <errno.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string>
#include <cstring>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <libgen.h>
#include <unistd.h>
#include <utility>
#include <algorithm>
#include <math.h>

#define SYSTEM_ERROR "System Error\n"

using namespace std;

/**
 * Compare between block's access number
 */
bool accesNumComperator(block* b1, block* b2)
{
	return (b1->accessNum > b2->accessNum);
}

/**
 * The constructor. Construct a new cache object.
 */
CacheFileSystemBuffer::CacheFileSystemBuffer(int numberOfBlocks, int blockSize)
{
	this->numberOfBlocks = numberOfBlocks;
	this->blockSize = blockSize;
}

/**
 * The de-constructor.
 */
CacheFileSystemBuffer::~CacheFileSystemBuffer()
{
	vector<block*>::iterator it;
	for(it = cache.begin(); it != cache.end(); ++it)
	{
		freeBlock(*it);
	}
}

/**
 * Gets a file name and check if the file in the cache
 */
int CacheFileSystemBuffer::isFileInCache(char* fileName)
{

	string strFileName(fileName);
	if(this->nameToBlocks.find(strFileName) != this->nameToBlocks.end())
	{
		return SUCCESS;
	}
	return FAIL;

}

/**
 * free block from the heap
 */
void CacheFileSystemBuffer::freeBlock(block* toFree)
{

	free(toFree->data);
	toFree->data = NULL;

	delete toFree;

}

/**
 * remove the least frequently block from the cache
 */
void CacheFileSystemBuffer::removeBlockFromCache()
{
	//remove the least frequently used block from the cache
	sort(cache.begin(), cache.end(), accesNumComperator);
	block* toRemove = cache.back();
	cache.pop_back();
	//int leastAccessNum = cache.back()->accessNum;

	map<string, vector<block*>>::iterator it;
	vector<block*>::iterator blockIt;
	for(it = nameToBlocks.begin(); it != nameToBlocks.end(); it++)
	{
		for(blockIt = it->second.begin(); blockIt != it->second.end(); blockIt++)
		{
			if((*blockIt) == toRemove)
			{
				it->second.erase(blockIt);
				break;
			}
		}
	}

	freeBlock(toRemove);

}

/**
 * insert new block from the cache. if necessary remove block from the cache
 */
void CacheFileSystemBuffer::insertToCache(block* toInsert, char* fullPath)
{

	//first check if the cache is full
	string strFileName/* = new string*/(fullPath);
	if(cache.size() >= this->numberOfBlocks)
	{
		removeBlockFromCache();
	}

	cache.push_back(toInsert);

	if(nameToBlocks.find(strFileName) == nameToBlocks.end())
	{
		nameToBlocks[strFileName] = vector<block*>();
	}

	nameToBlocks[strFileName].push_back(toInsert);
	//delete strFileName;
	return;

}

/**
 * Create a new block, import it's data from the disc, insert it to the cache.
 */
void CacheFileSystemBuffer::importFromMemToCache(char* fullPath, int blockNum, struct fuse_file_info *fi)
{
	// creating the block
	try {
		block* newBlock = new block();
		newBlock->accessNum = 0;
		newBlock->blockNum = blockNum;
		newBlock->data = (char*)malloc(this->blockSize*sizeof(char));
		if(newBlock->data == NULL)
		{
			cerr << SYSTEM_ERROR;
			exit(1);
		}
		size_t i;
		for(i = 0; i < this->blockSize; i++)
		{
			newBlock->data[i] = '\0';
		}
		int dataSize = pread(fi->fh, newBlock->data, this->blockSize, (blockNum - 1) * this->blockSize);
		newBlock->dataSize = dataSize;

		// insert the new block to the cache
		insertToCache(newBlock, fullPath);
	} catch (bad_alloc& e) {
		cerr << SYSTEM_ERROR;
		exit(1);
	}


}

/**
 * get the relevant data from the cache according to the given offset and size
 */
void CacheFileSystemBuffer::importFromCache(char* fullPath, char* buf, size_t size, off_t offset, int blockNum)
{
	vector<block*>::iterator it;
	block* theBlock;
	string strFileName(fullPath);
	for(it = (nameToBlocks[strFileName]).begin(); it != (nameToBlocks[strFileName]).end(); it++)
	{
		if((*it)->blockNum == blockNum)
		{
			theBlock = (*it);
			break;
		}
	}

	theBlock->accessNum++;
	memcpy(buf, theBlock->data + offset, size);

}

/**
 * Calculate the number of blocks that need to read.
 */
int CacheFileSystemBuffer::actualBlockNums(size_t size, off_t offset ,struct fuse_file_info *fi)
{
	struct stat statbuf;
	fstat(fi->fh, &statbuf);
	int actualSize;
	if(offset > statbuf.st_size)
	{
		return FAIL;
	}

	if(size + offset < (size_t)(statbuf.st_size))
	{
		actualSize = size + offset;
	}
	else
	{
		actualSize = statbuf.st_size;
	}

	int blockCounter = 0;

	int pos = (int) (offset + (this->blockSize - (offset % this->blockSize)));
	while (pos < actualSize)
	{
		pos += this->blockSize;
		blockCounter++;
	}

	return blockCounter;


}

/**
 * fill the buffer with the data requested according to the size and offset. return the size of the read data.
 */
int CacheFileSystemBuffer::getDataFormCache(char* fullPath, char* buf, size_t size, off_t offset, struct fuse_file_info *fi)
{

	int firstBlock = (offset / this->blockSize) + 1;
	int numberOfBlocks = actualBlockNums(size, offset, fi);
	if(numberOfBlocks == FAIL)
	{
		return -1;
	}
	int alreadyBuff = 0;
	int currentSize = 0;
	bool blockInCache;
	string strFileName(fullPath);

	// if the file exist in the cache or part of it.
	if(isFileInCache(fullPath) != FAIL)
	{
		vector<block*>::iterator it;
		int i;
		for(i = firstBlock; i <= (firstBlock + numberOfBlocks); i++)
		{
			blockInCache = false;
			if(size + (offset % this->blockSize) > this->blockSize)
			{
				currentSize = this->blockSize - (offset % this->blockSize);
			}
			else
			{
				currentSize = size;
			}
			// if the block i in the cache
			for(it = nameToBlocks[strFileName].begin(); it != nameToBlocks[strFileName].end(); it++)
			{
				if((*it)->blockNum == i)
				{
					importFromCache(fullPath, buf + alreadyBuff, currentSize, (offset % this->blockSize), i);
					offset += currentSize;
					size -= currentSize;
					alreadyBuff += currentSize;
					blockInCache = true;
					break;
				}
			}
			// the block not in the cache
			if(!blockInCache)
			{
				importFromMemToCache(fullPath, i, fi);
				importFromCache(fullPath, buf + alreadyBuff, currentSize, (offset % this->blockSize), i);
				offset += currentSize;
				size -= currentSize;
				alreadyBuff += currentSize;
			}
		}

	}
	else // the file not in the cache
	{
		int i;
		for(i = firstBlock; i <= (firstBlock + numberOfBlocks); i++)
		{
			if(size + (offset % this->blockSize) > this->blockSize)
			{
				currentSize = this->blockSize - (offset % this->blockSize);
			}
			else
			{
				currentSize = size;
			}
			importFromMemToCache(fullPath, i, fi);
			importFromCache(fullPath, buf + alreadyBuff, currentSize, (offset % this->blockSize), i);
			offset += currentSize;
			size -= currentSize;
			alreadyBuff += currentSize;
		}
	}

	return alreadyBuff;
}

/**
 * Run on all the cache and fill the given vector with the info about every block in the cache
 */
void CacheFileSystemBuffer::getBlockInfo(vector<block_info*>* info)
{
	block_info* blockInfo;

	map<string, vector<block*>>::iterator it;
	vector<block*>::iterator blockIt;
	for(it = nameToBlocks.begin(); it != nameToBlocks.end(); it++)
	{
		for(blockIt = it->second.begin(); blockIt != it->second.end(); blockIt++)
		{
			try {
				blockInfo = new block_info();
				blockInfo->fileName = it->first;
				blockInfo->accessNum = (*blockIt)->accessNum;
				blockInfo->blockNum = (*blockIt)->blockNum;
				(*info).push_back(blockInfo);
			} catch (bad_alloc& e) {
				cerr << SYSTEM_ERROR;
				exit(1);
			}

		}
	}


}

/**
 * update the file names in the cache
 */
void CacheFileSystemBuffer::updateFileNameInCache(char* oldPath, char* newPath, int isDir)
{
	// first need to check if the current file has relative blocks in the cache
	string strOldFileName(oldPath);
	string strNewFileName(newPath);

	// the changed path was a directory
	if(isDir == SUCCESS)
	{
		strOldFileName.append("/");
		strNewFileName.append("/");
		int length;
		map<string, vector<block*>>::iterator it;
		for(it = nameToBlocks.begin(); it != nameToBlocks.end();/* it++*/)
		{
			if(it->first.find(strOldFileName) == 0)
			{
				length = strOldFileName.length();
				string newPath = it->first;
				newPath.replace(0, length, strNewFileName);
				nameToBlocks.insert(it, pair<string,vector<block*> >(newPath,nameToBlocks[it->first]));
				it = nameToBlocks.erase(it);
			}
			else
			{
				++it;
			}
		}
	}
	else // the changed path was a file
	{
		if(isFileInCache(oldPath) != FAIL)
		{
			nameToBlocks[strNewFileName] = nameToBlocks[strOldFileName];
			nameToBlocks.erase(strOldFileName);
		}
	}


}
