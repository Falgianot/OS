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
 
 typedef struct indir_array{
	int offsets[128];
	
 }indir_array;
 
 
 //Reserve the first x blocks to store the root inodes. If a dir is created
 //that dir inode will point to a block that has all its inodes. 
 typedef struct inode{
	 char file_name[100];
	 int type; //1 = file, 0 = dir, -1 = not in use
	 
	 int file_size;
	 int num_blocks;
	 int offset;
	 mode_t mode;
	 
	 uid_t uid;
	 gid_t gid;
	 time_t    st_atime;   /* time of last access */
	 time_t    st_mtime;   /* time of last modification */
	 time_t    st_ctime;   /* time of last status change */
	 
	 
	 int d_block[NUM_DIRECT];
	 int indirect_block[NUM_INDIRECT];
	 int double_indirect_block[2];
	 
 }inode;
 
 
char * my_strcpy(char * dest, char * src){
	//log_msg("SRC:%s\n",src);
	char * start = dest;

	while(*src !='\0'){
		
		*dest = *src;
		//log_msg("MIDDLE\n");
		dest++;
		src++;
	}
	//log_msg("FINAL\n");
	*dest = '\0';
	return start;



}



 
void print_offset(){
	char buf[BLOCK_SIZE];
	block_read(3,&buf);
	inode * in = (inode*)buf;
	int i =0;
	while(i<NUM_DIRECT){
		log_msg("OFFSET OF 138 INODE= %d\n",in->d_block[i]);
		i++;
		
	}
	
}
//subtract block_num-138, mod that by 4096(make sure its int), that is bit map block number
//add 138 to the mod answer

//given a block number, flip the bit in the bit map
int flip_bit(int block_num){
	int bit_num = block_num-138;
	int bit_block = (int)((bit_num/4096))+130;

	char buf[BLOCK_SIZE];
	block_read(bit_block,&buf);
	
	int byte = (bit_num/8);

	//print_offset();
	unsigned char temp ='0';
	memcpy(&temp,&buf[byte],1);//byte to flip
	//log_msg("TEMPPPPP:%x\n",temp);
	int bit_index = bit_num%8;
	temp = temp >> (7-bit_index);
	
	//print_offset();
	temp = temp &1;
	unsigned char temp2='0';
	memcpy(&temp2,&temp,1);
	
	//print_offset();
	temp2 = temp2|1;
	temp2 = temp2<<(7-bit_index);
	unsigned char flipped;
	memcpy(&flipped,&buf[byte],1);

	//print_offset();
	//log_msg("BITNUM:%i      BITBLOCK:%i    BYTE:%i     BITiNDEX:%i\n",bit_num,bit_block,byte,bit_index);
	
	//log_msg("BEFORE  temp:%x 	temp2:%x 	flipped:%x  buf:%x\n",temp,temp2,flipped,buf[byte]);

	int to_return = -1;
	if(temp == 1){
		//log_msg("FLIPPING 1 TO 0\n");//use to free
		//XOR
		flipped = flipped^temp2;
		//log_msg("flipped from 1 to 0:%x\n",flipped);
		to_return = 0;
	}else if(temp == 0){
		//log_msg("FLIPPING 0 TO 1\n");//free to use
		//OR
		flipped = flipped|temp2;
		//log_msg("flipped from 0 to 1:%x\n",flipped);
		to_return = 1;
	}

	//log_msg("AFTER :temp:%x 	temp2:%x 	flipped:%x  buf:%x\n",temp,temp2,flipped,buf[byte]);

	//print_offset();
	memcpy(&buf[byte],&flipped,1);
	
	//print_offset();
	block_write(bit_block,&buf);
	block_read(bit_block,&buf);
	//print_offset();
	//log_msg("AFTER2 :temp:%x 	temp2:%x 	flipped:%x  buf:%x\n",temp,temp2,flipped,(unsigned char)buf[byte]);
	return to_return;
}

int find_free_block(){
	
	char buf[BLOCK_SIZE];
	int i = FREE_BLOCK_START;
	int free = -1;
	
	//flip the bit at block location starting at 138. If flip_bit returns 1 it is free. If flip_bit returns 0 it is in use
	//i is the location of the free block
	while(i<NUM_BLOCKS){
		free = flip_bit(i);
		//found free bit
		log_msg("free:%i   blocknum:%i\n",free,i);
		if(free==1){
			//print_offset();
			return i;
		}
		//not free
		else if(free==0){
			flip_bit(i);
		}
		
		i++;
	}
	
	return -1;
	
}

int search_dir(const char * path, inode * in){
	/* So find filename/dirname by going to end or upto next /
	 * Then indirect pointers for filename/dirname
	 * If filename/dirname = path, get that inode.
	 * If it is file, do stuff later like read/write/open/close.
	 * If it is dir, then search indirect again or return if no more in pathname.
	 * If no match, do nothing
	
	*/
	
	//Get indirect block for dir
	int indir_offset = in->indirect_block[0];
	char buff[BLOCK_SIZE];
	int a = block_read(indir_offset,&buff);
	
	//Cast it 
	indir_array * b = (indir_array *)buff;
	int i = 0;
	
	
	char n_path[strlen(path)];//removes the slash
	strcpy(n_path,path);
	char * new_path = n_path + 1;
	log_msg("New path:%s\n",new_path);
	while(i<128){
		//log_msg("offset:%i\n",b[i]);
		//Now check inodes in this block. //If offset is 0 then its empty.
		int inode_offset = b->offsets[i];
		//log_msg("OFFFFFSET:%i\n",inode_offset);
		//if offset is 0 then there is not an inode
		if(inode_offset==0){
			i++;
			continue;
		}
		char buffer[BLOCK_SIZE];
		block_read(inode_offset, &buffer);
		inode * temp_inode = (inode *)buffer;
		
		log_msg("FOUND INODE NAME:%s    Size:%i\n",temp_inode->file_name,temp_inode->file_size);
		//Now compare file name to path
		if(strcmp(temp_inode->file_name,new_path)==0){
			log_msg("FOUND INODE IN SEARCH DIR\n");
			return temp_inode->offset;
		}
		
		
		//If current inode is a dir, search it 
		if(temp_inode->type==0){
			int end = 0;
			while(new_path[end]!='/'){
				end++;
			}
			char* to_send = new_path+end;
			log_msg("Send to:%s\n",to_send);
			int ii = search_dir(to_send,temp_inode);
			if(ii!=-1){
				return ii;
			}
		}
		i++;
	}
	
	return -1;
	
 }
 
 
 //function to find inode with given path
 int find_inode(const char * path){
	
	
	//REMEMBER TO ADJUST THIS WHEN WE INCORPORATE DIRECTORIES
	
	//check root inode at location block 2
	char buf[BLOCK_SIZE];
	int a = block_read(1,&buf);
	inode * in = (inode *)buf;
	
	log_msg("FINDING INODE for :%s     %s\n",path, in->file_name);
	if(strcmp(path,in->file_name)==0){
		log_msg("FOUND INODE\n");
		return 1;//index of root inode
	}
	
	
	//Start searching through the directory. Starting root.
	return search_dir(path,in);
	

 }
 
 
 //info about the filesystem
 typedef struct super_block{
	int init;
	int size_fs;
	int num_files;//update on create and unlink
	int max_file_size;//update on write and unlink. max amount file can write.
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
    
	//RRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRREMEMBER TO CHECK SUPERBLOCK TO PRESERVE FS
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
	a->file_name[0] = '/';
	a->type = 0;
	a->file_size = 0;
	a->num_blocks = 0;
	a->mode = S_IFDIR|S_IRWXU|S_IRWXG|S_IRWXO;
	a->uid = getuid();
	a->gid = getgid();
	a->st_atime = time(NULL);
	a->st_mtime = a->st_atime;
	a->st_ctime = a->st_atime;
	a->offset = 0;
	
	a->indirect_block[0] = 2;
	char bb[BLOCK_SIZE];
	block_read(2,&bb);
	indir_array * o = (indir_array *)bb;
	int z = 0;
	while(z<128){
		o->offsets[z] = 0;
		z++;
		}
	block_write(2,o);
    block_write(1,a);
    
	//loop to create inodes
	i = 0;
	while(i<NUM_FILES){
        char buff2[BLOCK_SIZE];
        //log_msg("before INODE\n");
        int r = block_read(i+3,&buff2);
        //log_msg("after INODE\n");
        inode * in = (inode *)buff2;
		in->file_name[0] = 0;
        in->type = -1;
        in->file_size = 0;
        in->num_blocks = 0;
        in->mode = 0;
        in->uid = 0;
        in->gid = 0;
        in->st_atime = time(NULL);
        in->st_mtime = in->st_atime;
		in->st_ctime = in->st_atime;
		in->offset = i+3;
        
        int j = 0;
        while(j<NUM_DIRECT){
            in->d_block[j] = 0;
            j++;
        }
		
		j = 0;
		while(j<NUM_INDIRECT){
        in->indirect_block[j] = 0;
		j++;
		}
		j = 0;
		while(j<2){
        in->double_indirect_block[j] = 0;
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
	//log_msg("BOUT TO FIND INODE\n");
	char b[BLOCK_SIZE];
	int o = find_inode(path);
	block_read(o,&b);
	
	inode * in = (inode *)b;
	if(o==-1){
		in = NULL;
	}
	if(in!=NULL){
	//log_msg("NAME:%s\n",in->file_name);
	
	
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
	
	
	}else{
		log_msg("TRAAAAAAAAAAAAASH\n");
		retstat = -ENOENT;
	}
	
	
	
	//set statbuf with fields from inode
	
    log_msg("Returning from attr\n");
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
	//check path. mountdir/hello.txt,   /hello/gbye.txt
	
	char n_path[strlen(path)];//removes the slash
	strcpy(n_path,path);
	char * new_path = n_path + 1;
	log_msg("New path from create:%s\n",new_path);
	
	
	int exist = sfs_open(path,fi);
	if(exist==0){
			return 0;
		
	}
	
	//we will have to change this when we do directories
	char buf[BLOCK_SIZE];
	block_read(1,&buf);
	inode * in = (inode *)buf;
	int indir_offset = in->indirect_block[0];
	log_msg("CREATE offset:%i\n",indir_offset);
	
	
	char buf3[BLOCK_SIZE];
	block_read(indir_offset,&buf3);
	
	indir_array * c = (indir_array *)buf3;
	
	
	
	//get super block
	char buf_super[BLOCK_SIZE];
	block_read(0,&buf_super);
	super_block * sb = (super_block *)buf_super;
	
	
	
    //find free inode
	int i = 0;
	while(i<NUM_FILES){
		char buf2[BLOCK_SIZE];
		block_read(i+3,&buf2);
		inode * in2 = (inode *)buf2;
		
		
		
		//log_msg("MODE:0%03o\n",S_IFREG|S_IRUSR|S_IWUSR);
		if(in2->file_name[0]==0){
			
			if((mode&(S_IFDIR|S_IRUSR|S_IWUSR))==mode){
				log_msg("Create dir:0%03o\n",S_IFDIR);
				in2->type = 0;
			}else if((mode&(S_IFREG|S_IRUSR|S_IWUSR))==mode){
				log_msg("Create file:0%03o\n",S_IFREG);
				in2->type = 1;
			}
			strcpy(in2->file_name,new_path);
			in2->file_name[strlen(new_path)] = '\0';
			//log_msg("CHECKING WHATEVER:%s\n",in2->file_name);
			in2->file_size = 0;
			in2->num_blocks = 0;
			in2->mode = mode;
			in2->uid = fuse_get_context()->uid;
			in2->gid = fuse_get_context()->gid;
			struct timespec requestStart;
			clock_gettime(CLOCK_REALTIME, &requestStart);
			in2->st_atime = requestStart.tv_sec;
			in2->st_mtime = requestStart.tv_sec;
			in2->st_ctime = requestStart.tv_sec;
			//log_msg("WRITING INODE:%i\n",i+3);
			block_write(i+3,in2);
			
			//update number of files in fs
			sb->num_files = sb->num_files + 1;
			block_write(0,sb);
			
			
			
			//log_msg("SIZEOF:%i\n",sizeof(*in2));
			//char bu[512];
			//block_read(i+3,&bu);
			//log_msg("Reading back:%s\n",((inode *)bu)->file_name);
			break;
		}
		
		i++;
	}
	// i is the offset we put in indirect block;
	int j = 0;
	while(j<128){
		if(c->offsets[j]==0){
			//found free spot
			c->offsets[j]=i+3;
			break;
		}
		j++;
	}

	block_write(indir_offset,c);
	//log_msg("PUTTING INODE:%i\n",indir_o);
	
	
	//put that offset into the indirect block of the current dir
    
    return retstat;
}



int slash_count(const char * path){
	int i =0;
	int slash_counter=0;
	while (i<strlen(path)){
		if(path[i]=='/'){
			slash_counter++;
		}
		i++;
	}
	log_msg("the number of slashes is equal %d\n", slash_counter);
	return slash_counter;
	
	
	
	}

/** Remove a file */
int sfs_unlink(const char *path)
{
    int retstat = 0;
    log_msg("sfs_unlink(path=\"%s\")\n", path);
	
	
	
	//get the superblock
	char buf_super[BLOCK_SIZE];
	block_read(0,&buf_super);
	super_block * sb = (super_block *)buf_super;
	
	
	
	//find the inode
	char b[BLOCK_SIZE];
	int o = find_inode(path);
	block_read(o,&b);
	inode * in = (inode *)b;
	
	//free blocks and set the bitmap
	int j = 0;
	int offset = 0;
   	char empty_buf[BLOCK_SIZE];
   	char buf[BLOCK_SIZE];
   	memset(empty_buf,0,BLOCK_SIZE);

	
	while(j<NUM_DIRECT){
		offset = in->d_block[j];
		log_msg("offfffff:%i\n");
		if(offset!=0){
			
			
			block_write(offset,&empty_buf);
			flip_bit(offset);
			in->d_block[j] = 0;
		}
		j++;
	}
	
	j = 0;
	while(j<NUM_INDIRECT){
		offset = in->indirect_block[j];
		if(offset!=0){
			flip_bit(offset);
			int k = 0;
			block_read(offset,&buf);
			indir_array * arr =(indir_array *)buf;
			while(k<128){
				if(arr->offsets[k]!=0){
					block_write(arr->offsets[k],&empty_buf);
					flip_bit(arr->offsets[k]);
				
				}
				k++;
			}
			
		}
		
		
		block_write(offset,empty_buf);
		in->indirect_block[j] = 0;
		j++;

	}
	
	//do later
	j = 0;
	while(j<2){
		in->double_indirect_block[j] = 0;
		j++;
	}
	
	
	//update parent dir indirect.
	//check root first
	int slashes =0;
	log_msg("slashes is equal %d\n",slashes);
	slashes = slash_count(path);
	log_msg("slashes is equal %d\n",slashes);
	
	if(slashes==1){
		log_msg("Removing file from indir of root\n");
		char b[BLOCK_SIZE];
		block_read(2,b);
		indir_array * a =(indir_array *)b;
		int l = 0;
		while(l<128){
			if(a->offsets[l]!=0){
				log_msg("Removing file from indir of root fo sho\n");
				char bu[BLOCK_SIZE];
				block_read(a->offsets[l],bu);
				inode * i_n = (inode *)bu;
				if(strcmp(i_n->file_name,in->file_name)==0){
					log_msg("%s, %s\n",i_n->file_name,in->file_name);
					in->file_name[0] = 0;
					in->type = -1;
					in->file_size = 0;
					in->num_blocks = 0;
					in->mode = 0;
					in->uid = 0;
					in->gid = 0;
					in->st_atime = time(NULL);
					in->st_mtime = in->st_atime;
					in->st_ctime = in->st_atime;
					//WILL THE OFFSET BE RESET?
					block_write(a->offsets[l],in);
					a->offsets[l]=0;
					block_write(2,a);
					break;
					}
				
				}
			
			l++;
			}
		
	
}else{
	//other dirs
	
	
}
	//update num files in fs
	sb->num_files = sb->num_files + 1;
	block_write(0,sb);

    
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
	
	//check if file exists
	char b[BLOCK_SIZE];
	int o = find_inode(path);
	block_read(o,&b);
	inode * in = (inode *)b;
	
	if(in==NULL){
		log_msg("ERROR ON OPEN\n");
		return -EACCES;
	}
	
	if(in->mode!=(S_IFREG|S_IRUSR|S_IWUSR)){
			log_msg("ERROR ON OPEN   perm:0%03o\n",in->mode);
			return -EACCES;
		}
	
	
	//if file exists, check permissions. Return
	if(in->mode==(S_IFREG|S_IRUSR|S_IWUSR)){
		//Update access time
		log_msg("OPENING FILE\n");
	
		struct timespec requestStart;
		clock_gettime(CLOCK_REALTIME, &requestStart);
		in->st_atime = requestStart.tv_sec;
		
		block_write(in->offset,in);
		
		retstat = 0;
		return retstat;
	}else{
		return -EACCES;
	}
	
	
	
    
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


//This function fills in the passed buffer with the files data
void fill_buffer(char * buffer, inode * in){
	
	log_msg("FILLING BUFFER from file %s\n",in->file_name);	
	int offset = 0;
	
	
	
	int j = 0;
	char empty_buf[BLOCK_SIZE];
	//direct
	while(j<NUM_DIRECT){
		offset = in->d_block[j];
		print_offset();
		//log_msg("offfffff:%i\n");
		if(offset!=0){
			block_read(offset,&empty_buf);
			log_msg("Reading %s from fill_buffer\n",empty_buf);
			strcat(buffer,empty_buf);
		}
		j++;
	}
	
	j = 0;
	//indirect
	char buf[BLOCK_SIZE];
	while(j<NUM_INDIRECT){
		offset = in->indirect_block[j];
		if(offset!=0){
			int k = 0;
			block_read(offset,&buf);
			indir_array * arr =(indir_array *)buf;
			
			while(k<128){
				if(arr->offsets[k]!=0){
					block_read(arr->offsets[k],&empty_buf);
					strcat(buffer,empty_buf);
				}
				k++;
			}
			
		}
		
		j++;

	}
	//double indirect
	//do later
	j = 0;

	while(j<2){
		in->double_indirect_block[j] = 0;
		j++;
	}
	
	
	
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
	    print_offset();
	    char b[BLOCK_SIZE];
	    int o = find_inode(path);
	    block_read(o,&b);
	    inode* in = (inode*) b;
	    
	    char buffer_data[in->num_blocks*512];
		fill_buffer(buffer_data,in);
		log_msg("buffer_data includes : %s\n",buffer_data);
		
		//char final_buffer[(in->num_blocks*512)-offset];
		
		int i =0;
		int z = offset;
		log_msg("strlen of buffer_data:%d\n",strlen(buffer_data));
		while(i< strlen(buffer_data) -(int)offset){
			(buf[i]) = (buffer_data[z]);
			log_msg("final_buffer in read[insert]: %c and buffer_data[z]:%c\n",buf[i],buffer_data[z]);
			z++;
			i++;
			
		}

		
	    

   
    return strlen(buf);
}


//This function puts data back into inodes data blocks after a write
void fill_data(char * buffer, inode * in){
	
	log_msg("FILLING DATA\n");	
	int offset = 0;
	int global_offset = 0;
	int j = 0;
	char empty_buf[BLOCK_SIZE];
	//direct
	while(j<NUM_DIRECT){
		offset = in->d_block[j];
		//log_msg("offfffff:%i\n");
		if(offset!=0){
			log_msg("DIR PUTTING BACK IN:%s\n",buffer+(512*global_offset));
			block_write(offset,buffer+(512*global_offset));
			global_offset++;
			
		}
		j++;
	}
	
	j = 0;
	//indirect
	char buf[BLOCK_SIZE];
	while(j<NUM_INDIRECT){
		offset = in->indirect_block[j];
		if(offset!=0){
			int k = 0;
			block_read(offset,&buf);
			indir_array * arr =(indir_array *)buf;
			
			while(k<128){
				if(arr->offsets[k]!=0){
					log_msg("INDIR PUTTING BACK IN:%s\n",buffer+(512*global_offset));
					block_write(arr->offsets[k],buffer+(512*global_offset));
					global_offset++;
				}
				k++;
			}
			
		}
		
		j++;

	}
	//double indirect
	//do later
	j = 0;

	while(j<2){
		in->double_indirect_block[j] = 0;
		j++;
	}
	
	
	
}

//These functions loop through the blocks to write the offset into the arrays.
int find_free_direct(inode * in,int off){
	log_msg("FIND FREE DIRECT\n");
	int offset = 0;
	
	int j = 0;
	char empty_buf[BLOCK_SIZE];
	//direct
	while(j<NUM_DIRECT){
		if(in->d_block[j]==0){
			print_offset();
			in->d_block[j] = off;
			block_write(in->offset,in);
			log_msg("FOUND FREE DIRECT index:%i   offset:%i\n",j,off);
			return 1;
		}
		j++;
	}
	
	return -1;
}

int find_free_indirect(inode * in,int off){
	log_msg("FIND FREE INDIRECT\n");
	
	int offset = 0;
	
	int j = 0;
	char empty_buf[BLOCK_SIZE];
	char buf[BLOCK_SIZE];
	while(j<NUM_INDIRECT){
		offset = in->indirect_block[j];
		if(offset==0){
			//make a new indirect block
			int free = find_free_block();
			if(free!=-1){
				//found a free block to make an indirect
				in->indirect_block[j] = free;//set index of indirect block which was free
				block_write(in->offset,in);
				
			}else{
				log_msg("ERROR FINDING INDIRECT\n");
				return -1;
			}
			block_read(free,&buf);//read that indirect block and initializeit
			indir_array * arr =(indir_array *)buf;
			int l = 0;
			while(l<128){
				arr->offsets[l] = 0;
				l++;
			}
			
			arr->offsets[0] = off;//set the offset of data block in indirect bock
			block_write(free,arr);
			log_msg("FOUND NEW FREE INDIRECT off:%i\n",off);
			return 1;
				
		}else{
			//check curent indirect block for an open spot.
			int k = 0;
			block_read(offset,&buf);//offset is index of indirect block
			indir_array * arr =(indir_array *)buf;
			
			while(k<128){
				if(arr->offsets[k]==0){
					arr->offsets[k] = off;
					block_write(offset,arr);
					log_msg("FOUND FREE SPOT IN INDIRECT BLOCK index:%i  off\n",k,off);
					return 1;
				}
				k++;
			}
				
			
			
		}
		
		j++;

	}
	
	return -1;
}

int find_free_d_indirect(inode * in){
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
	
	//find inode for path
	char bbb[BLOCK_SIZE];
	int o = find_inode(path);
	block_read(o,&bbb);
	inode * in = (inode *)bbb;
	
	//construct buffer with all data from that nodes data blocks. // max they can write is 4096
	//char * buffer_data = (char *)malloc(sizeof(char)*(in->num_blocks*512));//load files data into this
	char buffer_data[in->num_blocks*512];


	int ok = 0;
	while(ok<in->num_blocks*512){
		buffer_data[ok]=0;
		ok++;
	}
	
	log_msg("File name to write:%s\n",in->file_name);
	fill_buffer(buffer_data,in);
	
	
	//figure out how many blocks we will need. ((num_blocks*512)-(file_size))-size = rest needed to fill
	//(rest/512)+1=#of blocks needed
	
	int num_blks_needed = 0;
	if(in->file_size==0){
		
		log_msg("INIT WRITE\n");
		num_blks_needed = (size/512)+1;
		if(size%512==0){
			num_blks_needed-=1;		
		}
		//data to be written into fs
		char write_data[num_blks_needed*512];
		//log_msg("wd init size:%i\n",strlen(write_data));
		memcpy(write_data,buf,strlen(buf));
		//log_msg("Data:%s   numblocks:%i\n",write_data,num_blks_needed);
		
		
		//search for free blocks and write to it
		int i = 0;
		while(i<num_blks_needed){
			int free =find_free_block();
			if(free!=-1){
				//found free block
				//log_msg("FREE:%i     Data written:%s    writedata:%i\n",free,write_data+(512*i),strlen(write_data));
				//update blocks
				
				int success = 0;
				success = find_free_direct(in,free);
				block_write(free,(write_data+(512*i)));
			}
			else{
				//error
				return 0;
					
			}
		
		
			i++;		
		}
		
		in->file_size = in->file_size + (int)(size);
		log_msg("NEW FILE SIZE:%i   offset:%i   num_blks%i\n",in->file_size,in->offset,num_blks_needed);
		in->num_blocks = in->num_blocks + num_blks_needed;
		block_write(in->offset,in);
		return size;
		
	}else{
		log_msg("EXTRA WRITE fillbuffer:%s  blocks in inode:%i         filesize:%i\n",buffer_data,in->num_blocks,in->file_size);
		
		//Start at offset.
		int remainder = ((in->num_blocks * 512) - (in->file_size));//how much we can put in current block
		log_msg("Remainder:%i      size-rem:%i\n",remainder,size-remainder);
		
		//This means we can fit the write in the last block possibly
		if((int)(size - remainder) <= 0){
			int z = 0;
			int insert = offset;//starting point to insert
			//log_msg("insert::%i      srlen(buf);:%i\n",insert,strlen(buf));
			
			//copy byte by byte into buffer_data
			while(z<strlen(buf)){
				(buffer_data[insert]) = (buf[z]);
				log_msg("buffer_data[insert]: %c and buf[z]:%c\n",buffer_data[insert],buf[z]);
				insert++;
				z++;
			}
			
			//write it back in
			log_msg("EXTRA BUFFER:%s\n",buffer_data);
			fill_data(buffer_data,in);
			in->file_size = strlen(buffer_data);
			log_msg("NEW FILE SIZE:%i   offset:%i   num_blks%i\n",in->file_size,in->offset,num_blks_needed);
			block_write(in->offset,in);
			
		}else{
		//This means we need fill last block, and then need new blocks.
		
		
			log_msg("filling oriignal block and need more\n");
			
			//fill in last block if possible.Take remainder bytes from buf, put it in buffer_data then get new blocks.
			if(remainder !=0){
				int z = 0;
				int insert = offset;//starting point to insert
				//log_msg("insert::%i      srlen(buf);:%i\n",insert,strlen(buf));
				
				//copy byte by byte into buffer_data
				while(z<remainder){
					(buffer_data[insert]) = (buf[z]);
					log_msg("buffer_data[insert]: %c and buf[z]:%c\n",buffer_data[insert],buf[z]);
					insert++;
					z++;
				}
				
				num_blks_needed =((strlen(buf)-z)/512)+1;
				if((strlen(buf)-z)%512==0){
					num_blks_needed-=1;		
				}
				
				
				//write it back in
				log_msg("EXTRA BUFFER:%s\n",buffer_data);
				fill_data(buffer_data,in);
				in->file_size = strlen(buffer_data);
				log_msg("NEW FILE SIZE:%i   offset:%i   num_blks%i\n",in->file_size,in->offset,num_blks_needed);
				
				int i = 0;
				while(i<num_blks_needed){
					int free =find_free_block();
					log_msg("FOUND NEW FREE BLOCK IN WRITE:%i\n",free);
					if(free!=-1){
						//found free block
						//log_msg("FREE:%i     Data written:%s    writedata:%i\n",free,write_data+(512*i),strlen(write_data));
						//update blocks
						int free_index=0;
						char free_block[BLOCK_SIZE];
						
						block_read(free,&free_block);
						while(z<(int)size){
							(free_block[free_index]) = (buf[z]);
							log_msg("free_block[free_index]: %c and buf[z]:%c\n",free_block[free_index],buf[z]);
							free_index++;
							z++;	
						}
						int success = 0;
						success = find_free_direct(in,free);
						if(success == -1){
							success = find_free_indirect(in,free);
						}
						in->file_size += strlen(free_block);
						in->num_blocks = in->num_blocks + 1;
						
						log_msg("NEW FILE SIZE WITH EXTRA BLOCKS:%i   offset:%i   num_blks%i\n",in->file_size,in->offset,num_blks_needed);
						block_write(in->offset,in);
						block_write(free,free_block);
					}
					else{
						//error
						return 0;
							
					}
				
				
					i++;		
				}
				
				
				
			}else{
				//Need new blocks right away.
				int z =0;
				int i = 0;
				while(i<num_blks_needed){
					int free =find_free_block();
					if(free!=-1){
						//found free block
						//log_msg("FREE:%i     Data written:%s    writedata:%i\n",free,write_data+(512*i),strlen(write_data));
						//update blocks
						int free_index=0;
						char free_block[BLOCK_SIZE];
						int abc = 0;
						while (abc < BLOCK_SIZE){
							free_block[abc] = 0;
							abc++;
						}
						log_msg("str len %d\n",strlen(buf));
						log_msg("string buf %s\n",buf);
						block_read(free,&free_block);
						while(z<(int)size){
							(free_block[free_index]) = (buf[z]);
							log_msg("free_block that needs a block right away[free_index]: %c and buf[z]:%c\n",free_block[free_index],buf[z]);
							free_index++;
							z++;	
						}
						int success = 0;
						success = find_free_direct(in,free);
						if(success == -1){
							success = find_free_indirect(in,free);
						}
						in->file_size += (int)size;
						in->num_blocks = in->num_blocks + 1;
						
						log_msg("NEW FILE SIZE WITH EXTRA BLOCKS only!!!!!!!!!!!!!!!!!!:%i   offset:%i   num_blks%i\n",in->file_size,in->offset,num_blks_needed);
						block_write(in->offset,in);
						block_write(free,free_block);
					}
					else{
						//error
						return 0;
							
					}
				
				
					i++;		
				}
			}
		}
		
	}
	//in->file_size += size;
	//in->num_blocks += num_blks_needed;
	
	
	//update sb
	
	//write to that buffer at offset with buf passed in. this may overwrite data.
	
	//if they need a new block, check bitmap.
    
    
    return size;
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
	log_msg("IN READDIR\n");
    //find inode with given path
    char bbb[BLOCK_SIZE];
    int o = find_inode(path);
    block_read(o,&bbb);
	inode * in = (inode *)bbb;
	//get indirect block
	int indir_offset = in->indirect_block[0];
	log_msg("indir off:%d\n",indir_offset);
	//now fill the names in this dir into buffer
	char buff[BLOCK_SIZE];
	int a = block_read(indir_offset,&buff);
	
	//Cast it 
	indir_array * b = (indir_array *)buff;
	int i = 0;
	//log_msg("b[0]:%i     b[1]:%i\n",b->offsets[0],b->offsets[1]);
	while(i<128){
		int a = block_read(indir_offset,&buff);
	
		//Cast it 
		indir_array * b = (indir_array *)buff;
		
		int inode_offset = b->offsets[i];
		
		//log_msg("%reaadier offset: %d\n",inode_offset);
		//if offset is 0 then there is not an inode
		if(inode_offset==0){
			i++;
			continue;
		}
		block_read(inode_offset, &buff);
		inode * temp_inode = (inode *)buff;
		//log_msg("readdir filename: %s\n",temp_inode->file_name);
		int status = filler(buf,temp_inode->file_name,NULL,0);
		if(status ==1)
			log_msg("buffer full\n");
		else
			log_msg("buffer not full\n");
		//log_msg("second time b[0]:%i     b[1]:%i\n",b->offsets[0],b->offsets[1]);
		
		i++;
	}
	

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
