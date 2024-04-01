#include<unistd.h>
#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>

#include"dataset.h"
#include"cluster.h"
#include"binary_array.h"

static inline int comp_int(const void *a, const void* b) {
  int* a_i = (int*)a;
  int* b_i = (int*)b;

  return(a_i[0]-b_i[0]);
}

static inline int binary_search(int* array, size_t array_size, int target) {

  int left = 0;
  int right = array_size - 1;
  int pos;
  
  while( left <= right ) {
    pos = (left+right)/2;
    if (array[pos] < target) {
      left = pos + 1;
    } else if (array[pos] > target) {
      right = pos - 1;
    } else {
      return pos;
    }
  }
  return -1;
}

void store_split_set(char* filename, split_set s) {

  char s_set[] = "SPLIT_SET";

  int i,j;
  
  int fd = open(filename, O_WRONLY | O_CREAT,
		S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

  size_t check = 0;
  size_t count;
  
  size_t si = sizeof(int);
  
  check += write(fd, s_set, 9);
  check += write(fd, &s.n_clusters, sizeof(int));
  count = 9+si;
  for(i = 0; i < s.n_clusters; i++) {
    check += write(fd, &s.clusters[i].n_members, sizeof(int));
    check += write(fd, &s.clusters[i].id, sizeof(int));
    count += 2*si;
    for(j= 0; j < s.clusters[i].n_members; j++) {
      check += write(fd, s.clusters[i].members+j, sizeof(int));
      count += si;
    }
  }
  if (check != count) {
    printf("There was an error in creating " 
	   "the binary clusterfile %s", filename);
  }
  close(fd);
}

cluster intersection_of_clusters(cluster a, cluster b) {

  cluster big;
  cluster small;

  int i, j, count;
  int search_result;
  
  cluster intersection;
  
  if ( a.n_members >= b.n_members ) {
    big = a;
    small = b;
  } else {
    big = b;
    small = a;
  }

  intersection.members= (int*)malloc(sizeof(int)*small.n_members);

  count = 0;
  if( big.n_members > 20 ) {
  
    qsort(big.members, big.n_members, sizeof(int), comp_int);

    for(i = 0 ; i < small.n_members; i++) {
      
      search_result = binary_search(big.members,
				    big.n_members,
				    small.members[i]);

      if (search_result != -1) {
	intersection.members[count] = small.members[i];
	count++;
      }
    }
    
  } else {

    for(i = 0 ; i < small.n_members; i++) {
      for(j = 0; j < big.n_members;j++) {
	if (small.members[i] == big.members[j]) {
	  intersection.members[count] = small.members[i];
	  count++;
	}
      }
    }
    
  }
  intersection.n_members = count;
  intersection.id = -1;
  intersection.members =
    (int*)realloc(intersection.members, sizeof(int)*count);

  return(intersection);
}

cluster_connections*
generate_split_set_relation(split_set ancient, split_set new) {

  int i,j, counter;
  
  cluster_connections* connections_ancient_to_new =
    (cluster_connections*)malloc(sizeof(cluster_connections)*new.n_clusters);
  
  cluster intersection;
  
  for(i=0;i<new.n_clusters;i++) {

    connections_ancient_to_new[i].connections =
      (int*)malloc(sizeof(int)*ancient.n_clusters);
    counter = 0;
    for(j=0;j<ancient.n_clusters;j++) {

      intersection = intersection_of_clusters(ancient.clusters[j],
					      new.clusters[i]);

      if ( (double)intersection.n_members >
	   (double)ancient.clusters[j].n_members*(double)0.8 ) {

	connections_ancient_to_new[i].connections[counter] = j;
	counter++;

      }
      if (intersection.n_members > 0) {
	free(intersection.members);
      }
    }
    connections_ancient_to_new[i].connections =
      (int*)realloc(connections_ancient_to_new[i].connections,
		    sizeof(int)*counter);
    connections_ancient_to_new[i].n_connections = counter;
  }
  return(connections_ancient_to_new);
}

cluster data_not_in_clusters(split_set s, dataset ds) {

  int i,j;
  
  cluster out;

  char* out_b = alloc_and_set_zero_binary_array(ds.n_values);
  size_t count; 

  out.members = (int*)malloc(sizeof(int)*ds.n_values);
  
  for( i = 0; i< s.n_clusters; i++) {
    for (j = 0; j< s.clusters[i].n_members; j++) {
      set_value_in_binary_array_at_index( out_b, s.clusters[i].members[j]);
    }
  }

  count = 0;
  for( i = 0; i< ds.n_values; i++) {
    if(!(get_value_in_binary_array_at_index(out_b,i))){
      out.members[count] = i;
      count++;
    }
  }
  out.id = -1;
  out.members = (int*)realloc(out.members,sizeof(int)*count);
  out.n_members = count;
  free(out_b);
  return(out);
}

void free_split_set_and_associated_clusters(split_set s) {

  int i;
  for(i=0;i<s.n_clusters;i++) {
    free(s.clusters[i].members);
  }
  free(s.clusters);
}
    
