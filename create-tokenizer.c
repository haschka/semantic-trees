#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include "wordlist.h"

int main(int argc,char** argv) {

  int fd = open(argv[1],O_RDONLY);

  size_t dict_size;
  
  off_t file_size;

  char * training_text;

  wordlist wl;
  tokenlist tl;

  if(argc < 3) {
    printf("Arguments are:\n"
	   "  [file]    text to train the tokenizer on\n"
	   "  [file]    file to save the trained tokenizer to\n"
	   "  [integer] dictonairy size\n");
    return(1);
  }

  if(fd == -1) {
    fprintf(stderr,"Error: Failed to Open Textinput File\n");
  }

  if (1 != sscanf(argv[3],"%li",&dict_size)) {
    fprintf(stderr,"Error: could not get dictionary size from input\n");
  }
  
  file_size = lseek(fd,0,SEEK_END);
  lseek(fd,0,SEEK_SET);

  training_text = (char*)malloc(sizeof(char)*(file_size+1));

  read(fd,training_text,file_size);
  training_text[file_size] = 0;
  
  wl = generate_word_list(training_text);
  printf("Finished generating wordlist\n");
  tl = train_bpe_to_dictsize(wl,dict_size);
  printf("BPE tokenizer trained! Saving!\n");
  save_tokenizer_to_file(argv[2],tl);
  return(0);
}
  
    
  

  
