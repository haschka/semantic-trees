#include<stdio.h>
#include <string.h>
#include <stdlib.h>

#include "vector-db.h"
#include "load-texts.h"
#include "wordlist.h"

int main(int argc, char** argv) {

  size_t i,j;
  
  database vdb;
  char** embeddings;
  char** resumes;

  tokenlist tl;
  wordlist* wl;

  size_t n_resumes;

  char* bpe_filename = argv[2];
  char* vdb_filename = argv[3];
  
  size_t max_len;
  
  if(argc < 4) {
    printf("Arguments are: \n"
	   " [file]     Preprocessed file containing resumes\n"
	   " [file]     Trained BPE tokenizer\n"
	   " [file]     Vector database to write\n");
    
    return(1);
  }

  resumes = extract_texts_from_jsonl_file("resumes",
					  argv[1],&n_resumes);

  tl = read_tokenizer_from_file(argv[2]);

  wl = (wordlist*)malloc(sizeof(wordlist)*n_resumes);
  embeddings = (char**)malloc(sizeof(char*)*n_resumes);
  
  for(i=0;i<n_resumes;i++) {
    wl[i] = generate_word_list(resumes[i]);
    initial_wordlist_tokenization_with_flat_list(wl+i,&tl);
    for(j=0;j<tl.n_rules;j++) {
      apply_merge_rule(wl+i,tl.merge_rules[j]);
    }
    embeddings[i] = binary_embedding(wl[i],tl.n_tokens);
    if(i%1000 == 0) {
      printf("%lu resumes encoded!\n",i);
      fflush(stdout);
    } 
  }

  for(i=0;i<n_resumes;i++) {
    free(resumes[i]);
  }
  free(resumes);
  
  resumes = extract_texts_from_jsonl_file("resumes",argv[1],&n_resumes);

  vdb.vector = embeddings;
  vdb.text = resumes;
  vdb.type = BINARY_VECTOR;
  vdb.vector_length = tl.n_tokens;
  vdb.n_entries = n_resumes;

  write_db_to_disk(vdb_filename,vdb);

  return(0);
}

   
			    
