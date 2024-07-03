typedef struct {
  char** list;
  size_t* word_counts;
  size_t* tokens_per_word;
  size_t* positions;
  size_t n_positions;
  int** token_idx;
  size_t n_words;
} wordlist;

typedef struct {
  char* initial_flat_list;
  size_t initial_flat_list_size;
  char** list;
  size_t n_tokens;
  /* first and second index to merge to third: i.e. [0],[1] -> [2] */
  size_t **merge_rules;
  size_t n_rules;
} tokenlist;

wordlist generate_word_list(char* data);
int save_tokenizer_to_file(char* filename, tokenlist tl);
tokenlist read_tokenizer_from_file(char* filename);
tokenlist train_bpe_to_dictsize(wordlist wl, size_t requested_tokens);
void initial_wordlist_tokenization_with_flat_list(wordlist* wl, tokenlist* tl);
void apply_merge_rule(wordlist* wl, size_t* rule);
char* real_embedding(wordlist wl, size_t dictsize);
char* binary_embedding(wordlist wl, size_t dictsize);

void free_word_list(wordlist wl);
