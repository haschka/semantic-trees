#include <stdio.h>
#include "vector-db.h"

int main(int argc, char** argv) {

  database db;
  size_t i;
  size_t idx;
  
  if(argc < 1) {
    printf("Usage: %s [vector-db]\n", argv[0]);
    printf("Database indices are read from stdin!");
    return(1);
  }
  
  db = read_db_from_disk(argv[1]);

  while(1 == fscanf(stdin,"%li",&idx)) {
    print_text_only_from_db_entry_by_index(idx,db);
  }
}
