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

int imagefd;
struct ext2_super_block sblock;
struct ext2_group_desc *gdescriptors;
unsigned char *Inode_bitmap;
unsigned int number_of_groups;
unsigned int block_size;

int Open(char * pathname, int flags){
  int fd = open(pathname, flags);
  if (fd == -1){
    fprintf(stderr, "Open error:%s\n", strerror(errno));
  }
  return fd;
}

ssize_t Pread(int fd, void *buf, size_t count, off_t offset){
  ssize_t p = pread(fd, buf, count, offset);
  if (p == -1){
    fprintf(stderr, "pread error:%s\n", strerror(errno));
  }
  return p;
}

int block_offset(int block){
  return (1024 + ((block-1)*block_size));
}


void indirectblocks(){
}

void directory (){
}

void Inode(){
  struct ext2_inode inodes;
  char type_of_file = '?';
  for (unsigned int j = 0; j < number_of_groups; j++) {
    for (unsigned int k = 2; k < sblock.s_inodes_count; k++) {
      int allocated = 1;
      for (unsigned int i = 0 ; i < block_size; i++) {
	unsigned char bitmap = Inode_bitmap[(block_size *j) + i];
	for (int bit = 0; bit < 8; bit++) {
	  unsigned int inode_num = j * sblock.s_inodes_per_group + i * 8 + bit + 1;
	  if (k == inode_num){
	    if (((bitmap & ( (unsigned int) 1 << bit )) >> bit) == 0){//FREE
	      allocated = 0;
	    }
	    break;
	  }
	}
      }
      if (allocated == 0){
	continue;
      }
      int block = (gdescriptors[j].bg_inode_table);
      int offset = block_offset(block) + (k-1) * sizeof(struct ext2_inode);//add inode table offset to inode offset
      //      printf("%d\n", offset);
      Pread(imagefd, &inodes, sizeof(struct ext2_inode), offset);
     
      if (inodes.i_mode & 0xA000){
	type_of_file = 's';
      }
      else if (inodes.i_mode & 0x8000){
	type_of_file = 'f';
      }
      else if (inodes.i_mode & 0x4000){
	type_of_file = 'd';
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

      printf("INODE,%d,%c,%u,%u,%u,%u,%s,%s,%s,%u,%u",k,type_of_file,inodes.i_mode,inodes.i_uid,inodes.i_gid,inodes.i_links_count,ctime, mtime, atime, inodes.i_size,inodes.i_blocks);

      for (int k = 0; k < EXT2_N_BLOCKS; k++) {
	printf(",%u", inodes.i_block[k]);
      }
      printf("\n");

      if (k == 2) {
	k = sblock.s_first_ino - 1;
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
    
    printf("GROUP,%i,%u,%u,%u,%u,%u,%u,%u\n", i,total_blocks, total_inodes, gdescriptors[i].bg_free_blocks_count, gdescriptors[i].bg_free_inodes_count, gdescriptors[i].bg_block_bitmap, gdescriptors[i].bg_inode_bitmap, gdescriptors[i].bg_inode_table);
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
  imagefd = Open(argv[1], O_RDONLY);// open the file image
  superblock();
  group();
  freeblock();
  freeInode();
  Inode();
}
  
