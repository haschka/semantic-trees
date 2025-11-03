#include<stdio.h>
#include<stdlib.h>
#include<fcntl.h>
#include<unistd.h>
#include<string.h>

#include <json-c/json.h>

char** extract_texts_from_jsonl_file(char* key, char* file,
				     size_t* n) {

  FILE* f = fopen(file,"r");

  char * line = NULL;
  size_t line_size = 0;

  struct json_object* object_j;
  struct json_object* text_j;

  const char* return_string_buffer;

  char ** texts = NULL;
  size_t n_texts = 0;
  
  size_t return_string_length;
  
  while(-1 != getline(&line, &line_size, f)) {

    object_j = json_tokener_parse(line);
    text_j = json_object_object_get(object_j,key);

    return_string_buffer = json_object_get_string(text_j);

    return_string_length = strlen(return_string_buffer);

    texts = realloc(texts,sizeof(char**)*(n_texts+1));
    texts[n_texts] = (char*)malloc(sizeof(char)*(return_string_length+1));
    memcpy(texts[n_texts],
	   return_string_buffer,
	   sizeof(char)*(return_string_length));
    texts[n_texts][return_string_length] = 0;

    n_texts++;
    if(object_j != NULL) {
      while(json_object_put(object_j) != 1) {
	// free json
      }
    }
  }
  free(line);
  n[0] = n_texts;
  return(texts);
}

char** texts_from_files(char** files, int n_files) {
  int i;
  int fd;
  char fileindex[11];

  char * filename;

  size_t textsize;

  char ** textdb = (char**)malloc(sizeof(char*)*n_files);
  
  for(i=0;i<n_files;i++) {

    fd = open(files[i], O_RDONLY);

    textsize = lseek(fd, 0 ,SEEK_END);
    textdb[i] = (char*)malloc(sizeof(char)*textsize+1);

    lseek(fd,0,SEEK_SET);
    
    if(textsize != read(fd, textdb[i], textsize)) {
      printf("Error reading textfiles at file %s",filename);
      _exit(1);
    }
    textdb[i][textsize]=0;
    close(fd);
  }
  return(textdb);
}

char** cut_texts_in_subtexts(char* text,
			     size_t text_length,
			     size_t requested_length,
			     size_t* n_texts){

  size_t previous_position = 0;
  size_t position = requested_length;
  size_t texts_counter = 0;
  
  char** texts = NULL;
  
  if (text_length < requested_length) {
    texts = (char**)malloc(sizeof(char*));

    texts[0] = (char*)malloc(sizeof(char)*(text_length+1));

    memcpy(texts[0],text,sizeof(char)*text_length);
    texts[0][text_length] = 0;
    n_texts[0] = 1;
    return(texts);
  }

  while(position < text_length - 1) {

    while(text[position] != ' ' && position > previous_position) {
      position--;
    }

    if(position == previous_position) {
      fprintf(stderr, "No new words found on interval at cutting texts");
      position += requested_length;
    }

    texts = (char**)realloc(texts,sizeof(char*)*(texts_counter+1));
    texts[texts_counter] =
      (char*)malloc(sizeof(char)*(position-previous_position+1));
    
    memcpy(texts[texts_counter],
	   text+previous_position,
	   sizeof(char)*position-previous_position);

    texts[texts_counter][position-previous_position] = 0;
    texts_counter++;

    
      
    previous_position = position;
    position += requested_length;
  }

  if(previous_position < text_length && position >= text_length - 1) {

    position = text_length - 1;

    texts = (char**)realloc(texts,sizeof(char*)*(texts_counter+1));
    texts[texts_counter] =
      (char*)malloc(sizeof(char)*(position-previous_position+1));
    
    memcpy(texts[texts_counter],
	   text+previous_position,
	   sizeof(char)*position-previous_position);

    texts[texts_counter][position-previous_position] = 0;
    texts_counter++;
  }

  n_texts[0] = texts_counter; 
  return(texts);
}
      
char** cut_texts_in_subtexts_with_overlap(char* text,
					  size_t text_length,
					  size_t requested_length,
					  size_t * n_texts) {

  size_t n_texts_a, n_texts_b, i;

  size_t requested_length_half = requested_length / 2;

  if (requested_length_half > text_length) {
    fprintf(stderr,
	    "Error: requested text lengths / 2 is > total text length\n");
    _exit(1);
  }
  
  char ** texts_a = cut_texts_in_subtexts(text,
					  text_length,
					  requested_length,
					  &n_texts_a);

  char ** texts_b = cut_texts_in_subtexts(text + requested_length_half,
					  text_length - requested_length_half,
					  requested_length,
					  &n_texts_b);

  n_texts[0] = n_texts_a+n_texts_b;
  
  texts_a = (char**)realloc(texts_a,sizeof(char*)*n_texts[0]);

  for(i=0;i<n_texts_b;i++){
    (texts_a+n_texts_a)[i] = texts_b[i];
  }

  free(texts_b);
  
  return(texts_a);
}
   
    

    
    
      
			     

  
  

  
    
    
