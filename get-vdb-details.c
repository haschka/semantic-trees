#include<stdio.h>

#include"vector-db.h"

int main(int argc, char** argv) {

  if(argc < 2) {

    printf("Usage: %s [vector_database] \n",argv[0]);
    return(1);
    
  }
  database db = read_db_from_disk(argv[1]);
  
  if(db.type == BINARY_VECTOR) {
    printf("Binary Vector Database\n");
  } else if (db.type = DOUBLE_VECTOR) {
    printf("Double Precision Database\n");
  }
  printf("Vector Length %lu \n",db.vector_length);
  printf("Number of Entries %lu \n",db.n_entries);
  fflush(stdout);
  
  return(0);
  
}

  
