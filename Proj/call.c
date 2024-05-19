/* 
* CSCI3150 Introduction to Operating Systems
*
* --- Declaration ---
* I declare that the assignment here submitted is original except for source
* materials explicitly acknowledged. I also acknowledge that I am aware of
* University policy and regulations on honesty in academic work, and of the
* disciplinary guidelines and procedures applicable to breaches of such policy
* and regulations, as contained in the website
* http://www.cuhk.edu.hk/policy/academichonesty/
*
*
* Source material acknowledgements (if any):
* 
* Students whom I have discussed with (if any):
* 
*/

#include "call.h"
const char *HD = "HD";

/* self-defined helper functions */
inode* read_inode(int fd, int i_number);
int read_byte(int block, int offset, int count, void* buf);
int read_dir_mappings(int fd, int blk_num, DIR_NODE* p_block);
int min(int a, int b);


int open_t(char *pathname)
{
	int inode_number;
	// write your code here.
	
	int hard_disk = open(HD, O_RDONLY);
	inode* ip = read_inode(hard_disk, 0); // read the root dir
	DIR_NODE* p_block = (DIR_NODE*)malloc(BLK_SIZE);

	char* path_cpy = (char*)malloc(sizeof(*pathname)); 
	strcpy(path_cpy, pathname);

	for(char* dir = strtok(path_cpy, "/"); 
			  dir != NULL; 
			  dir = strtok(NULL, "/")) 
	{
		read_dir_mappings(hard_disk, ip->direct_blk[0], p_block);
		for(int i=0; i<ip->sub_f_num; ++i) // search for the wanted file / dir
			if(strcmp(dir, p_block[i].f_name) == 0) {
				free(ip);
				ip = read_inode(hard_disk, p_block[i].i_number);
				break;
			}
	}

	inode_number = ip->i_number;

	close(hard_disk);
	free(path_cpy);
	free(p_block);
	free(ip);

	return inode_number;

}


int read_t(int i_number, int offset, void *buf, int count)
{
	int read_bytes;
	// write your code here.
	read_bytes = 0;
	int hard_disk = open(HD, O_RDONLY);
	inode* ip = read_inode(hard_disk, i_number);
	
	// check offset out of range
	if(offset > ip->f_size) {
		free(ip);
		close(hard_disk);
		return 0;
	}
	
	// check overflowing original file
	if(offset + count > ip->f_size)
		count = ip->f_size - offset;

	int curr_pos = offset / BLK_SIZE;
	int rem_offset = offset % BLK_SIZE;

	// read direct data blocks
	for(curr_pos; read_bytes<count && curr_pos<2; ++curr_pos) {
		int n;
		if(read_bytes == 0) // check if reading the first block
			n = read_byte(ip->direct_blk[curr_pos], rem_offset,
						  min(BLK_SIZE - rem_offset, count),
						  buf);
		else
			n = read_byte(ip->direct_blk[curr_pos], 0,
						  min(BLK_SIZE, count - read_bytes),
						  buf + read_bytes);
		if(n < 0) {
			printf("Error: direct data blocks\n");
			return -1;
		} else
			read_bytes += n;
	}

	// read indirect data blocks pointers
	int indirect_ptrs[BLK_SIZE / sizeof(int)];
	read_byte(ip->indirect_blk, 0, BLK_SIZE, indirect_ptrs);

	// read indirect data blocks
	for(curr_pos-=2; read_bytes<count; ++curr_pos) {
		int n;
		if(read_bytes == 0) // check if reading the first block
			n = read_byte(indirect_ptrs[curr_pos], rem_offset,
						  min(BLK_SIZE - rem_offset, count),
						  buf);
		else
			n = read_byte(indirect_ptrs[curr_pos], 0,
						  min(BLK_SIZE, count - read_bytes),
						  buf + read_bytes);
		if(n < 0) {
			printf("Error: indirect data blocks\n");
			return -1;
		} else
			read_bytes += n;
	}

	free(ip);
	close(hard_disk);
	return read_bytes; 

}

// you are allowed to create any auxiliary functions that can help your implementation. But only "open_t()" and "read_t()" are allowed to call these auxiliary functions.

/* read_inode() from Lab4 */
inode* read_inode(int fd, int i_number)
{
	inode* ip = malloc(sizeof(inode));
	int currpos = lseek(fd, I_OFFSET + i_number * sizeof(inode), SEEK_SET);
	if(currpos < 0) {
		printf("Error: lseek()\n");
		return NULL;
	}
	
	//read inode from disk
	int ret = read(fd, ip, sizeof(inode));
	if(ret != sizeof(inode)) {
		printf("Error: pread()\n");
		return NULL;
	}
	return ip;
}

int read_byte(int block, int offset, int count, void* buf) {
	int hard_disk = open(HD, O_RDONLY);
	int n = pread(hard_disk, buf, count, D_OFFSET + block * BLK_SIZE + offset);
	close(hard_disk);
	if(n < 0) {
		printf("Error: pread()\n");
		return -1;
	}
	return n;
}

int read_dir_mappings(int fd, int blk_num, DIR_NODE* p_block) {
	// Considering that SFS only supports at most 100 inodes so that only direct_blk[0] will be used,
	// the implementation is much easier
	int ret = pread(fd, p_block, BLK_SIZE, D_OFFSET + blk_num * BLK_SIZE);
	if(ret < 0) {
		printf("Error: pread()\n");
		return -1;
	}
	return ret;
}

int min(int a, int b) {
	return a < b ? a : b ;
}
