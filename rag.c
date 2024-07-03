#include <stdio.h>
#include <string.h>
#include <curl/curl.h>

#include <json-c/json.h>

#include <readline/readline.h>

typedef struct {
  char* data;
  size_t size;
} response_data;

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

  return_string_pointer = json_object_to_json_string(new_query); 
  return_length = strlen(return_string_pointer);
  
  return_string = (char*)malloc(sizeof(char)*(return_length+1));
  memcpy(return_string,return_string_pointer,return_length);
  return_string[return_length]=0;
  
  return(return_string);
  
}

static size_t write_function_callback(char* in_data,
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

char* generate_single_instruct_prompt(char* user_input) {

  char system_msg[] ="<|begin_of_text|>"
    "<|start_header_id|>system<|end_header_id|>\n\n"
    "You are a helpful AI assistant answering prompt "
    "to the best of your knowledge\n"
    "<|eot_id|><|start_header_id|>user<|end_header_id|>\n\n";

  char terminator[] = "\n<|eot_id|>\n"
    "<|start_header_id|>assistant<|end_header_id|>\n\n";

  size_t total_length =
    strlen(system_msg)
    +strlen(user_input)
    +strlen(terminator);
  
  char* instruct_prompt = (char*)malloc(sizeof(char)*(total_length+1));

  instruct_prompt[0] = 0;

  strcat(instruct_prompt,system_msg);
  strcat(instruct_prompt,user_input);
  strcat(instruct_prompt,terminator);

  return(instruct_prompt);
} 

char* update_conversation_only_prompt(char* user_input,
				      char* machine_response,
				      char* prompt) {

  size_t current_prompt_length;
  size_t user_input_length;
  size_t machine_response_length;

  size_t user_tag_length;
  size_t terminator_length;

  size_t total_length;
  
  char user_tag[] = "<|eot_id|><|start_header_id|>user<|end_header_id|>\n\n";
  char terminator[] = "\n<|eot_id|>\n"
    "<|start_header_id|>assistant<|end_header_id|>\n\n";

  
  if (machine_response == NULL) {
    prompt = generate_single_instruct_prompt(user_input);
    return(prompt);
  }

  current_prompt_length = strlen(prompt);
  user_input_length = strlen(user_input);
  machine_response_length = strlen(machine_response);

  terminator_length = strlen(terminator);
  user_tag_length = strlen(user_tag);

  total_length = current_prompt_length
    +user_input_length
    +machine_response_length
    +terminator_length
    +user_tag_length;
  
  prompt = (char*)realloc(prompt,sizeof(char)*(total_length+1));
  strcat(prompt, machine_response);
  strcat(prompt, user_tag);
  strcat(prompt, user_input);
  strcat(prompt, terminator);

  return(prompt);
}

char* generate_llm_answer_from_full_response(char* response) {

  struct json_object *response_j = json_tokener_parse(response);
  struct json_object *content;

  char* response_string;
  size_t response_string_length;

  const char* export_string;
  
  if(response_j == NULL) {
    fprintf(stderr, "Full JSON reponse from LLM is not a valid json_object\n");
    return(NULL);
  }

  json_object_object_get_ex(response_j,"content",&content);

  response_string_length = json_object_get_string_len(content);

  export_string = json_object_get_string(content);
  
  response_string = (char*)malloc(sizeof(char)*(response_string_length+1));

  memcpy(response_string,
	 export_string,
	 sizeof(char)*(response_string_length));

  response_string[response_string_length] = 0;

  return(response_string);
}
  

int main(int argc, char** argv) {

  char* prompt = NULL;
  char* query_string = NULL;
  char* response = NULL;

  char* iobuffer;
  
  char* llm_answer;
  
  CURL *curl = curl_easy_init();

  CURLcode result;
  
  response_data rd;

  int terminate = 0;
  
  rd.size = 0;
  rd.data = NULL;

  if(!curl) {
    fprintf(stderr,"Failed to initialize CURL, EXITING\n");
    return(1);
  }
 
  while(!terminate) {

    
    iobuffer = readline("\033[33;1m Human > \033[0m");

    if (NULL == iobuffer) {
      terminate = 1;
      goto exit_repl;
    }

    prompt = update_conversation_only_prompt(iobuffer,
					     response,
					     prompt);

    if(response != NULL) {
      free(response);
    }
    free(iobuffer);
    
    query_string = generate_server_query(prompt, 128);
    
    if (rd.data != NULL) {
      free(rd.data);
      rd.data = NULL;
      rd.size = 0;
    }
  
    curl_easy_setopt(curl, CURLOPT_URL,
		     "http://192.168.1.104:8080/completion");
    
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE,(long)strlen(query_string)); 
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, query_string);
    
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_function_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,(void*)&rd);

    result = curl_easy_perform(curl);
    
    response = generate_llm_answer_from_full_response(rd.data);
    fputs("\033[33;1m Machine > \033[0m", stdout);
    fputs(response,stdout);
    fputs("\n",stdout);
    
  }
 exit_repl:
  curl_easy_cleanup(curl);
}
  

		 
  
