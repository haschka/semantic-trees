#include<stdio.h>
#include "vector-db.h"

int main(int argc, char** argv) {

  size_t idx_a, idx_b;
  database db;
  
  if(argc < 2) {
    printf("Usage: %s [vector-db] idx_a idx_b",argv[0]);
    return(1);
  }

  sscanf(argv[2],"%lu",&idx_a);
  sscanf(argv[3],"%lu",&idx_b);
  
  db = read_db_from_disk(argv[1]);

  printf("Scalar Result: %lf \n"
	 "Vector Result: %lf \n",
	 Yule_distance_scalar(db.vector[idx_a],
			      db.vector[idx_b],
			      db.vector_length),
	 Yule_distance(db.vector[idx_a],
		       db.vector[idx_b],
		       db.vector_length));

}
