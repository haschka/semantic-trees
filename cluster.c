#include<unistd.h>
#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<string.h>
#include<limits.h>
#include<fcntl.h>

#include"vector-db.h"
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

split_set read_split_set(char* filename) {

  char s_set[] = "SPLIT_SET";
  char buffer[10];
  
  int i,j;

  int fd = open(filename, O_RDONLY);

  size_t check = 0;
  size_t count;

  split_set s;

  size_t si = sizeof(int);
  
  check += read(fd,buffer,9);
  buffer[9] = 0;
  if (0 != strcmp(buffer,s_set)) {
    printf("Error %s is not a split_set\n", filename);
    _exit(1);
  }
  check += read(fd, &s.n_clusters, sizeof(int));
  count = 9+si;
  s.clusters = (cluster*)malloc(sizeof(cluster)*s.n_clusters);
  for(i = 0; i < s.n_clusters; i++) {
    check += read(fd, &s.clusters[i].n_members, sizeof(int));
    check += read(fd, &s.clusters[i].id, sizeof(int));
    count += 2*si;
    s.clusters[i].members = (int*)malloc(sizeof(int)*s.clusters[i].n_members);
    for(j= 0; j < s.clusters[i].n_members; j++) {
      check += read(fd, s.clusters[i].members+j, sizeof(int));
      count += si;
    }
  }
  if (check != count) {
    printf("There was an error reading " 
	   "the binary clusterfile %s", filename);
  }
  close(fd);
  return(s);
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

void write_split_set_to_db_indices(char* prefix, split_set s) {

  size_t i,j;

  char file_name[PATH_MAX];

  FILE* f;

  char i_buffer[7];
  
  for (i = 0; i< s.n_clusters; i++) {

    file_name[0]= 0;
    strcat(file_name, prefix);
    strcat(file_name, "-");
    sprintf(i_buffer, "%06lu",i);
    strcat(file_name, i_buffer);

    f = fopen(file_name,"w");
    
    for (j=0; j< s.clusters[i].n_members; j++) {
      fprintf(f,"%i\n",s.clusters[i].members[j]);
    }
    fclose(f);
  }
}

void write_split_set_as_vdbs(char* prefix, split_set s, database db) {

  size_t i,j;

  char file_name[PATH_MAX];

  char i_buffer[7];

  database write_out_base;

  size_t write_vector_length, copy_vector_length, current_text_length;

  write_out_base.type = db.type;
  write_out_base.vector_length = db.vector_length;

  if(write_out_base.type == BINARY_VECTOR) {
    write_vector_length = write_out_base.vector_length/8+1;
  } else if (write_out_base.type == DOUBLE_VECTOR) {
    write_vector_length = write_out_base.vector_length*sizeof(double);
  }

  copy_vector_length = write_vector_length+(32-write_vector_length%32);
  
  for (i = 0; i< s.n_clusters; i++) {
    
    file_name[0]= 0;
    strcat(file_name, prefix);
    strcat(file_name, "-");
    sprintf(i_buffer, "%06lu",i);
    strcat(file_name, i_buffer);

    write_out_base.n_entries = s.clusters[i].n_members;

    write_out_base.vector =
      (char**)malloc(sizeof(char*)*write_out_base.n_entries);
    write_out_base.text =
      (char**)malloc(sizeof(char*)*write_out_base.n_entries);

    for(j=0; j< s.clusters[i].n_members; j++) {
      posix_memalign((void**)(write_out_base.vector+j),
		     32,
		     copy_vector_length);
      memcpy(write_out_base.vector[j],
	     db.vector[s.clusters[i].members[j]],
	     copy_vector_length);

      current_text_length = strlen(db.text[s.clusters[i].members[j]]);
      write_out_base.text[j] =
	(char*)malloc(sizeof(char)*current_text_length+1);
      memcpy(write_out_base.text[j],db.text[s.clusters[i].members[j]],
	     sizeof(char)*current_text_length+1);
    }
    
    write_db_to_disk(file_name, write_out_base);

    for(j=0; j< s.clusters[i].n_members; j++) {
      free(write_out_base.vector[j]);
      free(write_out_base.text[j]);
    }

    free(write_out_base.vector);
    free(write_out_base.text);

  }
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

cluster data_not_in_clusters(split_set s, database ds) {

  int i,j;
  
  cluster out;

  char* out_b = alloc_and_set_zero_binary_array(ds.n_entries);
  size_t count; 

  out.members = (int*)malloc(sizeof(int)*ds.n_entries);
  
  for( i = 0; i< s.n_clusters; i++) {
    for (j = 0; j< s.clusters[i].n_members; j++) {
      set_value_in_binary_array_at_index( out_b, s.clusters[i].members[j]);
    }
  }

  count = 0;
  for( i = 0; i< ds.n_entries; i++) {
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
    
