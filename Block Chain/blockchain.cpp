/*
 * blockchain.cpp
 *
 *  Created on: Apr 27, 2015
 *      Author: Yuval & Eric
 */

#include "blockchain.h"
#include "BlockChainObj.h"
#include "Block.h"
#include <unistd.h>
#include <string.h>
#include <iostream>
#include <cstdlib>
#include <pthread.h>
#include <queue>
#include <list>
#include <limits.h>
#include <map>
#include <set>
#include <list>
#include "hash.h"
#include <string.h>

using namespace std;

#define FAIL -1
#define BLOCK_DOESNT_EXIST -2
#define ATTACHED 1
#define NOT_ATTACHED 0
#define GENESIS_ID 0
#define NOT_CALLED -2
#define CALLED 3
#define CALLED_AND_FINISH 0
#define NO_BLOCK_TO_ATTACH_NOW -1
#define SUCCESS 0

#define UNINITIALIZED 0
#define INITIALIZING 1
#define INITIALIZED 2
#define CLOSING 3
#define CLOSED 4

/* struct with all the info to hash block's data and make the attachment */
typedef struct {
	Block* block;
	int length;
	char* data;
} TStruct;

// --------------- globals ---------------
/* The chain */
BlockChainObj* theChain = NULL;
/* list of pending blocks */
list<TStruct> hashQueue;
/* list of block to attached now */
list<int> attachBlocksNum;
/* The blockNum of the current hashed block */
int currentHashingBlockNum;
/* The daemon thread */
pthread_t hashingThread;
/* The closing thread */
pthread_t closingThread;
/* The static mutex */
static pthread_mutex_t theMutex = PTHREAD_MUTEX_INITIALIZER;
/* The chain status */
int chainStatus = UNINITIALIZED;
/* True if the deamon runs */
bool isDeamonRun;

/**
 * Free all the allocated memory
 */
void terminateLibrary()
{

	Block* blockToClose;
	TStruct toTerminate;
	while (!hashQueue.empty())
	{
		toTerminate = hashQueue.front();
		hashQueue.pop_front();

		free(toTerminate.block->getData());
		toTerminate.block->addData(NULL);

		free(toTerminate.data);
		theChain->updateAvailableBlockNum(toTerminate.block->getBlockNum());
		toTerminate.data = NULL;
		delete toTerminate.block;

	}
	//dealing with all the blocks that attached to the chain:
	map<int, Block*>::iterator it;
	for (it = theChain->usedBlock.begin(); it != theChain->usedBlock.end();) {
		blockToClose = it->second;
		free(blockToClose->getData());
		theChain->updateAvailableBlockNum(blockToClose->getBlockNum());
		blockToClose->addData(NULL);
		delete blockToClose;

		++it;
	}

	delete theChain;
	theChain = NULL;
}

/**
 * Lock the static mutex and check it correctness.
 */
void mutexLocker()
{
	int res = pthread_mutex_lock(&theMutex);
	if(res != SUCCESS)
	{
		terminateLibrary();
		exit(1);
	}
}

/**
 * Unlock the static mutex and check it correctness.
 */
void mutexUnlocker()
{
	int res = pthread_mutex_unlock(&theMutex);
	if(res != SUCCESS)
	{
		terminateLibrary();
		exit(1);
	}
}

/**
 * The function of the closing thread
 */
void *closingThraedFunc(void* args) {


	mutexLocker();

	TStruct toAttach;
	int nonce;
	char* hashedData;
	Block* blockToClose;
	isDeamonRun = false;

	mutexUnlocker();

	// make the deamon thread to stop
	pthread_join(hashingThread, NULL);

	mutexLocker();

	if (theChain == NULL) {
		mutexUnlocker();
		return NULL;
	}
	// dealing with all the pending threads.
	while (!hashQueue.empty()) {
		toAttach = hashQueue.front();
		hashQueue.pop_front();

		nonce = generate_nonce(toAttach.block->getBlockNum(),
				toAttach.block->getFather()->getBlockNum());

		free(toAttach.block->getData());
		toAttach.block->addData(NULL);

		hashedData = generate_hash(toAttach.data, toAttach.length, nonce);

		cout << hashedData << endl;

		free(hashedData);
		hashedData = NULL;
		free(toAttach.data);
		theChain->updateAvailableBlockNum(toAttach.block->getBlockNum());

		theChain->usedBlock.erase(toAttach.block->getBlockNum());

		toAttach.data = NULL;

		//cout << "Pending block : " << toAttach.block->getBlockNum() << endl;

		delete toAttach.block;


	}
	//dealing with all the blocks that attached to the chain:
	map<int, Block*>::iterator it;
	for (it = theChain->usedBlock.begin(); it != theChain->usedBlock.end();) {
		blockToClose = it->second;

		//cout << "Closing block : " << blockToClose->getBlockNum() << endl;

		free(blockToClose->getData());
		theChain->updateAvailableBlockNum(blockToClose->getBlockNum());
		blockToClose->addData(NULL);

		delete blockToClose;
		++it;
	}


	delete theChain;
	theChain = NULL;

	close_hash_generator();

	chainStatus = CLOSED;
	mutexUnlocker();
	pthread_exit(NULL);

}

/**
 * The function of the deamon thread
 */
void *theHashingCenter(void* args) {
	TStruct toAttach;
	int nonce;

	while (isDeamonRun) {

		if (theChain == NULL) {
			return NULL;
		}

		if (hashQueue.empty()) {
			continue;
		}

		int blockNumToAttached;

		mutexLocker();

		while (!attachBlocksNum.empty()) {

			blockNumToAttached = attachBlocksNum.front();
			attachBlocksNum.pop_front();

			// remove from hashQueue to new thread
			list<TStruct>::iterator it;
			for (it = hashQueue.begin(); it != hashQueue.end();) {

				if (it->block->getBlockNum() == blockNumToAttached) {
					toAttach = *it;
					it = hashQueue.erase(it);
				} else {
					++it;
				}

			}
			// return the block to the pending list if it is not attached already
			if(!toAttach.block->getIsAtteched())
			{
				hashQueue.push_front(toAttach);
			}
		}

		// pop the next block to hash and attached
		toAttach = hashQueue.front();
		hashQueue.pop_front();

		currentHashingBlockNum = toAttach.block->getBlockNum();


		// check if the block is orphan or toLongest called
		if (toAttach.block->getToLongest()) {
			toAttach.block->setToLongest();
			toAttach.block->setFather(theChain->getRandomDeepestBlock());
		}

		nonce = generate_nonce(toAttach.block->getBlockNum(),
				toAttach.block->getFather()->getBlockNum());


		mutexUnlocker();

		char* hashedData = generate_hash(toAttach.data, toAttach.length, nonce);
		toAttach.block->addData(hashedData);

		mutexLocker();


		// if close chain called
		if (chainStatus == CLOSING) {
			cout << toAttach.block->getData() << endl;
			free(toAttach.data);
			free(toAttach.block->getData());
			toAttach.block->addData(NULL);

			toAttach.data = NULL;
			mutexUnlocker();
			continue;
		}

		// if to_longest called
		if (toAttach.block->getToLongest()) {

			toAttach.block->setFather(theChain->getRandomDeepestBlock());
			free(toAttach.block->getData());
			toAttach.block->addData(NULL);

			hashQueue.push_front(toAttach);

		}
		else
		{
			toAttach.block->setIsAtteched();
			theChain->addToDeepestBlocks(toAttach.block);
			theChain->incBlockCounter();
			free(toAttach.data);
			toAttach.data = NULL;
		}

		mutexUnlocker();

	}
	pthread_exit(NULL);

}

// --------------- library  functions -------------- //
int init_blockchain() {

	mutexLocker();

	if (chainStatus != UNINITIALIZED && chainStatus != CLOSED) {
		mutexUnlocker();
		return FAIL;
	}


	Block* genesisBlock = NULL;

	isDeamonRun = true;
	chainStatus = INITIALIZING;
	init_hash_generator();

	try {
		genesisBlock = new Block(GENESIS_ID, NULL);
		theChain = new BlockChainObj(genesisBlock);
		theChain->addToUsedBlock(GENESIS_ID, genesisBlock);
		genesisBlock->setIsAtteched();
	} catch (bad_alloc& ba) {

		mutexUnlocker();
		return FAIL;
	}

	int res = pthread_create(&hashingThread, NULL, theHashingCenter, NULL);
	if (res != SUCCESS) {
		close_hash_generator();
		delete genesisBlock;
		delete theChain;

		mutexUnlocker();
		return FAIL;
	}


	chainStatus = INITIALIZED;
	mutexUnlocker();
	return SUCCESS;


}

int add_block(char *data, int length)
{
	// check if closing in process
	if(chainStatus != INITIALIZED)
	{
		return FAIL;
	}

	mutexLocker();

	int blockNum = theChain->getLowestAvalibleBlockNum();
	if (blockNum == FAIL) {

		mutexUnlocker();
		return FAIL;
	}

	Block* newBlock = new Block(blockNum, theChain->getRandomDeepestBlock());
	theChain->addToUsedBlock(blockNum, newBlock);

	char* copiedData = (char*) malloc(sizeof(char) * length);
	memcpy(copiedData, data, length);
	TStruct toHash = { newBlock, length, copiedData };
	hashQueue.push_back(toHash);


	mutexUnlocker();

	return blockNum;
}

int to_longest(int block_num)
{
	// check if closing in process
	if(chainStatus != INITIALIZED)
	{
		return FAIL;
	}

	// check if it is the genesis block
	if (block_num == GENESIS_ID) {
		return FAIL;
	}

	mutexLocker();


	// check if the block exist
	if (!theChain->blockNumInUse(block_num)) {

		mutexUnlocker();
		return BLOCK_DOESNT_EXIST;
	}

	Block* block = theChain->getBlockByBlockNum(block_num);
	// check if block already attached
	if (block->getIsAtteched()) {

		mutexUnlocker();
		return ATTACHED;
	}

	block->setToLongest();


	mutexUnlocker();

	return SUCCESS;
}

int attach_now(int block_num) {

	// check if closing in process
	if(chainStatus != INITIALIZED)
	{
		return FAIL;
	}

	if (currentHashingBlockNum == block_num) {
		while(was_added(block_num) != 1);
		return SUCCESS;
	}

	mutexLocker();


	if (!theChain->blockNumInUse(block_num))
	{

		mutexUnlocker();
		return BLOCK_DOESNT_EXIST;
	}

	Block* block = theChain->getBlockByBlockNum(block_num);
	if(!block->getIsAtteched())
	{
		attachBlocksNum.push_back(block_num);

	}
	mutexUnlocker();

	while(was_added(block_num) != 1);
	return SUCCESS;
}

int was_added(int block_num) {

	if(chainStatus != INITIALIZED)
	{
		return FAIL;
	}

	mutexLocker();

	Block* block = theChain->getBlockByBlockNum(block_num);
	if (block == NULL) {

		mutexUnlocker();
		return BLOCK_DOESNT_EXIST;
	}

	if (block->getIsAtteched())
	{

		mutexUnlocker();
		return ATTACHED;
	}

	mutexUnlocker();
	return NOT_ATTACHED;
}

int chain_size()
{

	if(chainStatus != CLOSING && chainStatus != INITIALIZED)
	{
		return FAIL;
	}

	return theChain->getBlockCounter();
}


int prune_chain() {
	// check if closing in process
	if(chainStatus != INITIALIZED)
	{

		return FAIL;
	}

	mutexLocker();


	set<int> blocksInTheChain;
	Block* deepest = theChain->getRandomDeepestBlock();
	Block* traveler = deepest;

	while (traveler != NULL)
	{
		blocksInTheChain.insert(traveler->getBlockNum());
		traveler = traveler->getFather();

	}

	theChain->clearDeepestBlocks(deepest);

	map<int, Block*>::iterator it;
	for (it = theChain->usedBlock.begin(); it != theChain->usedBlock.end();)
	{
		// check if the block is not on the longest chain
		if (blocksInTheChain.find(it->first) == blocksInTheChain.end()) {
			if (it->second->getIsAtteched()) {
				delete theChain->getBlockByBlockNum(it->first);
				theChain->updateAvailableBlockNum(it->first);
				it = theChain->usedBlock.erase(it);
			} else {
				//the block is still pending
				if (blocksInTheChain.find(it->second->getFatherBlockNum()) == blocksInTheChain.end()) {
					it->second->setFather(deepest);

				}
				++it;
			}

		}
		else
		{
			++it;
		}
	}

	mutexUnlocker();

	return SUCCESS;
}

void close_chain()
{

	mutexLocker();

	if (chainStatus == INITIALIZED)
	{
		chainStatus = CLOSING;
		int res = pthread_create(&closingThread, NULL, closingThraedFunc, NULL);
		if (res != SUCCESS)
		{
			terminateLibrary();
			exit(1);
		}
	}

	mutexUnlocker();

}

int return_on_close() {

	mutexLocker();

	if (chainStatus == UNINITIALIZED)
	{
		mutexUnlocker();
		return FAIL;
	}

	if (chainStatus == INITIALIZED)
	{
		mutexUnlocker();
		return NOT_CALLED;
	}

	mutexUnlocker();

	if (chainStatus == CLOSING)
  	{
		pthread_join(closingThread, NULL);
	}

	return CALLED_AND_FINISH;

}
