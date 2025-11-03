#include <stdio.h>
#include"vector-db.h"
#include"cluster.h"

int main(int argc, char** argv) {
  
  split_set s;

  database db;

  if(argc < 3) {
    printf("Usage: Arguments are: \n"
	   "  [split-set] A split set to extract data indices from\n"
	   "  [prefix]    A path-prefix for the index files to be generated\n");
    return(1);
  }
  
  s = read_split_set(argv[1]);
  
  write_split_set_to_db_indices(argv[2],s);
  
  return(0);
  
}
