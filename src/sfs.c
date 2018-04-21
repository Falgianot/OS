/*
  Simple File System

  This code is derived from function prototypes found /usr/include/fuse/fuse.h
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>
  His code is licensed under the LGPLv2.

*/

#include "params.h"
#include "block.h"

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fuse.h>
#include <libgen.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

#ifdef HAVE_SYS_XATTR_H
#include <sys/xattr.h>
#endif

#include "log.h"

#define NUM_FILES 128
#define NUM_BLOCKS 32768
#define NUM_DIRECT 8
#define NUM_INDIRECT 4
#define FS_SIZE (32768*512)
#define MAX_FILE_SIZE 8388608 //8MB
#define NUM_INODES 128
#define BMAP_INDEX 130 //130-138 Are for Bitmap
#define FREE_BLOCK_START 138

/*
	char buf[512];
	block_read(130,buf);
	
	char temp = buf[55];
	char t = temp|64;
	log_msg("t:%c\n",t);
	buf[55]=t;
	
	block_write(130,buf);
	*/


///////////////////////////////////////////////////////////
//
// Prototypes for all these functions, and the C-style comments,
// come indirectly from /usr/include/fuse.h
//

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
 
 //This stores the block number for a specific inode(file) Use the number to go to the offset
 //(block_num*BLOCK_SIZE) to get to that block
 typedef struct block_node{
	 int block_num;
	 struct block_node * next;
 }block_node;
 
 //Reserve the first x blocks to store the root inodes. If a dir is created
 //that dir inode will point to a block that has all its inodes. 
 typedef struct inode{
	 char * file_name;
	 int type; //1 = file, 0 = dir, -1 = not in use
	 
	 int file_size;
	 int num_blocks;
	 int mode;
	 
	 uid_t uid;
	 gid_t gid;
	 time_t    st_atime;   /* time of last access */
	 time_t    st_mtime;   /* time of last modification */
	 time_t    st_ctime;   /* time of last status change */
	 
	 
	 int d_block[NUM_DIRECT];
	 int indirect_block[NUM_INDIRECT];
	 int double_indirect_block[2];
	 
 }inode;
 
 
 
  
 inode * search_dir(const char * path){
	
	
	
	
 }
 
 
 //function to find inode with given path
 inode * find_inode(const char * path){
	
	
	//REMEMBER TO ADJUST THIS WHEN WE INCORPORATE DIRECTORIES
	
	//check root inode at location block 2
	char buf[BLOCK_SIZE];
	int a = block_read(1,&buf);
	inode * in = (inode *)buf;
	
	log_msg("FINDING INODE for :%s     %s\n",path, in->file_name);
	if(strcmp(path,in->file_name)==0){
		log_msg("FOUND INODE\n");
		return in;
	}
	
	//then check regular inodes starting at block 4
	
	/* So find filename/dirname by going to end or upto next /
	 * Then indirect pointers for filename/dirname
	 * If filename/dirname = path, get that inode.
	 * If it is file, do stuff later like read/write/open/close.
	 * If it is dir, then search indirect again or return if no more in pathname.
	 * If no match, do nothing
	
	*/
	
	int i = 0;
	while(i<NUM_FILES){
		//check indirect of root dir
		
		i++;
		//if root indirect points to dir, need to check that so it we might need a new function that checks dirs.
		
		
		
	}
	
	
	return NULL;
	

 }
 
 
 //info about the filesystem
 typedef struct super_block{
	int init;
	int size_fs;
	int num_files;
	int max_file_size;
	int num_inodes;
	//bitmap for free inodes and blocks
	
 }super_block;
 
 
 //MAKE A ARRAY FOR INDIRECT AND DINDIRECT BLOCKS AND WRITE THAT INTO THE BLOCK
 
 
 
void *sfs_init(struct fuse_conn_info *conn)
{
    fprintf(stderr, "in bb-init\n");
    log_msg("\nsfs_init()\n");
    log_conn(conn);
    log_fuse_context(fuse_get_context());
	
	//Open our disk. Initialize filesystem structure
	disk_open("/.freespace/dv262/testfsfile");
	log_msg("OPENING DISK\n");
    
	
	int i = 0;
	char zero[512];
	memset(zero,0,512);
	while(i<32768){
		block_write(i,&zero);
		i++;
	}
	
	
	
	
    //first block is superblock
    char buff1[BLOCK_SIZE];
    int s = block_read(0,&buff1);
    super_block * sb = (super_block *)buff1;
    sb->init = 1;
    sb->size_fs = FS_SIZE;
    sb->num_files = NUM_FILES;
    sb->max_file_size = MAX_FILE_SIZE;
    sb->num_inodes = NUM_INODES;
    log_msg("WRITING SUPER BLOCK\n");
    block_write(0,sb);
    
	
	
	//Make the second block root inode, third block a single indirect to point to inodes.
	char buf[BLOCK_SIZE];
	int q = block_read(1,&buf);
	inode * a = (inode *)buf;
	a->file_name = "/\0";
	a->type = 0;
	a->file_size = 0;
	a->num_blocks = 0;
	a->mode = S_IFDIR;
	a->uid = S_IRWXU;
	a->gid = S_IRWXG;
	a->st_atime = time(NULL);
	a->st_mtime = a->st_atime;
	a->st_ctime = a->st_atime;
	
	a->indirect_block[0] = 2;
    block_write(1,a);
    
	//loop to create inodes
	i = 0;
	while(i<NUM_FILES){
        char buff2[BLOCK_SIZE];
        //log_msg("before INODE\n");
        int r = block_read(i+3,&buff2);
        //log_msg("after INODE\n");
        inode * in = (inode *)buff2;
		in->file_name = NULL;
        in->type = -1;
        in->file_size = 0;
        in->num_blocks = 0;
        in->mode = 0;
        in->uid = 0;
        in->gid = 0;
        in->st_atime = time(NULL);
        in->st_mtime = in->st_atime;
		in->st_ctime = in->st_atime;
        
        int j = 0;
        while(j<NUM_DIRECT){
            in->d_block[j] = -1;
            j++;
        }
		
		j = 0;
		while(j<NUM_INDIRECT){
        in->indirect_block[j] = -1;
		j++;
		}
		j = 0;
		while(j<2){
        in->double_indirect_block[j] = -1;
		j++;
		}
		
        block_write(i+3,in);
        
		i++;
	}
	
	//Init bitmap
	i = BMAP_INDEX;
	while(i<BMAP_INDEX+8){
		block_write(i,&zero);
		i++;
	}
	
	log_msg("FINISHING INIT\n");
    return SFS_DATA;
}

/**
 * Clean up filesystem
 *
 * Called on filesystem exit.
 *
 * Introduced in version 2.3
 */
void sfs_destroy(void *userdata)
{
    log_msg("\nsfs_destroy(userdata=0x%08x)\n", userdata);
}

/** Get file attributes.
 *
 * Similar to stat().  The 'st_dev' and 'st_blksize' fields are
 * ignored.  The 'st_ino' field is ignored except if the 'use_ino'
 * mount option is given.
 */
int sfs_getattr(const char *path, struct stat *statbuf)
{
    int retstat = 0;
    char fpath[PATH_MAX];
    
    log_msg("\nsfs_getattr(path=\"%s\", statbuf=0x%08x)\n",
	  path, statbuf);
	
	//find inode with path
	log_msg("BOUT TO FIND INODE\n");
	inode * in = find_inode(path);
	
	if(in!=NULL){
	log_msg("NAME:%s\n",in->file_name);
	
	
	statbuf->st_ino = 0;
	statbuf->st_mode = in->mode;
	statbuf->st_nlink = 0;
	statbuf->st_uid = in->uid;
	statbuf->st_gid = in->gid;
	statbuf->st_rdev = 0;
	statbuf->st_size = in->file_size;
	statbuf->st_blocks = in->num_blocks;
	statbuf->st_atime = in->st_atime;
	statbuf->st_mtime = in->st_mtime;
	statbuf->st_ctime = in->st_ctime;
	
	
	}
	
	
	
	//set statbuf with fields from inode
	
    
    return retstat;
}

/**
 * Create and open a file
 *
 * If the file does not exist, first create it with the specified
 * mode, and then open it.
 *
 * If this method is not implemented or under Linux kernel
 * versions earlier than 2.6.15, the mknod() and open() methods
 * will be called instead.
 *
 * Introduced in version 2.5
 */
int sfs_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
    int retstat = 0;
    log_msg("\nsfs_create(path=\"%s\", mode=0%03o, fi=0x%08x)\n",
	    path, mode, fi);
    
    
    return retstat;
}

/** Remove a file */
int sfs_unlink(const char *path)
{
    int retstat = 0;
    log_msg("sfs_unlink(path=\"%s\")\n", path);

    
    return retstat;
}

/** File open operation
 *
 * No creation, or truncation flags (O_CREAT, O_EXCL, O_TRUNC)
 * will be passed to open().  Open should check if the operation
 * is permitted for the given flags.  Optionally open may also
 * return an arbitrary filehandle in the fuse_file_info structure,
 * which will be passed to all file operations.
 *
 * Changed in version 2.2
 */
int sfs_open(const char *path, struct fuse_file_info *fi)
{
    int retstat = 0;
    log_msg("\nsfs_open(path\"%s\", fi=0x%08x)\n",
	    path, fi);

    
    return retstat;
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
int sfs_release(const char *path, struct fuse_file_info *fi)
{
    int retstat = 0;
    log_msg("\nsfs_release(path=\"%s\", fi=0x%08x)\n",
	  path, fi);
    

    return retstat;
}

/** Read data from an open file
 *
 * Read should return exactly the number of bytes requested except
 * on EOF or error, otherwise the rest of the data will be
 * substituted with zeroes.  An exception to this is when the
 * 'direct_io' mount option is specified, in which case the return
 * value of the read system call will reflect the return value of
 * this operation.
 *
 * Changed in version 2.2
 */
int sfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    int retstat = 0;
    log_msg("\nsfs_read(path=\"%s\", buf=0x%08x, size=%d, offset=%lld, fi=0x%08x)\n",
	    path, buf, size, offset, fi);

   
    return retstat;
}

/** Write data to an open file
 *
 * Write should return exactly the number of bytes requested
 * except on error.  An exception to this is when the 'direct_io'
 * mount option is specified (see read operation).
 *
 * Changed in version 2.2
 */
int sfs_write(const char *path, const char *buf, size_t size, off_t offset,
	     struct fuse_file_info *fi)
{
    int retstat = 0;
    log_msg("\nsfs_write(path=\"%s\", buf=0x%08x, size=%d, offset=%lld, fi=0x%08x)\n",
	    path, buf, size, offset, fi);
    
    
    return retstat;
}


/** Create a directory */
int sfs_mkdir(const char *path, mode_t mode)
{
    int retstat = 0;
    log_msg("\nsfs_mkdir(path=\"%s\", mode=0%3o)\n",
	    path, mode);
   
    
    return retstat;
}


/** Remove a directory */
int sfs_rmdir(const char *path)
{
    int retstat = 0;
    log_msg("sfs_rmdir(path=\"%s\")\n",
	    path);
    
    
    return retstat;
}


/** Open directory
 *
 * This method should check if the open operation is permitted for
 * this  directory
 *
 * Introduced in version 2.3
 */
int sfs_opendir(const char *path, struct fuse_file_info *fi)
{
    int retstat = 0;
    log_msg("\nsfs_opendir(path=\"%s\", fi=0x%08x)\n",
	  path, fi);
    
    
    return retstat;
}

/** Read directory
 *
 * This supersedes the old getdir() interface.  New applications
 * should use this.
 *
 * The filesystem may choose between two modes of operation:
 *
 * 1) The readdir implementation ignores the offset parameter, and
 * passes zero to the filler function's offset.  The filler
 * function will not return '1' (unless an error happens), so the
 * whole directory is read in a single readdir operation.  This
 * works just like the old getdir() method.
 *
 * 2) The readdir implementation keeps track of the offsets of the
 * directory entries.  It uses the offset parameter and always
 * passes non-zero offset to the filler function.  When the buffer
 * is full (or an error happens) the filler function will return
 * '1'.
 *
 * Introduced in version 2.3
 */
int sfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
	       struct fuse_file_info *fi)
{
    int retstat = 0;
    
    
    return retstat;
}

/** Release directory
 *
 * Introduced in version 2.3
 */
int sfs_releasedir(const char *path, struct fuse_file_info *fi)
{
    int retstat = 0;

    
    return retstat;
}

struct fuse_operations sfs_oper = {
  .init = sfs_init,
  .destroy = sfs_destroy,

  .getattr = sfs_getattr,
  .create = sfs_create,
  .unlink = sfs_unlink,
  .open = sfs_open,
  .release = sfs_release,
  .read = sfs_read,
  .write = sfs_write,

  .rmdir = sfs_rmdir,
  .mkdir = sfs_mkdir,

  .opendir = sfs_opendir,
  .readdir = sfs_readdir,
  .releasedir = sfs_releasedir
};

void sfs_usage()
{
    fprintf(stderr, "usage:  sfs [FUSE and mount options] diskFile mountPoint\n");
    abort();
}

int main(int argc, char *argv[])
{
    int fuse_stat;
    struct sfs_state *sfs_data;
    
    // sanity checking on the command line
    if ((argc < 3) || (argv[argc-2][0] == '-') || (argv[argc-1][0] == '-'))
	sfs_usage();

    sfs_data = malloc(sizeof(struct sfs_state));
    if (sfs_data == NULL) {
	perror("main calloc");
	abort();
    }

    // Pull the diskfile and save it in internal data
    sfs_data->diskfile = argv[argc-2];
    argv[argc-2] = argv[argc-1];
    argv[argc-1] = NULL;
    argc--;
    
    sfs_data->logfile = log_open();
    
    // turn over control to fuse
    fprintf(stderr, "about to call fuse_main, %s \n", sfs_data->diskfile);
    fuse_stat = fuse_main(argc, argv, &sfs_oper, sfs_data);
    fprintf(stderr, "fuse_main returned %d\n", fuse_stat);
    
    
    
    
    return fuse_stat;
}
