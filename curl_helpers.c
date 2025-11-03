#include<stdio.h>
#include<stdlib.h>
#include <string.h>
#include <json-c/json.h>
#include"curl_helpers.h"

char* generate_server_query(char* user_query,
			    int n_tokens) {

  struct json_object* new_query = json_object_new_object();

  char* return_string;
  size_t return_length;

  const char* return_string_pointer;
  
  json_object_object_add(new_query,
			 "prompt",
			 json_object_new_string(user_query));
  
  json_object_object_add(new_query,
			 "n_predict",
			 json_object_new_int(n_tokens));

  json_object_object_add(new_query,
			 "stream",
			 json_object_new_boolean(1));

  return_string_pointer = json_object_to_json_string(new_query); 
  return_length = strlen(return_string_pointer);
  
  return_string = (char*)malloc(sizeof(char)*(return_length+1));
  memcpy(return_string,return_string_pointer,return_length);
  return_string[return_length]=0;
  
  return(return_string);
  
}

size_t write_function_callback(char* in_data,
			       size_t size,
			       size_t nmemb,
			       void* clientdata) {

  size_t totalsize = size * nmemb;
  response_data * data = (response_data *)clientdata;

  data->data = (char*)realloc(data->data , data->size + totalsize + 1);
  if(!data->data) {
    return 0;
  }

  memcpy(data->data+(data->size),in_data, totalsize);
  data->size += totalsize;
  data->data[data->size] = 0;

  return(totalsize);
}

size_t write_function_callback_stream_llm(char* in_data,
					  size_t size,
					  size_t nmemb,
					  void* clientdata) {

  response_data * data = (response_data*)clientdata;
  size_t totalsize = size * nmemb;

  struct json_object* response_line;
  struct json_object* content;

  const char* export_string;

  size_t export_string_length;
  
  response_line = json_tokener_parse(in_data+5);

  json_object_object_get_ex(response_line,"content",&content);

  if(content != NULL) {
  
    export_string_length = json_object_get_string_len(content);
    
    export_string = json_object_get_string(content);
    
    fputs(export_string,stdout);
    fflush(stdout);
    
    data->data = (char*)realloc(data->data ,
				data->size + export_string_length + 1);

    memcpy(data->data+data->size,export_string,export_string_length);
    
    data->size += export_string_length;
    data->data[data->size] = 0;
  }

  if(response_line != NULL) {
    while(json_object_put(response_line) != 1) {
      // free json
    }
  }
  return(totalsize);
}
  
  
  
