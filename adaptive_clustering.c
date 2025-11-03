#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<ctype.h>
#include<unistd.h>

#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>

#include"vector-db.h"
#include"cluster.h"
#include"dbscan.h"

void file_error(char* path) {
  printf("failed to open file %s\n",path);
  _exit(1);
}

void print_arguments() {

  printf("Arguments are: \n"
	 "   [file] Vector Database File\n"
	 "   (float) initial epsilon value \n"
	 "   (float) epsilon increase between cluster search \n"
	 "   (int) minimum numbers of clusters in epsilon neigbourhood \n"
	 "   (string) path/andprefix of split_set_files \n"
	 "   (int) number of threads to use \n"
	 );
}

int main(int argc, char** argv) {

  int i,j,k;
  
  database vdb;

  float epsilon;
  int minpts;

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

  vdb = read_db_from_disk(argv[1]);

  printf("Vector Database read!\n");

  adaptive_dbscan(dbscan, vdb, epsilon_start, epsilon_inc, minpts,
		  split_files_prefix, n_threads);

}
