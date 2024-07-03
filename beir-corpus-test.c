#include <stdio.h>
#include <string.h>

#include <json-c/json_object.h>
#include <json-c/json.h>

#include "vector-db.h"
#include "wordlist.h"

typedef struct {
  char* _id;
  char* text;
} beir_line;

typedef struct {
  database db;
  char** _id;
} extended_database;

typedef struct {
  char* question;
  char** answer;
  size_t n_answers;
  int* score;
} qrel;

typedef struct {
  qrel* rel;
  size_t n_relations;
} qrels;
  
beir_line distill_json_line_to_beir(char* line) {

  beir_line b;
  struct json_object* beir_j = json_tokener_parse(line);

  struct json_object* _id_j;
  struct json_object* text_j;

  const char* export_string;
  size_t text_length;
  
  json_object_object_get_ex(beir_j,"_id",&_id_j);
  json_object_object_get_ex(beir_j,"text",&text_j);

  text_length = json_object_get_string_len(_id_j);
  b._id = (char*)malloc(sizeof(char)*(text_length+1));
  export_string = json_object_get_string(_id_j);
  memcpy(b._id,export_string,sizeof(char)*text_length);
  b._id[text_length] = 0;
  
  text_length = json_object_get_string_len(text_j);
  b.text = (char*)malloc(sizeof(char)*(text_length+1));
  export_string = json_object_get_string(text_j);
  memcpy(b.text,export_string,sizeof(char)*text_length);
  b.text[text_length] = 0;

  return(b);
}

extended_database edb_from_bier(char* bier_file, char* tokenizer_file) {

  size_t i,counter;
  char * line = NULL;
  size_t line_size = 0;

  FILE* f = fopen(bier_file,"r");

  tokenlist tl = read_tokenizer_from_file(tokenizer_file);
  wordlist wl;

  beir_line bl;
  
  char** embeddings = NULL;
  char** ids = NULL;
  char** texts = NULL;

  database vdb;

  char* word_list_buffer = NULL;
  size_t word_list_buffer_size = 0;
  size_t current_text_length;
  
  extended_database edb;
  
  counter = 0;
  
  while (-1 != getline(&line, &line_size, f)) {
    bl = distill_json_line_to_beir(line);
    current_text_length = strlen(bl.text);
    if(current_text_length+1 > word_list_buffer_size) {
      word_list_buffer =
	(char*)realloc(word_list_buffer,sizeof(char)*current_text_length+1);
      word_list_buffer_size = current_text_length+1;
    }
    memcpy(word_list_buffer,bl.text,current_text_length+1);
    wl = generate_word_list(word_list_buffer);
    initial_wordlist_tokenization_with_flat_list(&wl,&tl);
    for(i=0;i<tl.n_rules;i++) {
      apply_merge_rule(&wl,tl.merge_rules[i]);
    }
    
    embeddings = (char**)realloc(embeddings,sizeof(char*)*(counter+1));
    embeddings[counter] = binary_embedding(wl,tl.n_tokens);

    ids = (char**)realloc(ids,sizeof(char*)*(counter+1));
    ids[counter] = bl._id;

    texts = (char**)realloc(texts,sizeof(char*)*(counter+1));
    texts[counter] = bl.text;

    free_word_list(wl);
    counter++;
  }

  if(word_list_buffer != NULL) {
    free(word_list_buffer);
  }
  
  vdb.vector = embeddings;
  vdb.text = texts;
  vdb.type = BINARY_VECTOR;
  vdb.vector_length = tl.n_tokens;
  vdb.n_entries = counter;

  edb.db = vdb;
  edb._id = ids;

  return(edb);
}

long is_relation_in_rels(qrel* relations,char* question, size_t n_relations) {

  long i;
  
  for(i=0;i<n_relations;i++) {
    if (strcmp(relations[i].question,question) == 0)  return(i);
  }
  return(-1);
  
}
  
qrels read_beir_relations_table(char* filename) {

  size_t i;
  
  FILE* f = fopen(filename,"r");

  char* line = NULL;
  size_t linesize = 0;

  size_t end;
  size_t pos[2];

  size_t pos_count, rel_count;

  char* current_question = NULL;
  size_t current_question_length = 0;

  long which_relation;
  qrel* this_relation;
  
  qrels relations;
  relations.rel = NULL;
  relations.n_relations = 0;

  rel_count = 0;

  /* skip first line */
  getline(&line,&linesize,f);
  
  while(-1 != getline(&line, &linesize,f)) {

    pos_count = 0;
    for(i=0;i<linesize;i++) {
      if(line[i] == '\t' && pos_count < 2) {
	pos[pos_count] = i;
	pos_count++;
      }
    }

    if(current_question_length < pos[0]+1) {
      current_question_length = pos[0]+1;
      current_question = (char*)realloc(current_question,
					sizeof(char)*current_question_length);
    }
    
    memcpy(current_question,line,pos[0]);
    current_question[pos[0]]=0;

    which_relation = is_relation_in_rels(relations.rel,
					 current_question,
					 relations.n_relations);
    
    if (-1 == which_relation) {
      /* relation is not in relations.rel*/  

      relations.rel =
	(qrel*)realloc(relations.rel,
		       sizeof(qrel)*(relations.n_relations+1));

      this_relation = relations.rel+relations.n_relations;
      
      this_relation->question =
	(char*)malloc(sizeof(char)*(pos[0]+1));
      
      this_relation->answer =
	(char**)malloc(sizeof(char*));
      
      this_relation->answer[0] =
	(char*)malloc(sizeof(char)*(pos[1]-pos[0]));
      
      memcpy(this_relation->question,
	     line,pos[0]);
      memcpy(this_relation->answer[0],
	     line+pos[0]+1,pos[1]-pos[0]-1);
      
      this_relation->question[pos[0]] = 0;
      this_relation->answer[0][pos[1]-pos[0]-1] = 0;
      
      this_relation->score =
	(int*)malloc(sizeof(int));
      
      sscanf(line+pos[1],"%i",this_relation->score);
      
      this_relation->n_answers = 1;
      
      relations.n_relations++;

    } else {
      /*relation is in relations.rel*/

      this_relation = relations.rel+which_relation;
      this_relation->answer =
	(char**)realloc(this_relation->answer,
			sizeof(char*)*(this_relation->n_answers+1));

      this_relation->answer[this_relation->n_answers] =
	(char*)malloc(sizeof(char)*(pos[1]-pos[0]));

      memcpy(this_relation->answer[this_relation->n_answers],
	     line+pos[0]+1,pos[1]-pos[0]-1);

      this_relation->answer[this_relation->n_answers][pos[1]-pos[0]-1] = 0;

      this_relation->score =
	(int*)realloc(this_relation->score,
		      sizeof(int)*(this_relation->n_answers+1));
      
      sscanf(line+pos[1],"%i",this_relation->score+this_relation->n_answers);

      this_relation->n_answers++;
    }
    /* end of while loop, reading relations file */
  }

  if(current_question != NULL) {
    free(current_question);
  }
  
  return(relations);
}

int comp_qrel(const void* a, const void* b) {

  qrel* a_q = (qrel*)a;
  qrel* b_q = (qrel*)b;

  return(strcmp(a_q->question,b_q->question));
}

long binarySearch_qrel(qrels* relations, char* query) {

  long low = 0;
  long high = relations->n_relations-1;
  long mid;

  int cmp;
  
  while(low <= high) {
    mid = low + ( high - low ) / 2;

    cmp = strcmp(relations->rel[mid].question,query);

    if (cmp == 0) return mid;
    if (cmp < 0) low = mid + 1;
    if (cmp > 0) high = mid - 1;
  }
  return(-1);
}

static inline void sort_qrels(qrels* relations) {
  qsort(relations->rel,relations->n_relations,sizeof(qrel),comp_qrel);
}
 
int main(int argc, char** argv) {

  size_t i,j;
  
  char* beir_questions = argv[1];
  char* beir_corpus = argv[2];
  char* beir_relations = argv[3];

  char* tokenizer = argv[4];

  size_t* closest_distance;
  size_t relation_index;

  extended_database edb_queries;
  extended_database edb_corpus;
  qrels relations;
  
  long score;
  long totalscore;
  long highest_score;

  qrel* this_relation;

  if(argc < 4) {
    printf("Arguments are: \n"
	   "  [file] BEIR jsonl file containing questions \n"
	   "  [file] BEIR jsonl file containing the corpus \n"
	   "  [file] BEIR qrels question - answer relations \n"
	   "  [file] trained BPE tokenizer \n");
    return(1);
  }
  
  edb_queries = edb_from_bier(beir_questions, tokenizer);
  edb_corpus = edb_from_bier(beir_corpus, tokenizer);

  relations = read_beir_relations_table(beir_relations);

  sort_qrels(&relations);

  totalscore = score = 0.;

  for(i = 0; i < edb_queries.db.n_entries ; i++) {

    closest_distance = create_closest_distances(edb_corpus.db,
						Yule_distance,
						edb_queries.db.vector[i],2);

    relation_index = binarySearch_qrel(&relations, edb_queries._id[i]);

    if (relation_index != -1) {
      
      this_relation = relations.rel+relation_index;
      
      highest_score = 0;
      for(j=0;j<this_relation->n_answers;j++) {
	if ( this_relation->score[j] > highest_score ) {
	  highest_score = this_relation->score[j];
	}
      }
      totalscore += highest_score;
      
      if ( 0 == strcmp(edb_queries._id[i],
		       edb_corpus._id[closest_distance[0]])) {
	
	/* retrieved question is identical in corpus, skip first
	   retrieved answer */
	for(j=0;j<this_relation->n_answers;j++) {
	  if (  0 == strcmp(this_relation->answer[j],
			    edb_corpus._id[closest_distance[1]]) ) {
	    
	    score += this_relation->score[j];
	  }
	}
      }
    }
  }

  printf("QUERY ACCURACY: %10.5lf\n", (double)score/(double)totalscore);

  return(0);
}
    
    
  
  
  
  


    

  
    
      
  
  
  
      
