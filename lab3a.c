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

int imagefd;
struct ext2_super_block sblock;
struct ext2_group_desc *gdescriptors;

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


void indirectblocks(){
}

void directory (){
}

void Inode(){
}

void freeInode(){
}

void freeblock(){
}


void group(){
  int number_of_groups = 1 + (sblock.s_blocks_count - 1)/sblock.s_blocks_per_group;
  printf("number of groups %d\n", number_of_groups);
//(totalnumber of blocks -1)/blocks per group
  int block_size = 1024 << sblock.s_log_block_size;
  //first group is at the offset 1024 + block_size
  //second group is at the offset 1024 + block_size + (block_size * blocks_per_group)
  //third group is at the offset 1024 + block_size+ 2(block_size * blockes_per_group)
  gdescriptors = (struct ext2_group_desc*) malloc(number_of_groups * sizeof(struct ext2_group_desc));
  
  for (int i = 0; i< number_of_groups; i++){
    int offset = 1024 + block_size + (block_size * i * sblock.s_blocks_per_group);
    Pread(imagefd, &gdescriptors[0], sizeof(struct ext2_group_desc), offset);

    unsigned int total_blocks = sblock.s_blocks_per_group; //TODO may have more...
    unsigned int total_inodes = sblock.s_inodes_per_group;
    
    printf("GROUP,%i,%u,%u,%u,%u,%u,%u,%u\n", i,total_blocks, total_inodes, gdescriptors[i].bg_free_blocks_count, gdescriptors[i].bg_free_inodes_count, gdescriptors[i].bg_block_bitmap, gdescriptors[i].bg_inode_bitmap, gdescriptors[i].bg_inode_table);
  }

}


void superblock(){
  //The superblock is always located at byte offset 1024 from the beginning of the file, block device or partition formatted with Ext2 and later variants (Ext3, Ext4).
  Pread(imagefd, &sblock, sizeof(struct ext2_super_block), 1024);
  unsigned int block_size = 1024 << sblock.s_log_block_size;
  printf("SUPERBLOCK,%u,%u,%u,%u,%u,%u,%u\n", sblock.s_blocks_count, sblock.s_inodes_count, block_size, sblock.s_inode_size, sblock.s_blocks_per_group, sblock.s_inodes_per_group, sblock.s_first_ino);  
}


int main (int argc, char *argv[]){
  //argv is the name of the image.
  imagefd = Open(argv[1], O_RDONLY);// open the file image
  superblock();
  group();
}
  
