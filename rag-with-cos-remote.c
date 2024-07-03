#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <curl/curl.h>

#include <json-c/json.h>

#include <readline/readline.h>

#include "vector-db.h"
#include "wordlist.h"
#include "local_resolve.h"
#include "curl_helpers.h"
      
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

char *generate_instruct_prompt_with_db(char *user_input,
				       char **conversation,
				       size_t* conversation_length,
				       database vdb,
				       tokenlist tl,
				       size_t n_db_results) {

  size_t i;
  char* embeddings;
  size_t* closest_results;

  char* user_input_for_wordlist;
  wordlist wl;

  char context_count_header[20];
  
  char system_msg_start[] = "<|begin_of_text|>"
    "<|start_header_id|>system<|end_header_id|>\n\n"
    "You are a helpful AI assistant answering prompt "
    "takeing the following \n"
    "contexts into account in you answer as good as you can \n"; 

  char user_msg_start[] =
    "<|eot_id|><|start_header_id|>user<|end_header_id|>\n\n";

  char assistent_msg_start[] =
    "<|eot_id|><|start_header_id|>assistent<|end_header_id|>\n\n";

  char new_line[] = " \n ";
  
  size_t system_msg_start_len = strlen(system_msg_start);

  size_t current_vdb_result_len;
  size_t current_prompt_len;
  
  char* final_prompt = NULL;

  user_input_for_wordlist = (char*)malloc(sizeof(char)*(strlen(user_input)+1));
  memcpy(user_input_for_wordlist,user_input,sizeof(char)*(strlen(user_input)));
  user_input_for_wordlist[strlen(user_input)] = 0;
  	 
  wl = generate_word_list(user_input_for_wordlist);
  initial_wordlist_tokenization_with_flat_list(&wl,&tl);
  
  for(i=0;i<tl.n_rules;i++) {
    apply_merge_rule(&wl,tl.merge_rules[i]);
  }

  free(user_input_for_wordlist);
  
  embeddings = binary_embedding(wl,tl.n_tokens);

  closest_results =
    create_closest_distances(vdb,
			     Yule_distance,
			     embeddings,
			     n_db_results);

  final_prompt =
    (char*)realloc(final_prompt,sizeof(char)*(system_msg_start_len+1));

  memcpy(final_prompt,system_msg_start,sizeof(char)*system_msg_start_len);
  final_prompt[system_msg_start_len] = 0;

  current_prompt_len = system_msg_start_len;
  
  for(i=0;i<n_db_results;i++) {
    sprintf(context_count_header,"Context %li:\n",i+1);
    current_prompt_len += strlen(context_count_header);
    final_prompt =
      (char*)realloc(final_prompt,sizeof(char)*(current_prompt_len+1));
    strcat(final_prompt, context_count_header);

    current_prompt_len += strlen(vdb.text[closest_results[i]]);
    final_prompt =
      (char*)realloc(final_prompt,sizeof(char)*(current_prompt_len+1));

    strcat(final_prompt, vdb.text[closest_results[i]]);

    current_prompt_len += strlen(new_line);
    final_prompt =
      (char*)realloc(final_prompt,sizeof(char)*(current_prompt_len+1));
    strcat(final_prompt, new_line);
  }

  /* adding tokens to start a new user message to conversation */
  conversation_length[0] += strlen(user_msg_start);
  conversation[0] =
    (char*)realloc(conversation[0],sizeof(char)*(conversation_length[0]+1));
  strcat(conversation[0],user_msg_start);

  /* adding new user message to conversation */
  conversation_length[0] += strlen(user_input);
  conversation[0] =
    (char*)realloc(conversation[0],sizeof(char)*(conversation_length[0]+1));
  strcat(conversation[0],user_input);

  /* adding the start message for the assistents following generated answer */
  conversation_length[0] += strlen(assistent_msg_start);
  conversation[0] =
    (char*)realloc(conversation[0],sizeof(char)*(conversation_length[0]+1));
  strcat(conversation[0],assistent_msg_start);

  /* build new prompt from past conversation */
  current_prompt_len += conversation_length[0];
  final_prompt =
    (char*)realloc(final_prompt,sizeof(char)*(current_prompt_len+1));
  strcat(final_prompt,conversation[0]);

  return(final_prompt);
}

void add_llm_response_to_conversation(char** conversation,
				      size_t* conversation_length,
				      char* system_response) {
  conversation_length[0] += strlen(system_response);
  conversation[0] =
    (char*)realloc(conversation[0],sizeof(char)*(conversation_length[0]+1));
  strcat(conversation[0],system_response);
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

  char* conversation = (char*)malloc(sizeof(char)*1);
  size_t conversation_length = 0;
  
  database vdb;
  tokenlist tl;
  size_t n_db_results;
  int n_tokens;
  
  char* llm_answer;
  
  CURL *curl = curl_easy_init();


  struct curl_slist *host = NULL;
  char* curl_slist_string;
  size_t curl_slist_string_size;
  char* ip_address;
  
  CURLcode result;
  
  response_data rd;

  int terminate = 0;
  
  
  char* url;
  size_t url_size;

  conversation[0] = 0;
  
  if(argc < 5) {
    printf("Arguments are:\n"
	   "  [string]  hostname\n"
	   "  [int]     port\n"
	   "  [file]    vector-database\n"
	   "  [int]     db results per query\n"
	   "  [file]    tokenizer\n"
	   "  [int]     number of tokens per answer, -1 for infinite\n");
    return(1);
  }	   

  url_size = + strlen(argv[1]) + strlen(argv[2]) + 25;
  url = (char*)malloc(sizeof(char)*url_size);
  sprintf(url,"http://%s:%s/completion",argv[1],argv[2]);

  vdb = read_db_from_disk(argv[3]);
  sscanf(argv[4],"%li",&n_db_results);
  tl = read_tokenizer_from_file(argv[5]);

  sscanf(argv[6],"%i",&n_tokens);
  
  rd.size = 0;
  rd.data = NULL;

  if(!curl) {
    fprintf(stderr,"Failed to initialize CURL, EXITING\n");
    return(1);
  }

  ip_address = local_resolve(argv[1]);

  if(ip_address != NULL) {
  
    curl_slist_string_size = strlen(argv[1])
      + strlen(argv[2]) + strlen(ip_address) + 3;
    
    curl_slist_string = (char*)malloc(sizeof(char)*curl_slist_string_size);
    sprintf(curl_slist_string,"%s:%s:%s",argv[1],argv[2],ip_address);

    host = curl_slist_append(NULL,curl_slist_string);
    
  }  
  while(!terminate) {

    
    iobuffer = readline("\001\033[33;1m\002 Human > \001\033[0m\002");

    if (NULL == iobuffer) {
      terminate = 1;
      goto exit_repl;
    }

    if(response != NULL) {
      add_llm_response_to_conversation(&conversation,
				       &conversation_length,
				       response);
      free(response);
    }
    
    prompt = generate_instruct_prompt_with_db(iobuffer,
					      &conversation,
					      &conversation_length,
					      vdb,
					      tl,
					      n_db_results);

    /* fprintf(stderr,"PROMPT: %s",prompt); */
    
    free(iobuffer);	
    
    query_string = generate_server_query(prompt, n_tokens);
    
    if (rd.data != NULL) {
      free(rd.data);
      rd.data = NULL;
      rd.size = 0;
    }

    if(ip_address != NULL) {
      curl_easy_setopt(curl, CURLOPT_RESOLVE, host);
    }
    curl_easy_setopt(curl, CURLOPT_URL,url);
    
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
  if(ip_address != NULL) free(ip_address);
  curl_easy_cleanup(curl);
}
  

		 
  
