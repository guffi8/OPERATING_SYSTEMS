/*
 * CachingFileSystem.cpp
 *
 *  Created on: 15 April 2015
 *  Author: Netanel Zakay, HUJI, 67808  (Operating Systems 2014-2015).
 */

#define FUSE_USE_VERSION 26

#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <fuse.h>
#include <fcntl.h>
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
#include "CacheFileSystemLog.h"
#include "CacheFileSystemBuffer.h"

#define SUCCESS 0
#define FAIL -1
#define VALID_ARG_NUM 5
#define ROOT_DIR 1
#define MOUNT_DIR 2
#define BLOCK_NUM 3
#define BLOCK_SIZE 4
#define FILE_SYSTEM_LOGGER "/.filesystem.log"

#define INVALID_INPUT_ERR_MASSAGE "usage: CachingFileSystem rootdir mountdir numberOfBlocks blockSize\n"
#define SYSTEM_ERROR "System Error\n"
#define CACHE_DATA ((cache_data *) fuse_get_context()->private_data)

#define GETATTAR_FUNC "getattar"
#define F_GETATTAR_FUNC "fgettattr"
#define ACCESS_FUNC "access"
#define OPEN_FUNC "open"
#define READ_FUNC "read"
#define FLUSH_FUNC "flush"
#define RELEASE_FUNC "release"
#define OPEN_DIR_FUNC "opendir"
#define READ_DIR_FUNC "readdir"
#define RELEASE_DIR_FUNC "releasedir"
#define RENAME_FUNC "rename"
#define INIT_FUNC "init"
#define DESTROY_FUNC "destroy"
#define IOCTL_FUNC "ioctl"

using namespace std;

/**
 * Struct with all the global variables
 */
typedef struct cache_data {
	char* rootDirPath;
	CacheFileSystemLog* logger;
	CacheFileSystemBuffer* cacheBuffer;
} cache_data;

struct fuse_operations caching_oper;

/**
 * paste the relative mount file to the root file
 */
static void getFullPath(char fullPath[PATH_MAX], const char * path)
{
	strcpy(fullPath, CACHE_DATA->rootDirPath);
	strncat(fullPath, path + 1, PATH_MAX);
}

/**
 * Return 0 if the given path is directory, else return -1
 */
static int isDirectory(char const* path)
{
	DIR *dir= opendir(path);
	if(dir)
	{
		closedir(dir);
		return SUCCESS;
	}
	return FAIL;
}

/**
 * check if file exist
 */
static int isFileExist(char const* path)
{
	struct stat s;
	if(stat(path, &s) < 0)
	{
		return FAIL;
	}
	return SUCCESS;
}



/**
 * Get a char* input and return the length of the string that it represent
 */
static int length(const char* name)
{
	int counter = 0;
	while(*name != '\0')
	{
		counter++;
		name++;
	}
	return counter;
}

/** Get file attributes.
 *
 * Similar to stat().  The 'st_dev' and 'st_blksize' fields are
 * ignored.  The 'st_ino' field is ignored except if the 'use_ino'
 * mount option is given.
 */
int caching_getattr(const char *path, struct stat *statbuf){
	CACHE_DATA->logger->writeFileSystemFunc(GETATTAR_FUNC);

	// check if the given path is to long
	if(length(path) > PATH_MAX)
	{
		return -ENAMETOOLONG;
	}

	// check if trying to open the .fileSystem.log file
	if(strcmp(path, FILE_SYSTEM_LOGGER) == SUCCESS) {
		return -ENOENT;
	}

	char fullPath[PATH_MAX];
	getFullPath(fullPath, path);

	int res = lstat(fullPath, statbuf);
	if(res != 0)
	{
		return -ENOENT;
	}
	return SUCCESS;
}

/**
 * Get attributes from an open file
 *
 * This method is called instead of the getattr() method if the
 * file information is available.
 *
 * Currently this is only called after the create() method if that
 * is implemented (see above).  Later it may be called for
 * invocations of fstat() too.
 *
 * Introduced in version 2.5
 */
int caching_fgetattr(const char *path, struct stat *statbuf, struct fuse_file_info *fi){
	CACHE_DATA->logger->writeFileSystemFunc(F_GETATTAR_FUNC);

	// check if the given path is to long
	if(length(path) > PATH_MAX)
	{
		return -ENAMETOOLONG;
	}

	// check if trying to open the .fileSystem.log file
	if(strcmp(path, FILE_SYSTEM_LOGGER) == SUCCESS) {
		return -ENOENT;
	}

	if(strcmp(path, "/") == SUCCESS) {
		return caching_getattr(path, statbuf);
	}

	if(fstat(fi->fh, statbuf) < 0)
	{
		return -errno;
	}

    return SUCCESS;
}

/**
 * Check file access permissions
 *
 * This will be called for the access() system call.  If the
 * 'default_permissions' mount option is given, this method is not
 * called.
 *
 * This method is not called under Linux kernel versions 2.4.x
 *
 * Introduced in version 2.5
 */
int caching_access(const char *path, int mask)
{
	CACHE_DATA->logger->writeFileSystemFunc(ACCESS_FUNC);

	// check if the given path is to long
	if(length(path) > PATH_MAX)
	{
		return -ENAMETOOLONG;
	}

	// check if trying to open the .fileSystem.log file
	if(strcmp(path, FILE_SYSTEM_LOGGER) == SUCCESS) {
		return -ENOENT;
	}

	char fullPath[PATH_MAX];
	getFullPath(fullPath, path);

	if(access(fullPath, mask) < 0) {
		return -errno;
	}
    return 0;
}


/** File open operation
 *
 * No creation, or truncation flags (O_CREAT, O_EXCL, O_TRUNC)
 * will be passed to open().  Open should check if the operation
 * is permitted for the given flags.  Optionally open may also
 * return an arbitrary filehandle in the fuse_file_info structure,
 * which will be passed to all file operations.

 * pay attention that the max allowed path is PATH_MAX (in limits.h).
 * if the path is longer, return error.

 * Changed in version 2.2
 */
int caching_open(const char *path, struct fuse_file_info *fi){
	CACHE_DATA->logger->writeFileSystemFunc(OPEN_FUNC);

	// check if the given path is to long
	if(length(path) > PATH_MAX)
	{
		return -ENAMETOOLONG;
	}

	// check if trying to open the .fileSystem.log file
	if(strcmp(path, FILE_SYSTEM_LOGGER) == SUCCESS) {
		return -ENOENT;
	}

	int fd;
	char fullpath[PATH_MAX];

	getFullPath(fullpath, path);
	fd = open(fullpath, fi->flags);
	if(fd < 0)
	{
		return -errno;
	}
	if((fi->flags & 3) != O_RDONLY)
	{
		return -EACCES;
	}

	fi->fh = fd;
	return SUCCESS;
}


/** Read data from an open file
 *
 * Read should return exactly the number of bytes requested except
 * on EOF or error, otherwise the rest of the data will be
 * substituted with zeroes.
 *
 * Changed in version 2.2
 */
int caching_read(const char *path, char *buf, size_t size, off_t offset,
		struct fuse_file_info *fi){
	CACHE_DATA->logger->writeFileSystemFunc(READ_FUNC);

	// check if the given path is to long
	if(length(path) > PATH_MAX)
	{
		return -ENAMETOOLONG;
	}

	char fullPath[PATH_MAX];
	getFullPath(fullPath, path);
	int buffSize = CACHE_DATA->cacheBuffer->getDataFormCache(fullPath, buf, size, offset, fi);
	if (buffSize < 0)
	{
		return -ENXIO;
	}

	return buffSize;
}

/** Possibly flush cached data
 *
 * BIG NOTE: This is not equivalent to fsync().  It's not a
 * request to sync dirty data.
 *
 * Flush is called on each close() of a file descriptor.  So if a
 * filesystem wants to return write errors in close() and the file
 * has cached dirty data, this is a good place to write back data
 * and return any errors.  Since many applications ignore close()
 * errors this is not always useful.
 *Howev
e
r, there are several functions that are a bit more complicated and we will describe them here.

open
. While open is a simple function, pay attention that you should use the given flags (in the
fuse_f
ile_info
).

readdir
. This function implementation is a bit more complicated than a simple forward. Pay attention to the
difference between what you are requested to do and what Linux's “
readdir
” does.

read
. The read function is the most important function
in your file system
 * NOTE: The flush() method may be called more than once for each
 * open().  This happens if more than one file descriptor refers
 * to an opened file due to dup(), dup2() or fork() calls.  It is
 * not possible to determine if a flush is final, so each flush
 * should be treated equally.  Multiple write-flush sequences are
 * relatively rare, so this shouldn't be a problem.
 *
 * Filesystems shouldn't assume that flush will always be called
 * after some writes, or that if will be called at all.
 *
 * Changed in version 2.2
 */
int caching_flush(const char *path, struct fuse_file_info *fi)
{
	CACHE_DATA->logger->writeFileSystemFunc(FLUSH_FUNC);

	// check if the given path is to long
	if(length(path) > PATH_MAX)
	{
		return -ENAMETOOLONG;
	}

    return 0;
}

/** Release an open file
 *
 * Release is called when there are no more references to an open
 * file: all file descriptors are closed and all memory mappings
 * are unmapped.
 *
 * For every open() call there will be exactly one release() call
 * with the same flags and file descriptor.  It is possible to
 * have a file opened more than once, in which case only the last
 * release will mean, that no more reads/writes will happen on the
 * file.  The return value of release is ignored.
 *
 * Changed in version 2.2
 */
int caching_release(const char *path, struct fuse_file_info *fi){
	CACHE_DATA->logger->writeFileSystemFunc(RELEASE_FUNC);

	// check if the given path is to long
	if(length(path) > PATH_MAX)
	{
		return -ENAMETOOLONG;
	}
	return close(fi->fh);
}

/** Open directory
 *
 * This method should check if the open operation is permitted for
 * this  directory
 *
 * Introduced in version 2.3
 */
int caching_opendir(const char *path, struct fuse_file_info *fi){
	CACHE_DATA->logger->writeFileSystemFunc(OPEN_DIR_FUNC);

	// check if the given path is to long
	if(length(path) > PATH_MAX)
	{
		return -ENAMETOOLONG;
	}

	DIR *dp;
	char fullPath[PATH_MAX];
	getFullPath(fullPath, path);

	dp = opendir(fullPath);
	if (dp == NULL)
	{
		return -errno;
	}
	fi->fh = (intptr_t) dp;

	return 0;
}

/** Read directory
 *
 * This supersedes the old getdir() interface.  New applications
 * should use this.
 *
 * The readdir implementation ignores the offset parameter, and
 * passes zero to the filler function's offset.  The filler
 * function will not return '1' (unless an error happens), so the
 * whole directory is read in a single readdir operation.  This
 * works just like the old getdir() method.
 *
 * Introduced in version 2.3
 */
int caching_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
		struct fuse_file_info *fi){

	CACHE_DATA->logger->writeFileSystemFunc(READ_DIR_FUNC);

	// check if the given path is to long
	if(length(path) > PATH_MAX)
	{
		return -ENAMETOOLONG;
	}

	DIR *dp;
	struct dirent *de;
	dp = (DIR *) (uintptr_t) fi->fh;

	de = readdir(dp);
	if(de == 0) {
		return -errno;
	}
	bool isItTheRoot = false;
	if(strcmp(path, "/") == SUCCESS)
	{
		isItTheRoot = true;
	}

	do {

		if(isItTheRoot && strcmp(de->d_name, FILE_SYSTEM_LOGGER + 1) == SUCCESS) {
			continue;
		}
		if(filler(buf, de->d_name, NULL, 0) != SUCCESS) {
			return -ENOENT;
		}
	} while ((de = readdir(dp)) != NULL);

	return SUCCESS;
}

/** Release directory
 *
 * Introduced in version 2.3
 */
int caching_releasedir(const char *path, struct fuse_file_info *fi){
	CACHE_DATA->logger->writeFileSystemFunc(RELEASE_DIR_FUNC);

	// check if the given path is to long
	if(length(path) > PATH_MAX)
	{
		return -ENAMETOOLONG;
	}

	closedir((DIR *) (uintptr_t) fi->fh);
	return 0;
}

/** Rename a file */
int caching_rename(const char *path, const char *newpath){
	CACHE_DATA->logger->writeFileSystemFunc(RENAME_FUNC);

	// check if the given path and the given new path is to long
	if(length(path) > PATH_MAX || length(newpath) > PATH_MAX)
	{
		return -ENAMETOOLONG;
	}

	char fullPath[PATH_MAX];
	char fullNewPath[PATH_MAX];

	getFullPath(fullPath, path);
	getFullPath(fullNewPath, newpath);

	// check if the file exist
	if(isFileExist(fullNewPath) == SUCCESS)
	{
		return -EEXIST;
	}

	int isDir = isDirectory(fullPath);

	if(rename(fullPath, fullNewPath) < 0)
	{
		return -errno;
	}

	// Update all block file name that relative to new name .
	CACHE_DATA->cacheBuffer->updateFileNameInCache(fullPath, fullNewPath, isDir);

	return SUCCESS;
}

/**
 * Initialize filesystem
 *
 * The return value will passed in the private_data field of
 * fuse_context to all file operations and as a parameter to the
 * destroy() method.
 *
 * Introduced in version 2.3
 * Changed in version 2.6
 */
void *caching_init(struct fuse_conn_info *conn){
	CACHE_DATA->logger->writeFileSystemFunc(INIT_FUNC);
	return fuse_get_context()->private_data;
}


/**
 * Clean up filesystem
 *
 * Called on filesystem exit.
 *
 * Introduced in version 2.3
 */
void caching_destroy(void *userdata){
	CACHE_DATA->logger->writeFileSystemFunc(DESTROY_FUNC);

	delete CACHE_DATA->logger;
	delete CACHE_DATA->cacheBuffer;
	free(CACHE_DATA->rootDirPath);
	delete CACHE_DATA;
}


/**
 * Ioctl from the FUSE sepc:
 * flags will have FUSE_IOCTL_COMPAT set for 32bit ioctls in
 * 64bit environment.  The size and direction of data is
 * determined by _IOC_*() decoding of cmd.  For _IOC_NONE,
 * data will be NULL, for _IOC_WRITE data is out area, for
 * _IOC_READ in area and if both are set in/out area.  In all
 * non-NULL cases, the area is of _IOC_SIZE(cmd) bytes.
 *
 * However, in our case, this function only needs to print cache table to the log file .
 *
 * Introduced in version 2.8
 */
int caching_ioctl (const char *, int cmd, void *arg,
		struct fuse_file_info *, unsigned int flags, void *data) {
	CACHE_DATA->logger->writeFileSystemFunc(IOCTL_FUNC);

	// getting the info from the cache
	vector<block_info*> info;
	CACHE_DATA->cacheBuffer->getBlockInfo(&info);

	// print all the info to the .fileSystem.log
	vector<block_info*>::iterator it;
	for(it = info.begin(); it != info.end(); ++it)
	{
		CACHE_DATA->logger->writeLFUinfo((*it)->fileName.substr(length(CACHE_DATA->rootDirPath)),
				(*it)->blockNum, (*it)->accessNum);
		//free((*it)->fileName);
		delete (*it);
	}

	return 0;
}


// Initialise the operations.
// You are not supposed to change this function.
void init_caching_oper()
{
	caching_oper.getattr = caching_getattr;
	caching_oper.access = caching_access;
	caching_oper.open = caching_open;
	caching_oper.read = caching_read;
	caching_oper.flush = caching_flush;
	caching_oper.release = caching_release;
	caching_oper.opendir = caching_opendir;
	caching_oper.readdir = caching_readdir;
	caching_oper.releasedir = caching_releasedir;
	caching_oper.rename = caching_rename;
	caching_oper.init = caching_init;
	caching_oper.destroy = caching_destroy;
	caching_oper.ioctl = caching_ioctl;
	caching_oper.fgetattr = caching_fgetattr;


	caching_oper.readlink = NULL;
	caching_oper.getdir = NULL;
	caching_oper.mknod = NULL;
	caching_oper.mkdir = NULL;
	caching_oper.unlink = NULL;
	caching_oper.rmdir = NULL;
	caching_oper.symlink = NULL;
	caching_oper.link = NULL;
	caching_oper.chmod = NULL;
	caching_oper.chown = NULL;
	caching_oper.truncate = NULL;
	caching_oper.utime = NULL;
	caching_oper.write = NULL;
	caching_oper.statfs = NULL;
	caching_oper.fsync = NULL;
	caching_oper.setxattr = NULL;
	caching_oper.getxattr = NULL;
	caching_oper.listxattr = NULL;
	caching_oper.removexattr = NULL;
	caching_oper.fsyncdir = NULL;
	caching_oper.create = NULL;
	caching_oper.ftruncate = NULL;
}


//basic main. You need to complete it.
int main(int argc, char* argv[]) {

	// check input validation
	if((argc != VALID_ARG_NUM) || (atoi(argv[BLOCK_NUM]) <= 0) ||
			(atoi(argv[BLOCK_SIZE]) <= 0) || isDirectory(argv[ROOT_DIR]) != 0  ||
			isDirectory(argv[MOUNT_DIR]) != 0 || !(strcmp(argv[ROOT_DIR], argv[MOUNT_DIR]))) {
		cout << INVALID_INPUT_ERR_MASSAGE;
		exit(1);
	}

	// create the global data struct
	cache_data* data;
	try {
		data = new cache_data();

		char rootDir[PATH_MAX];
		strcpy(rootDir, argv[ROOT_DIR]);
		strcat(rootDir, "/");

		data->logger = new CacheFileSystemLog(rootDir);
		data->cacheBuffer = new CacheFileSystemBuffer(atoi(argv[BLOCK_NUM]), atoi(argv[BLOCK_SIZE]));
		data->rootDirPath = (char*)malloc(PATH_MAX*sizeof(char));
		strcpy(data->rootDirPath, rootDir);

	} catch (bad_alloc& ba) {
		cerr << SYSTEM_ERROR;
		exit(1);
	}

	// TODO

	init_caching_oper();
	argv[1] = argv[2];
	for (int i = 2; i< (argc - 1); i++){
		argv[i] = NULL;
	}
    argv[2] = (char*) "-s";
	argc = 3;

	int fuse_stat = fuse_main(argc, argv, &caching_oper, data);
	return fuse_stat;
}
