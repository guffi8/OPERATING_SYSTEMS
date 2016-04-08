/*
 * BlockChainObj.h
 *
 *  Created on: Apr 28, 2015
 *      Author: yuval
 */

#ifndef _BLOCKCHAINOBJ_H
#define _BLOCKCHAINOBJ_H

#include "Block.h"
#include <unistd.h>
#include <iostream>
#include <cstdlib>
#include <pthread.h>
#include <queue>
#include <list>
#include <limits.h>
#include <algorithm>
#include <map>

using namespace std;

/**
 * The Block Chain Object.
 */
class BlockChainObj {

public:
	BlockChainObj(Block* genesisBlock);
	~BlockChainObj();
	int getLowestAvalibleBlockNum();
	void pushNewBlockNum(int blockNum);
	void incBlockCounter();
	void decBlockCounter();
	int getBlockCounter();
	bool blockNumInUse(int blockNum);
	void addToUsedBlock(int blockNum, Block* block);
	void updateAvailableBlockNum(int blockNum);
	Block* getBlockByBlockNum(int blockNum);
	void clearDeepestBlocks(Block* toStay);

	void addToDeepestBlocks(Block* toAdd);
	Block* getRandomDeepestBlock();

	map<int, Block*> usedBlock;

private:

	int _avalibleBlockNum;

	pthread_mutex_t *_mutex;
	vector<Block*> _deepestBlocks;
	Block* _genesisBlock;
	int _blocksCounter;
	priority_queue<int, vector<int>, greater<int> > _lowestBlockNum;

	int getDeepestLevel();
};

#endif
