#include<stdio.h>
#include "vector-db.h"

int main(int argc, char** argv) {

  if(argc < 2) {
    printf("Usage: %s [vector-db]",argv[0]);
    return(1);
  }
  
  database db = read_db_from_disk(argv[1]);

  print_intra_distances_vdb(stdout,db);

}
