//Devika Chaudhuri
//Benjamin Domae

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include "ext2_fs.h"
#include <stdlib.h>
#include <time.h>

#define EXIT_SUC 0
#define EXIT_ERR_ARG 1
#define EXIT_ERR_MISC 2

int imagefd;
struct ext2_super_block sblock;
struct ext2_group_desc *gdescriptors;
unsigned char *Inode_bitmap;
unsigned int number_of_groups;
unsigned int block_size;

int Open(char * pathname, int flags){
  int fd = open(pathname, flags);
  if (fd == -1){
    fprintf(stderr, "Open error: %s\n", strerror(errno));
    exit(EXIT_ERR_ARG);
  }
  return fd;
}

ssize_t Pread(int fd, void *buf, size_t count, off_t offset){
  ssize_t p = pread(fd, buf, count, offset);
  if (p == -1){
    fprintf(stderr, "pread error: %s\n", strerror(errno));
    exit(EXIT_ERR_MISC);
  }
  return p;
}

int block_offset(int block){
  return (1024 + ((block-1)*block_size));
}


void indirectblocks(){
}

void directory (struct ext2_inode* dir_inode, unsigned int dir_inode_num){
  unsigned int offset_tot = 0;
  unsigned int block_index = 0;
  unsigned int next_name_len;
  unsigned int block_ptr = dir_inode->i_block[block_index];
  struct ext2_dir_entry current_dir_entry;

  // Grab the first entry
  Pread(imagefd, &current_dir_entry, sizeof(struct ext2_dir_entry), 
	(block_offset(block_ptr) + offset_tot));

  // Loop through all entries in the directory (until a entry with inode number 0)
  do {
    // Print the log entry
    printf("DIRENT,%u,%d,%u,%u,%u,\'%s\'\n",
	   dir_inode_num,
	   offset_tot,
	   current_dir_entry.inode,
	   current_dir_entry.rec_len,
	   current_dir_entry.name_len,
	   current_dir_entry.name);

    // Setup for the next entry
    offset_tot += current_dir_entry.rec_len;
    if (offset_tot >= block_size){
      // If this block is used up, go to the next one
      block_index++;
      block_ptr = dir_inode->i_block[block_index];
    }

    // Grab the next entry
    Pread(imagefd, &current_dir_entry, sizeof(struct ext2_dir_entry), 
	  (block_offset(block_ptr) + offset_tot));
    next_name_len = current_dir_entry.name_len;
  } while (next_name_len > 0);
}

void Inode(){
  struct ext2_inode inodes;
  char type_of_file = '?';
  int run_dir = 0;
  unsigned int dir_par_inode_num;
  for (unsigned int j = 0; j < number_of_groups; j++) {
    for (unsigned int k = 2; k < sblock.s_inodes_count; k++) {      
      int block = (gdescriptors[j].bg_inode_table);
      int offset = block_offset(block) + (k-1) * sizeof(struct ext2_inode);//add inode table offset to inode offset
      //      printf("%d\n", offset);
      Pread(imagefd, &inodes, sizeof(struct ext2_inode), offset);
      if (inodes.i_mode != 0 && inodes.i_links_count != 0){
	  
	if (S_ISREG(inodes.i_mode)){
	    type_of_file = 'f';
	  }
	if (S_ISLNK(inodes.i_mode)){
	  type_of_file = 's';
	}
	else if (S_ISDIR(inodes.i_mode)){
	  type_of_file = 'd';
	  // Run the directory function too
	  run_dir = 1;
	}
	
	char ctime[80];
	time_t rawtime = inodes.i_ctime;
	struct tm *info = gmtime(&rawtime);
	strftime(ctime, 80, "%x %X", info);
	  
	char mtime[80];
	time_t rawtime1 = inodes.i_mtime;
	struct tm *info1 = gmtime(&rawtime1);
	strftime(mtime, 80, "%x %X", info1);
	
	char atime[80];
	time_t rawtime2 = inodes.i_atime;
	struct tm *info2 = gmtime(&rawtime2);
	strftime(atime, 80, "%x %X", info2);  
	
	dir_par_inode_num = k;
	printf("INODE,%d,%c,%o,%u,%u,%u,%s,%s,%s,%u,%u",
	       k,
	       type_of_file,
	       (inodes.i_mode & 0x0fff),
	       inodes.i_uid,
	       inodes.i_gid,
	       inodes.i_links_count,
	       ctime, 
	       mtime, 
	       atime, 
	       inodes.i_size,
	       inodes.i_blocks);
	if (type_of_file != 's' || (type_of_file == 's' && inodes.i_size > 60)){
	  for (int k = 0; k < EXT2_N_BLOCKS; k++){
	    printf(",%u", inodes.i_block[k]);
	  }
	}
	else{
	  printf(",%u", inodes.i_block[0]);
	}
	printf("\n");
	
	if (k == 2) {
	  k = sblock.s_first_ino - 1;
	}
	
	if (run_dir) {
	  // If indicated, run the directory function
	  //	    directory(&inodes, dir_par_inode_num);
	  run_dir = 0;
	}
      }
    }
  }
}

	
    

void freeInode(){
  //same as freeblock except +1 block over.
  
  unsigned char b;
  Inode_bitmap = malloc(number_of_groups * block_size * sizeof(unsigned char));//for Inodes();
  for(unsigned int i = 0; i < number_of_groups; i++){
    int offset2 = (i * sblock.s_inodes_per_group);
    for (unsigned int k = 0; k < block_size; k ++){
      //parse the bitmap byte by byte
      int offset = (block_size * (gdescriptors[i].bg_block_bitmap +1)) + k;
      Pread(imagefd, &b, sizeof(unsigned char), offset);
      Inode_bitmap[(block_size*i) + k] = b;//stored in 2D array of [group][#byte], so the third byte of group 2 would be [block_size*1 + 2]
      for (unsigned int j = 0; j < 8; j++){
	//for every block per group, look at the corresponding bit
	if ( ((b & ( (unsigned int) 1 << j )) >> j) == 0){//FREE
	  printf("IFREE,%u\n", (offset2 + (k * 8) + j + 1));
	}
      }
    }
  }
}

void freeblock(){
  //Each bit represents the current state of a block within that block group,
  //where 1 means "used" and 0 "free/available". The first block of this block
  //group is represented by bit 0 of byte 0, the second by bit 1 of byte 0.
  //The 8th block is represented by bit 7 (most significant bit) of byte 0 while
  //the 9th block is represented by bit 0 (least significant bit) of byte 1.

  //Two blocks near the start of each group are reserved for the block usage
  // bitmap and the inode usage bitmap which show which blocks and inodes
  //are in use. Since each bitmap is limited to a single block, this means
  //that the maximum size of a block group is 8 times the size of a block. 
  unsigned char b;
  
  for(unsigned int i = 0; i < number_of_groups; i++){
    int offset2 = i * sblock.s_blocks_per_group;
    for (unsigned int k = 0; k < block_size; k ++){
      //parse the bitmap byte by byte
      int offset = (block_size * gdescriptors[i].bg_block_bitmap) + k;
      Pread(imagefd, &b, sizeof(unsigned char), offset);
      for (unsigned int j = 0; j < 8; j++){
	//for every block per group, look at the corresponding bit
	if ( ((b & ( (unsigned int) 1 << j )) >> j) == 0){//FREE
	  printf("BFREE,%u\n", (offset2 + (k * 8) + j + 1));
	}
      }
    }
  }  
}

void group(){
  number_of_groups = 1 + (sblock.s_blocks_count - 1)/sblock.s_blocks_per_group;
  printf("number of groups %d\n", number_of_groups);
  //(totalnumber of blocks -1)/blocks per group
  //first group is at the offset 1024 + block_size
  //second group is at the offset 1024 + block_size + (block_size * blocks_per_group)
  //third group is at the offset 1024 + block_size+ 2(block_size * blockes_per_group)
  gdescriptors = (struct ext2_group_desc*) malloc(number_of_groups * sizeof(struct ext2_group_desc));


  unsigned int total_left = sblock.s_blocks_count;
  unsigned int blocks_per_group = sblock.s_blocks_per_group;
  unsigned int total_blocks;
  for (unsigned int i = 0; i< number_of_groups; i++){
    int offset = 1024 + block_size + (block_size * i * sblock.s_blocks_per_group);
    Pread(imagefd, &gdescriptors[0], sizeof(struct ext2_group_desc), offset);
    if (total_left < blocks_per_group){
      total_blocks = total_left;
    }
    else
      total_blocks = sblock.s_blocks_per_group;

    unsigned int total_inodes = sblock.s_inodes_per_group;
    
    printf("GROUP,%i,%u,%u,%u,%u,%u,%u,%u\n", 
	   i,
	   total_blocks, 
	   total_inodes, 
	   gdescriptors[i].bg_free_blocks_count, 
	   gdescriptors[i].bg_free_inodes_count, 
	   gdescriptors[i].bg_block_bitmap, 
	   gdescriptors[i].bg_inode_bitmap, 
	   gdescriptors[i].bg_inode_table);
    total_left = total_left - total_blocks;
  }
}


void superblock(){
  //The superblock is always located at byte offset 1024 from the beginning of the file, block device or partition formatted with Ext2 and later variants (Ext3, Ext4).
  Pread(imagefd, &sblock, sizeof(struct ext2_super_block), 1024);
  block_size = 1024 << sblock.s_log_block_size;
  printf("SUPERBLOCK,%u,%u,%u,%u,%u,%u,%u\n", sblock.s_blocks_count, sblock.s_inodes_count, block_size, sblock.s_inode_size, sblock.s_blocks_per_group, sblock.s_inodes_per_group, sblock.s_first_ino);  
}


int main (int argc, char *argv[]){
  //argv is the name of the image.
  if(argc < 2){
    fprintf(stderr, "Error: No file system image provided!\n");
    exit(EXIT_ERR_ARG);
  }
  imagefd = Open(argv[1], O_RDONLY);// open the file image
  superblock();
  group();
  freeblock();
  freeInode();
  Inode();
}
  
