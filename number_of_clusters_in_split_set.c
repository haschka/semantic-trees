#include <stdio.h>
#include "vector-db.h"
#include "cluster.h"

int main(int argc, char** argv) {

  size_t i;

  split_set s;
  
  for(i=1;i<argc;i++) {

    s = read_split_set(argv[i]);
    printf("%s %i\n",argv[i],s.n_clusters);
    fflush(stdout);
    free_split_set_and_associated_clusters(s);

  }
  
  return(0);
  
}
    
    
   
    
  
