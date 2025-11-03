#include <stdio.h>
#include"vector-db.h"
#include"cluster.h"

int main(int argc, char** argv) {
  
  split_set s;

  database db;

  if(argc < 4) {
    printf("Usage: Arguments are: \n"
	   "  [vector-db] A vector database to extract subsets from.\n"
	   "  [split-set] A split set defining the subsets to be extracted\n"
	   "  [prefix]    A path-prefix for the files to be generated\n");
    return(1);
  }	   
  
  s = read_split_set(argv[2]);
  db = read_db_from_disk(argv[1]);
  
  write_split_set_as_vdbs(argv[3],s,db);
  
  return(0);
  
}
