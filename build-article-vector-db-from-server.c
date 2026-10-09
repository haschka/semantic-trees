#include<stdio.h>
#include <string.h>
#include <stdlib.h>

#include "vector-db.h"
#include "load-texts.h"
#include "embedding-from-server.h"


static inline int sanity_check(embedding e) {

  int i;

  /* check if vector has a length */
  if(e.vector_length <= 0) {
    return (0);
  }

  /* check if vector contains non zero elements */
  for(i=0;i<e.vector_length;i++) {
    if(e.vector[i] != 0.0) goto sanity;
  }
  free(e.vector);
  return(0);
 sanity:
  return(1);
}

int main(int argc, char** argv) {

  size_t i,j;
  
  database vdb;
  char** embeddings;
  char** titles;
  char** resumes;

  char** title_and_resumes;
  
  char** subtexts;
  size_t n_subtexts;

  size_t n_titles;
  size_t n_resumes;
  size_t length_of_resume;
  size_t new_length_of_resume;

  
  
  char* hostname = argv[2];
  char* port = argv[3];
  char* vdb_filename = argv[4];

  char sep[] ="<<<sep>>>";
  
  embedding e;

  size_t slices_lengths;

  size_t retries, max_retries = 8;
  
  if(argc < 5) {
    printf("Arguments are: \n"
	   " [file]     Preprocessed file containing titles and resumes\n"
	   " [hostname] Hostname of the server providing embeddings\n"
	   " [port]     Port of the server providing embeddings\n"
	   " [file]     Vector database to write\n");
    return(1);
  }

  titles = extract_texts_from_jsonl_file("title",argv[1],
					 &n_titles);
  
  resumes = extract_texts_from_jsonl_file("resumes",
					  argv[1],&n_resumes);
    
  if(n_titles != n_resumes) {
    fprintf(stderr,
	    "Number of titles is not the same as number "
	    "of resumes. JSON parsing error \n");
    return(1);
  }

  title_and_resumes = (char**)malloc(sizeof(char*)*n_resumes);
  
  for(i=0;i<n_resumes;i++) {
    /* (strlen(sep) = 10 + '0' termination) = 11 */
    length_of_resume = strlen(titles[i])+strlen(resumes[i])+11;
    title_and_resumes[i] = (char*)malloc(sizeof(char)*length_of_resume);
    title_and_resumes[i][0] = 0;
    strcat(title_and_resumes[i],titles[i]);
    strcat(title_and_resumes[i],sep);
    strcat(title_and_resumes[i],resumes[i]);

    free(resumes[i]);
    free(titles[i]);
    resumes[i] = title_and_resumes[i];
  }
  
  embeddings = (char**)malloc(sizeof(char*)*n_resumes);
  
  for(i=0;i<n_resumes;i++) {
    e = get_embedding_from_server(hostname,port,resumes[i]);

    /* imposing sanity checks */
    retries = 1;
    while(!sanity_check(e)) {
      length_of_resume = strlen(resumes[i]);
      new_length_of_resume =
	length_of_resume-retries*length_of_resume/max_retries;
      
      if(retries > max_retries) {
	fprintf(stderr,
		"Fatal: Failed to obtain embedding for resume %lu "
		"from server \n", i);
	return(1);
      }
      fprintf(stderr,
	      "No sane embedding recieved for resume %lu \n"
	      "Trying to scale down from %lu to %lu \n",
	      i,length_of_resume, new_length_of_resume );

      subtexts = cut_texts_in_subtexts(resumes[i],
				       length_of_resume,
				       new_length_of_resume,
				       &n_subtexts);
      
      e = get_embedding_from_server(hostname,port,subtexts[0]);

      for(j=0;j<n_subtexts;j++) {
	free(subtexts[j]);
      }
      free(subtexts);
      
      retries++;
    }
    
    posix_memalign((void**)(embeddings+i),
		   32,
		   sizeof(double)*(e.vector_length+(4-e.vector_length%4)));

    memset(embeddings[i],
	   0,
	   sizeof(double)*(e.vector_length+(4-e.vector_length%4)));

    memcpy(embeddings[i],e.vector,sizeof(double)*e.vector_length);
    free(e.vector);
  }

  vdb.vector = embeddings;
  vdb.text = resumes;
  vdb.type = DOUBLE_VECTOR;
  vdb.vector_length = e.vector_length;
  vdb.n_entries = n_resumes;

  write_db_to_disk(vdb_filename,vdb);

  return(0);
}
