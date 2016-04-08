/*
 * Block.h
 *
 *  Created on: Apr 27, 2015
 *      Author: Yuval & Eric
 */

#ifndef _BLOCK_H
#define _BLOCK_H

#include <unistd.h>
#include <iostream>
#include <cstdlib>
#include <pthread.h>
#include <queue>
#include <list>
#include <limits.h>
#include <algorithm>
#include <map>

/**
 * The Block Object
 */
class Block {

public:
	Block(int block_num, Block* father);
	~Block();

	void addData(char* hashedData);
	char* getData();
	int getDepth();

	int getBlockNum();
	void setIsAtteched();
	bool getIsAtteched();
	void setFather(Block* father);
	Block* getFather();
	bool getToLongest();
	void setToLongest();
	int getFatherBlockNum();

private:
	int _fatherBlockNum;
	const int _blockNum;
	char* _blockData;
	int _depth;
	Block* _father;
	bool _isAttechd;
	bool _toLongest;

};

#endif

