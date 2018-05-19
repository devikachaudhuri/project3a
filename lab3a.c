//Devika Chaudhuri
//Benjamin Domae

#include "ext2_fs.h"


int imagefd;

int Open(char * pathname, int flags, mode_t mode){
  int fd = open(pathname, flags, mode);
  if (fd == -1){
    fprintf(stderr, "Open error:%s\n", strerror(errno));
  }
  return fd;
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
}


void superblock(){
}


int main (int argc, char *argv[]){
  //argv is the name of the image.
  image fd = Open(argv[1], O_RDONLY);// open the file image
}
  
