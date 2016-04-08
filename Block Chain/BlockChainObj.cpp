/*
 * BlockChainObj.cpp
 *
 *  Created on: Apr 28, 2015
 *      Author: yuval
 */

#include "BlockChainObj.h"
#include <unistd.h>
#include <iostream>
#include <cstdlib>
#include <pthread.h>
#include <queue>
#include <list>
#include <limits.h>
#include <algorithm>
#include <map>
#include "hash.h"
#include <stdlib.h>

#define FAIL -1

/**
 * The constructor. Construct a new chain. Get the genesis block of the chain
 */
BlockChainObj::BlockChainObj(Block* genesisBlock)
{

	BlockChainObj::_genesisBlock = genesisBlock;
	BlockChainObj::_deepestBlocks.push_back(genesisBlock);
	BlockChainObj::_blocksCounter = 0;


	BlockChainObj::_avalibleBlockNum = 1;

	BlockChainObj::_mutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(BlockChainObj::_mutex, NULL);

}

/**
 * The de-constructor. free the object's mutex.
 */
BlockChainObj::~BlockChainObj()
{

	pthread_mutex_destroy(BlockChainObj::_mutex);
	free(BlockChainObj::_mutex);

}

/**
 * Return the lowest id available.
 */
int BlockChainObj::getLowestAvalibleBlockNum()
{
	pthread_mutex_lock(BlockChainObj::_mutex);
	int blockNum;

	// check if there is no a id available from pruned block
	if(BlockChainObj::_lowestBlockNum.empty())
	{
		// check if there are too many block already
		if(BlockChainObj::_avalibleBlockNum >= INT_MAX)
		{
			pthread_mutex_unlock(BlockChainObj::_mutex);
			return FAIL;
		}
		blockNum = BlockChainObj::_avalibleBlockNum;
		BlockChainObj::_avalibleBlockNum++;
		pthread_mutex_unlock(BlockChainObj::_mutex);

		return blockNum;
	}

	blockNum = BlockChainObj::_lowestBlockNum.top();
	BlockChainObj::_lowestBlockNum.pop();
	pthread_mutex_unlock(BlockChainObj::_mutex);

	return blockNum;

}

/**
 * If block is pruned so save is past id to reused by new block
 */
void BlockChainObj::pushNewBlockNum(int blockNum)
{
	pthread_mutex_lock(BlockChainObj::_mutex);
	_lowestBlockNum.push(blockNum);
	pthread_mutex_unlock(BlockChainObj::_mutex);
}

/**
 * Increase the number of the blocks in the chain.
 */
void BlockChainObj::incBlockCounter()
{
	pthread_mutex_lock(BlockChainObj::_mutex);
	BlockChainObj::_blocksCounter++;
	pthread_mutex_unlock(BlockChainObj::_mutex);
}

/**
 * Decrease the number of the blocks in the chain.
 */
void BlockChainObj::decBlockCounter()
{
	pthread_mutex_lock(BlockChainObj::_mutex);
	BlockChainObj::_blocksCounter--;
	pthread_mutex_unlock(BlockChainObj::_mutex);
}

/**
 * Return the current number of the block in the chain.
 */
int BlockChainObj::getBlockCounter()
{
	return BlockChainObj::_blocksCounter;
}

/**
 * Get block number and return true if the block is used already. return false otherwise.
 */
bool BlockChainObj::blockNumInUse(int blockNum)
{
	pthread_mutex_lock(BlockChainObj::_mutex);
	if(BlockChainObj::usedBlock.find(blockNum) == BlockChainObj::usedBlock.end())
	{
		pthread_mutex_unlock(BlockChainObj::_mutex);
		return false;
	}
	pthread_mutex_unlock(BlockChainObj::_mutex);
	return true;
}

/**
 * Add the new block to the used block' map.
 */
void BlockChainObj::addToUsedBlock(int blockNum, Block* block)
{
	pthread_mutex_lock(BlockChainObj::_mutex);
	BlockChainObj::usedBlock[blockNum] = block;
	pthread_mutex_unlock(BlockChainObj::_mutex);
}

/**
 * remove the block to the used block' map.
 */
void BlockChainObj::updateAvailableBlockNum(int blockNum)
{
	pthread_mutex_lock(BlockChainObj::_mutex);


	if(blockNum != 0)
	{
		BlockChainObj::_lowestBlockNum.push(blockNum);
	}

	pthread_mutex_unlock(BlockChainObj::_mutex);

}

/**
 * Get the block in the chain by his id.
 */
Block* BlockChainObj::getBlockByBlockNum(int blockNum)
{
	if(blockNumInUse(blockNum))
	{
		return BlockChainObj::usedBlock[blockNum];
	}
	return NULL;
}

/**
 * Add the given block to the most depth blocks. Save all blocks with the most deepest depth.
 */
void BlockChainObj::addToDeepestBlocks(Block* toAdd)
{
	pthread_mutex_lock(BlockChainObj::_mutex);
	if(toAdd->getDepth() >= BlockChainObj::getDeepestLevel())
	{
		if(toAdd->getDepth() > BlockChainObj::getDeepestLevel())
		{
			BlockChainObj::_deepestBlocks.erase(BlockChainObj::_deepestBlocks.begin(),
					BlockChainObj::_deepestBlocks.end());
		}
		BlockChainObj::_deepestBlocks.push_back(toAdd);
	}
	pthread_mutex_unlock(BlockChainObj::_mutex);

}

/**
 * Return the deepest level of the chain in current time.
 */
int BlockChainObj::getDeepestLevel()
{
	return BlockChainObj::_deepestBlocks.front()->getDepth();
}

/**
 * Return the deepest block in current time. Arbitrary between all the deepest's blocks
 */
Block* BlockChainObj::getRandomDeepestBlock()
{
	pthread_mutex_lock(BlockChainObj::_mutex);
	random_shuffle(BlockChainObj::_deepestBlocks.begin(),BlockChainObj::_deepestBlocks.end());
	Block* deepest = BlockChainObj::_deepestBlocks.front();
	pthread_mutex_unlock(BlockChainObj::_mutex);
	return deepest;
}

/**
 * Clear the deepest's block list and push the given block.
 */
void BlockChainObj::clearDeepestBlocks(Block* toStay)
{
	pthread_mutex_lock(BlockChainObj::_mutex);

	BlockChainObj::_deepestBlocks.erase(BlockChainObj::_deepestBlocks.begin(),
						BlockChainObj::_deepestBlocks.end());
	if(toStay != NULL)
	{
		BlockChainObj::_deepestBlocks.push_back(toStay);
	}

	pthread_mutex_unlock(BlockChainObj::_mutex);
}
