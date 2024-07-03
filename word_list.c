#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include "wordlist.h"
#include "binary_array.h"

void free_word_list(wordlist wl) {
  size_t i;
  for(i=0;i<wl.n_words;i++) {
    free(wl.list[i]);
    free(wl.token_idx[i]);
  }
  free(wl.list);
  free(wl.positions);
  free(wl.word_counts);
  free(wl.token_idx);
  free(wl.tokens_per_word);
}

static inline int check_if_char_in_token_list(char p,
					      char** list,
					      size_t list_length) {

  size_t i;
  for(i=0;i<list_length;i++) {
    if( p == list[i][0] ) {
      return 1;
    }
  }
  return 0;
}

static inline void add_word_to_list(char* word,
				    wordlist* wl ){

  size_t word_length=strlen(word);
  
  wl->list = (char**)realloc(wl->list,sizeof(char*)*(wl->n_words+1));
  
  wl->word_counts =
    (size_t*)realloc(wl->word_counts,sizeof(size_t)*(wl->n_words+1));

  wl->word_counts[wl->n_words] = 0;
		     		     
  wl->list[wl->n_words] = (char*)malloc(sizeof(char)*(word_length+1));
  memcpy(wl->list[wl->n_words],word,word_length);
  wl->list[wl->n_words][word_length] = 0;

  wl->n_words++;
  
}
    
static inline size_t check_if_word_is_in_word_list(char* word,
						   wordlist* wl) {
  size_t i;
  for(i=0;i<wl->n_words;i++) {
    if(!strcmp(word,wl->list[i])) {
      wl->word_counts[i]++;
      return i;
    }
  }
  return -1;
}

wordlist generate_word_list(char* data) {

  size_t datalength = strlen(data);

  wordlist wl;
  
  const char space[2] = " ";

  char* word = strtok(data,space);

  size_t i;

  size_t position = 0;

  size_t* word_positions = NULL;
  size_t word_counter = 0;
  
  wl.n_words = 0;
  wl.list = NULL;
  wl.word_counts = NULL;

  while (word != NULL) {
    position = check_if_word_is_in_word_list(word,&wl);
    if(position == -1) {
      position = wl.n_words;
      add_word_to_list(word,&wl);
    }
    if (word_counter % 1000 == 0) {
      word_positions = (size_t*)realloc(word_positions,
					sizeof(size_t)*(word_counter+1000));
    }
    word_positions[word_counter] = position;
    word = strtok(NULL, space);
    word_counter++;
  }

  word_positions = (size_t*)realloc(word_positions,
				    sizeof(size_t)*word_counter);
  
  wl.positions = word_positions;
  wl.n_positions = word_counter;
  
  for(i=0;i<wl.n_words;i++) {
    wl.word_counts[i]++;
  }
  
  return(wl);
}

tokenlist generate_initial_tokenlist(wordlist wl) {

  size_t i,j;
  unsigned char* current_word;
  size_t current_word_length;

  size_t occurencelist[256];

  tokenlist tl;

  size_t counter = 0;

  memset(occurencelist,0,sizeof(size_t)*256);
  
  for(i=0;i<wl.n_words;i++) {
    current_word = wl.list[i];
    current_word_length = strlen(current_word);
    
    for(j=0;j<current_word_length;j++) {
      occurencelist[current_word[j]]++;
    }
  }

  for(i=0;i<256;i++) {
    if(occurencelist[i] > 0) {
      counter++;
    }
  }

  tl.list = (char**)malloc(sizeof(char*)*counter);
  tl.n_tokens = counter;

  for(i=0;i<tl.n_tokens;i++) {
    tl.list[i] = (char*)malloc(sizeof(char)*2);
  }
  
  counter = 0;
  for(i=0;i<256;i++) {
    if(occurencelist[i] > 0) {
      tl.list[counter][0] = i;
      tl.list[counter][1] = 0;
      counter++;
    }
  }
  return(tl);
}

void initial_wordlist_tokenization(wordlist* wl, tokenlist *tl) {

  size_t i,j,k;
  char* flat_tl_list = (char*)malloc(sizeof(char)*tl->n_tokens);

  char* current_word;
  size_t current_word_length;

  wl->token_idx = (int**)malloc(sizeof(int*)*wl->n_words);
  wl->tokens_per_word = (size_t*)malloc(sizeof(size_t)*wl->n_words);
  
  for(i=0;i<tl->n_tokens;i++) {
    flat_tl_list[i] = tl->list[i][0];
  }

  for(i=0;i<wl->n_words;i++) {
    current_word_length = strlen(wl->list[i]);
    current_word = wl->list[i];
    wl->token_idx[i] = (int*)malloc(sizeof(int)*(current_word_length+1));
    wl->tokens_per_word[i] = current_word_length+1;
    for(j=0;j<current_word_length;j++) {
      for(k=0;k<tl->n_tokens;k++) {
	if(current_word[j] == flat_tl_list[k]) {
	  wl->token_idx[i][j] = k;
	  goto next_character;
	}
      }
    next_character:
    }
    /* add word stop token */
    wl->token_idx[i][current_word_length] = tl->n_tokens;
  }

  tl->initial_flat_list = flat_tl_list;
  tl->initial_flat_list_size = tl->n_tokens;

  tl->list = (char**)realloc(tl->list,sizeof(char*)*(tl->n_tokens+2));

  /*add word separater token*/
  tl->list[tl->n_tokens] = (char*)malloc(sizeof(char)*5);
  tl->list[tl->n_tokens][0] = '<';
  tl->list[tl->n_tokens][1] = '/';
  tl->list[tl->n_tokens][2] = 'w';
  tl->list[tl->n_tokens][3] = '>';
  tl->list[tl->n_tokens][4] = 0;

  /*add unkonwn token*/
  tl->list[tl->n_tokens+1] = (char*)malloc(sizeof(char)*6);
  tl->list[tl->n_tokens+1][0] = '<';
  tl->list[tl->n_tokens+1][1] = 'u';
  tl->list[tl->n_tokens+1][2] = 'n';
  tl->list[tl->n_tokens+1][3] = 'k';
  tl->list[tl->n_tokens+1][4] = '>';
  tl->list[tl->n_tokens+1][5] = 0;

  tl->n_tokens+=2;

} 

void initial_wordlist_tokenization_with_flat_list(wordlist* wl, tokenlist* tl){


  size_t i,j,k;
  char* flat_tl_list = tl->initial_flat_list;

  char* current_word;
  size_t current_word_length;

  wl->token_idx = (int**)malloc(sizeof(int*)*wl->n_words);
  wl->tokens_per_word = (size_t*)malloc(sizeof(size_t)*wl->n_words);
      
  for(i=0;i<wl->n_words;i++) {
    current_word_length = strlen(wl->list[i]);
    current_word = wl->list[i];
    wl->token_idx[i] = (int*)malloc(sizeof(int)*(current_word_length+1));
    wl->tokens_per_word[i] = current_word_length+1;
    for(j=0;j<current_word_length;j++) {
      for(k=0;k<tl->initial_flat_list_size;k++) {
	if(current_word[j] == flat_tl_list[k]) {
	  wl->token_idx[i][j] = k;
	  goto next_character;
	}
      }
      wl->token_idx[i][j] = tl->initial_flat_list_size+1;
    next_character:
    }
    /* add word stop token */
    wl->token_idx[i][current_word_length] = tl->initial_flat_list_size;
  }
}
  

size_t sizeof_longest_word(wordlist wl) {

  size_t max_size = 0;
  size_t current_size;

  size_t i;
  
  for(i=0;i<wl.n_words;i++) {
    current_size = strlen(wl.list[i]);
    if (current_size > max_size) {
      max_size = current_size;
    }
  }
  return(max_size);
}

void apply_merge_rule(wordlist* wl, size_t* rule) {

  size_t i,j,k;
  size_t n_tokens_per_word;

  int* this_start;
  
  for(i=0;i<wl->n_words;i++) {
    
    for(j=0;j<wl->tokens_per_word[i]-1;j++) {
      this_start = wl->token_idx[i]+j;

      if(this_start[0] == rule[0] &&
	 this_start[1] == rule[1]) {

	wl->token_idx[i][j] = rule[2];

	/* rotate tokens to fill gap */
	for(k=j+1;k<wl->tokens_per_word[i]-1;k++) {
	  wl->token_idx[i][k] = wl->token_idx[i][k+1];
	}
	wl->tokens_per_word[i]--;
      }
    }
  }
} 

void add_best_pair(wordlist* wl, tokenlist* tl) {

  size_t i,j;
  
  char* tok_a;
  char* tok_b;

  size_t tok_a_len;
  size_t tok_b_len;
  size_t merged_tok_len;
  
  int* this_start;
  
  char* tok_merge;

  int check;

  size_t sum;

  size_t current_max = 0;

  size_t max_i = 0;
  size_t max_j = 0;

  int* current_tokens;
  size_t current_word_counts;
  size_t n_tokens_to_scan;
  
  int* matrix = (int*)malloc(sizeof(int)*tl->n_tokens*tl->n_tokens);

  memset(matrix,0,sizeof(int)*tl->n_tokens*tl->n_tokens);
  
  for(i=0;i<wl->n_words;i++) {
    
    current_word_counts = wl->word_counts[i];
    n_tokens_to_scan = wl->tokens_per_word[i]-1;
    current_tokens = wl->token_idx[i];

    for(j=0;j<n_tokens_to_scan;j++) {
      this_start = current_tokens+j;
      matrix[this_start[1]*tl->n_tokens+this_start[0]]+=current_word_counts;
    }
  }

  for(j=0;j<tl->n_tokens;j++) {
    for(i=0;i<tl->n_tokens;i++) {
      if (matrix[j*tl->n_tokens+i] > current_max) {
	current_max = matrix[j*tl->n_tokens+i];
	max_j = j;
	max_i = i;
      }
    }
  }

  free(matrix);


  tok_a_len = strlen(tl->list[max_i]);
  tok_b_len = strlen(tl->list[max_j]);     
  merged_tok_len = tok_a_len+tok_b_len;
  tok_merge = (char*)malloc(sizeof(char)*(merged_tok_len+1));
  tok_merge[0] = 0;
  strcat(tok_merge,tl->list[max_i]);
  strcat(tok_merge,tl->list[max_j]);

  tl->n_tokens++;
  tl->list = (char**)realloc(tl->list,sizeof(char*)*tl->n_tokens);
  tl->list[tl->n_tokens-1] = tok_merge;

  tl->n_rules++;
  tl->merge_rules =
    (size_t**)realloc(tl->merge_rules,sizeof(size_t*)*tl->n_rules);

  tl->merge_rules[tl->n_rules-1] = (size_t*)malloc(sizeof(size_t)*3);
  tl->merge_rules[tl->n_rules-1][0] = max_i;
  tl->merge_rules[tl->n_rules-1][1] = max_j;
  tl->merge_rules[tl->n_rules-1][2] = tl->n_tokens-1;
}
	
void add_best_pair_old(wordlist* wl, tokenlist* tl) {

  size_t i,j,k,l;

  char* tok_a;
  char* tok_b;

  size_t tok_a_len;
  size_t tok_b_len;
  size_t merged_tok_len;
  
  size_t n_tokens_in_word;

  int* this_start;
  
  char* tok_merge;

  int check;

  size_t sum;

  size_t current_max = 0;

  size_t max_i = 0;
  size_t max_j = 0;
  
  for(i=0;i<tl->n_tokens;i++) {

    for(j=0;j<tl->n_tokens;j++) {

      sum = 0;
      
      for(k=0;k<wl->n_words;k++) {
	n_tokens_in_word = wl->tokens_per_word[k];
	for(l=0;l<n_tokens_in_word-1;l++) {
	  this_start = wl->token_idx[k]+l;
	  if(this_start[0] == i &&
	     this_start[1] == j) {
	    sum+=wl->word_counts[k];
	  }
	}
      }
      
      
      if(sum > current_max) {
	current_max = sum;
	max_j = j;
	max_i = i;
      }
    }
  }

  tok_a_len = strlen(tl->list[max_i]);
  tok_b_len = strlen(tl->list[max_j]);     
  merged_tok_len = tok_a_len+tok_b_len;
  tok_merge = (char*)malloc(sizeof(char)*(merged_tok_len+1));
  tok_merge[0] = 0;
  strcat(tok_merge,tl->list[max_i]);
  strcat(tok_merge,tl->list[max_j]);

  tl->n_tokens++;
  tl->list = (char**)realloc(tl->list,sizeof(char*)*tl->n_tokens);
  tl->list[tl->n_tokens-1] = tok_merge;

  tl->n_rules++;
  tl->merge_rules =
    (size_t**)realloc(tl->merge_rules,sizeof(size_t*)*tl->n_rules);

  tl->merge_rules[tl->n_rules-1] = (size_t*)malloc(sizeof(size_t)*3);
  tl->merge_rules[tl->n_rules-1][0] = max_i;
  tl->merge_rules[tl->n_rules-1][1] = max_j;
  tl->merge_rules[tl->n_rules-1][2] = tl->n_tokens-1;
}
      
tokenlist train_bpe_to_dictsize(wordlist wl, size_t requested_tokens) {

  tokenlist tl = generate_initial_tokenlist(wl);

  initial_wordlist_tokenization(&wl, &tl);
  
  while(tl.n_tokens < requested_tokens) {

    add_best_pair(&wl,&tl);
    apply_merge_rule(&wl,tl.merge_rules[tl.n_rules-1]);

    if(tl.n_tokens % 100 == 0) {
      printf("%li Tokens of %li trained!'\n",tl.n_tokens,requested_tokens);
    }
  }

  return(tl);
}

int save_tokenizer_to_file(char* filename, tokenlist tl) {

  size_t i;
  
  char desc[4] = "BPE";

  size_t current_token_length;
  
  int fd = open(filename,
		O_CREAT | O_WRONLY,
		S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP );

  if(fd == -1) {
    fprintf(stderr,"Opening file to save the trained tokenizer failed!",stderr);
    return(-1);
  }

  write(fd,desc,sizeof(char)*3);
  write(fd,&(tl.initial_flat_list_size),sizeof(size_t));
  write(fd,tl.initial_flat_list,sizeof(char)*tl.initial_flat_list_size);

  write(fd,&(tl.n_tokens),sizeof(size_t));
  for(i=0;i<tl.n_tokens;i++) {
    current_token_length = strlen(tl.list[i]);
    write(fd,&current_token_length,sizeof(size_t));
    write(fd,tl.list[i],sizeof(char)*current_token_length);
  }

  write(fd,&(tl.n_rules),sizeof(size_t));
  for(i=0;i<tl.n_rules;i++) {
    write(fd,tl.merge_rules[i],sizeof(size_t)*3);
  }
}

tokenlist read_tokenizer_from_file(char* filename) {

  size_t i;
  
  char desc[4] = "BPE";

  char *desc_check = (char*)malloc(sizeof(char)*4);
  
  size_t current_token_length;
  
  int fd = open(filename, O_RDONLY );

  tokenlist tl;
  
  if(fd == -1) {
    fprintf(stderr,"Opening file to read the trained tokenizer failed!",stderr);
    _exit(1);
  }

  memset(desc_check,0,sizeof(char)*4);
  
  read(fd,desc_check,sizeof(char)*3);

  desc_check[3] = 0;

  if(0 != strcmp(desc_check,desc)) {
    fprintf(stderr,"BPE file corrupted, magic check failed!",stderr);
    _exit(1);
  }
  
  read(fd,&(tl.initial_flat_list_size),sizeof(size_t));

  tl.initial_flat_list =
    (char*)malloc(sizeof(char)*(tl.initial_flat_list_size+1));

  read(fd,tl.initial_flat_list,sizeof(char)*tl.initial_flat_list_size);
  tl.initial_flat_list[tl.initial_flat_list_size] = 0;
  
  read(fd,&(tl.n_tokens),sizeof(size_t));
  tl.list = (char**)malloc(sizeof(char*)*tl.n_tokens);
  for(i=0;i<tl.n_tokens;i++) {
    read(fd,&current_token_length,sizeof(size_t));
    tl.list[i] = (char*)malloc(sizeof(char)*(current_token_length+1));
    read(fd,tl.list[i],sizeof(char)*current_token_length);
    tl.list[i][current_token_length] = 0;
  }

  read(fd,&(tl.n_rules),sizeof(size_t));
  tl.merge_rules = (size_t**)malloc(sizeof(size_t*)*tl.n_rules);
  
  for(i=0;i<tl.n_rules;i++) {
    tl.merge_rules[i] = (size_t*)malloc(sizeof(size_t)*3);
    read(fd,tl.merge_rules[i],sizeof(size_t)*3);
  }
  return(tl);
}

wordlist tokenize_text(char* text, tokenlist tl) {

  size_t i;
  
  wordlist wl = generate_word_list(text);
  initial_wordlist_tokenization_with_flat_list(&wl,&tl);
  for(i=0;i<tl.n_rules;i++) {
    apply_merge_rule(&wl, tl.merge_rules[i]);
  }
  return(wl);
}
  
void print_tokens(wordlist wl) {

  size_t i,j;
  size_t pos;
  for(i=0;i<wl.n_positions;i++) {
    pos = wl.positions[i];
    for(j=0;j<wl.tokens_per_word[pos];j++) {
      printf("[%i] ",wl.token_idx[pos][j]);
    }
    printf("\n");
  }
}

char* binary_embedding(wordlist wl, size_t dictsize) {

  char* vec = alloc_and_set_zero_binary_array(dictsize);

  char* aligned_vec;

  size_t bin_array_size = dictsize/8+1;
  
  size_t i,j;
  size_t pos;

  size_t final_aligned_size = bin_array_size+(32-bin_array_size%32);
  
  for(i=0;i<wl.n_words;i++) {
    for(j=0;j<wl.tokens_per_word[i];j++) {
      set_value_in_binary_array_at_index(vec,
					 wl.token_idx[i][j]);
    }
  }

  posix_memalign((void**)&aligned_vec,32,final_aligned_size);
  memset(aligned_vec,0,final_aligned_size);
  memcpy(aligned_vec,vec,bin_array_size);
  free(vec);
  
  return(aligned_vec);
}

char* real_embedding(wordlist wl, size_t dictsize) {

  size_t i,j;
  
  double* vec;
  double multiplicator;
  posix_memalign((void**)&vec,32,sizeof(double)*(dictsize+(8-dictsize%8)));

  memset(vec,0,sizeof(double)*(dictsize+(8-dictsize%8)));
  for(i=0;i<wl.n_words;i++) {
    multiplicator = wl.word_counts[i];
    for(j=0;j<wl.tokens_per_word[i];j++) {
      vec[wl.token_idx[i][j]] += multiplicator;
    }
  }
  return((char*)vec);
}
  
  
  
  
      

  


