#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<ctype.h>
#include<unistd.h>

#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>

#include"dataset.h"
#include"cluster.h"
#include"dbscan.h"

void file_error(char* path) {
  printf("failed to open file %s\n",path);
  _exit(1);
}

void print_arguments() {

  printf("Arguments are: \n"
	 "   [file] Embeddings File\n"
	 "   (float) initial epsilon value \n"
	 "   (float) epsilon increase between cluster search \n"
	 "   (int) minimum numbers of clusters in epsilon neigbourhood \n"
	 "   (string) path/andprefix of split_set_files \n"
	 "   (int) number of threads to use \n"
	 );
}

int main(int argc, char** argv) {

  int i,j,k;
  
  dataset ds;

  float epsilon;
  int minpts;

  int embeddings_fd;
  FILE* embeddings_f;

  data_shape shape;

  char split_files_prefix[255];

  float epsilon_start;
  float epsilon_inc;
  int n_threads;

  if(argc < 5) {
    print_arguments();
    return(1);
  }

  sscanf(argv[2], "%f", &epsilon_start);
  sscanf(argv[3], "%f", &epsilon_inc);
  sscanf(argv[4], "%i", &minpts);
  sscanf(argv[5], "%s", split_files_prefix);
  sscanf(argv[6], "%i", &n_threads);

  if ( -1 == (embeddings_fd = open(argv[1], O_RDONLY))) file_error(argv[1]);

  shape = shape_from_embeddings_file(embeddings_fd);
  embeddings_f = fdopen(embeddings_fd,"r");
  ds = load_embeddings_from_file_into_dataset(embeddings_f,shape);
  fclose(embeddings_f);

  printf("Dataset read!\n");

  adaptive_dbscan(dbscan, ds, epsilon_start, epsilon_inc, minpts,
		  split_files_prefix, n_threads);

 finish:
  free_dataset(ds);
}
