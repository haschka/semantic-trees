#include<stdio.h>
#include<fcntl.h>
#include<unistd.h>

char** texts_from_files(char* prefix, int n_files) {
  int i;
  int fd;
  char fileindex[11];

  size_t prefix_len = strnlen(prefix,PATH_MAX-11);

  char * filename = (char*)malloc(sizeof(char)*prefix_len+11);
  memcpy(filename,prefix,sizeof(char)*(prefix_len+1));

  size_t textsize;

  char ** textdb = (char**)malloc(sizeof(char*)*n_files);
  
  for(i=0;i<n_files;i++) {

    sprintf(fileindex,"%10d",i);
    strcat(filename,fileindex);

    fd = open(filename, O_RDONLY);

    textsize = lseek(fd, 0 ,SEEK_END);
    textdb[i] = (char*)malloc(sizeof(char)*textsize+1);

    if(textsize != read(fd, textdb[i], textsize)) {
      printf("Error reading textfiles at file %s",filename);
      _exit(1);
    }
    textdb[i][textsize]=0;
    close(fd);
  }
  return(textdb);
}
    
    
    
    
