#include<stdio.h>
#include<string.h>
#include<malloc.h>
#include<math.h>
#if defined(__AVX2__)
#include<immintrin.h>
#endif

#include "vector-db.h"
#include "wordlist.h"
#include "load-texts.h"

int main(int argc, char** argv) {

  size_t i,j;
  
  char** buffer;

  size_t slices_lengths;
  
  char* main_text;

  size_t n_slices;

  char** text_slices;
  char** embeddings;

  tokenlist tl;

  wordlist* wl;

  database vdb;

  if(argc < 4) {
    printf("Arguments are: \n"
	   "  [file]  Text to build the vector database from\n"
	   "  [file]  Trained BPE tokenizer\n"
	   "  [file]  Vector DB to create\n"
	   "  [int]   lengths of the slices\n");
    return(1);
  }
  
  buffer = texts_from_files(&(argv[1]), 1);
  main_text = buffer[0];
  
  tl = read_tokenizer_from_file(argv[2]);
  
  sscanf(argv[4],"%li",&slices_lengths);
  
  text_slices = cut_texts_in_subtexts_with_overlap(main_text,
						   strlen(main_text),
						   slices_lengths,
						   &n_slices);

  
  wl = (wordlist*)malloc(sizeof(wordlist)*n_slices);

  embeddings = (char**)malloc(sizeof(char*)*n_slices);
  
  for(i=0;i<n_slices;i++) {
    wl[i] = generate_word_list(text_slices[i]);
    initial_wordlist_tokenization_with_flat_list(wl+i,&tl);
    for(j=0;j<tl.n_rules;j++) {
      apply_merge_rule(wl+i,tl.merge_rules[j]);
    }
    embeddings[i] = binary_embedding(wl[i],tl.n_tokens);
  }

  for(i=0;i<n_slices;i++) {
    free(text_slices[i]);
  }
  free(text_slices);
  
  text_slices = cut_texts_in_subtexts_with_overlap(main_text,
						   strlen(main_text),
						   slices_lengths,
						   &n_slices);

  vdb.vector = embeddings;
  vdb.text = text_slices;
  vdb.type = BINARY_VECTOR;
  vdb.vector_length = tl.n_tokens;
  vdb.n_entries = n_slices;

  write_db_to_disk(argv[3],vdb);

  return(0);
  
}
  
    
  

					    
  
  
  
