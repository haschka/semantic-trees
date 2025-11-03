#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<curl/curl.h>

#include "local_resolve.h"
#include "curl_helpers.h"

int main(int argc, char** argv) {

  int n_tokens;

  CURL *curl = curl_easy_init();

  struct curl_slist *host = NULL;
  char* curl_slist_string;
  size_t curl_slist_string_size;
  char* ip_address;
  
  CURLcode result;
  response_data rd;
  char* url;
  size_t url_size;
  
  char* prompt = (char*)malloc(sizeof(char)*1000);
  char* query_string = NULL;
  char current_character; 

  size_t prompt_size = 0;

  if (argc < 4) {
    printf("Usage: \n"
	   " %s [host] [port] [n_tokens] \n"
	   " Reads a LLM prompt from stdin, sents it to the server \n"
	   " at [host] [port] and yields an llm completion to stdout. \n"
	   " [n-tokens] defines how many tokens the LLM should return, \n"
	   "   -1 yields as many tokens until an the LLM terminates the \n"
	   "   response \n",
	   argv[0]);
    return(1);
  }
  
  url_size = strlen(argv[1]) + strlen(argv[2]) + 25;
  url = (char*)malloc(sizeof(char)*url_size);
  sprintf(url,"http://%s:%s/completion",argv[1],argv[2]);

  sscanf(argv[3],"%i",&n_tokens);
  
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
  
  while (  EOF != (current_character = fgetc(stdin)) ) {
    prompt[prompt_size] = current_character;
    prompt_size++;
    if ( prompt_size%1000 == 0 ) {
      prompt = (char*)realloc(prompt, sizeof(char)*(prompt_size+1000));
    }
  }
  prompt = (char*)realloc(prompt, sizeof(char)*(prompt_size+1));

  prompt[prompt_size] = 0;
  
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
    
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
		   write_function_callback_stream_llm);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA,(void*)&rd);

  result = curl_easy_perform(curl);
  
  fputs("\n",stdout);

  if(ip_address != NULL) free(ip_address);
  if(query_string != NULL) free(query_string);
  curl_easy_cleanup(curl);
}
