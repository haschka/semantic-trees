#include<stdio.h>
#include<stdlib.h>
#include"load-texts.h"
#include"wordlist.h"

int main(int argc, char** argv) {

  size_t i;
  
  char ** text = texts_from_files(&(argv[1]),1);

  wordlist wl = generate_word_list(text[0]);

  
  for(i = 0;i<wl.n_words;i++) {
    printf("\t%li: %s\n",wl.word_counts[i],wl.list[i]);
  }

  return(0);
}
