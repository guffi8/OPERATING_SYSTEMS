/*
 * Block.cpp
 *
 *  Created on: Apr 27, 2015
 *      Author: Yuval & Eric
 */

#include <unistd.h>
#include <iostream>
#include <cstdlib>
#include <signal.h>
#include <pthread.h>
#include <queue>
#include <list>
#include "Block.h"
#include <stdlib.h>

/**
 * The constructor. construct new block. get the block's number and the block's father.
 */
Block::Block(int blockNum, Block* father): _blockNum(blockNum) {
	//Block::_blockNum = blockNum;
	Block::_blockData = NULL;
	Block::_father = father;

	if (Block::_father != NULL) {
		Block::_depth = Block::_father->_depth + 1;
		Block::_fatherBlockNum = father->getBlockNum();
	} else {
		Block::_fatherBlockNum = 0;
		Block::_depth = 0;
	}

	Block::_isAttechd = false;
	Block::_toLongest = false;


}

/**
 * The de-constructor. free the block's data.
 */
Block::~Block()
{
	if (Block::_blockData != NULL)
	{
		free(Block::_blockData);
	}
	Block::_blockData = NULL;
}

/**
 * Add new data to the block.
 */
void Block::addData(char* hashedData)
{
	Block::_blockData = hashedData;
}

/**
 * return the block data.
 */
char* Block::getData()
{
	return Block::_blockData;
}

/**
 * return the block's depth.
 */
int Block::getDepth()
{
	return Block::_depth;
}

/**
 * return the block id number.
 */
int Block::getBlockNum()
{
	return this->_blockNum;
}

/**
 * Mark the block as attached.
 */
void Block::setIsAtteched()
{
	Block::_isAttechd = true;
}

bool Block::getIsAtteched()
{
	return Block::_isAttechd;
}

/**
 * Set the block new father and update the block's depth according to the new father.
 */
void Block::setFather(Block* father)
{

	Block::_father = father;
	Block::_depth = Block::_father->_depth + 1;
	Block::_fatherBlockNum = Block::_father->getBlockNum();

}

/**
 * return the block's father.
 */
Block* Block::getFather()
{
	return Block::_father;
}

/**
 * return true if lo_longest called for this block, false otherwise.
 */
bool Block::getToLongest()
{
	return Block::_toLongest;
}

/**
 * Change the to_longest status of the block.
 */
void Block::setToLongest()
{
	if(Block::_toLongest) {
		Block::_toLongest = false;
	}
	else
	{
		Block::_toLongest = true;
	}
}

/**
 * return the id number of the block's father.
 */
int Block::getFatherBlockNum()
{
	return Block::_fatherBlockNum;
}



