#include <stdio.h>
#include "vector-db.h"

int main(int argc, char** argv) {

  database db;
  size_t i;
  size_t idx;
  
  if(argc < 2) {
    printf("Usage: %s [vector-db] [list of entry indices]\n", argv[0]);
    return(1);
  }

  db = read_db_from_disk(argv[1]);

  for(i = 2; i < argc; i++) {
    sscanf(argv[i], "%li", &idx );
    print_db_entry_by_index(idx,db);
  }
}
