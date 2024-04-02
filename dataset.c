#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>

#if defined (_YULE_DISTANCE)
#include"binary_array.h"
#endif

#include"dataset.h"

data_shape shape_from_embeddings_file(int infile) {

  data_shape s;

  unsigned char current_character;

  off_t size;

  int i;

  s.n_features = 0;
  s.n_samples = 0;

  size = lseek(infile, 0, SEEK_END);
  lseek(infile, 0, SEEK_SET);

  char* f_buffer = (char*)malloc(sizeof(char)*size);

  if (size != read(infile, f_buffer, size)) {
    printf("Could not read file in order to read obtain data shape\n");
    _exit(1);
  }

  i = 0;
  while( f_buffer[i] != '\n') {
    if ( f_buffer[i] == '\t') s.n_features++;
    i++;
  }

  while( i < size ) {
    if ( f_buffer[i] == '\n' ) s.n_samples++;
    i++;
  }
  return(s);
}

dataset load_embeddings_from_file_into_dataset(FILE* in_file,
					       data_shape shape) {

  dataset ds;
  int i, j;

#if defined(_YULE_DISTANCE)
  int bin_buffer;
#endif
  
  char buffer[1024];

  ds.n_dimensions = shape.n_features;

  ds.n_values = shape.n_samples;

#if defined(_COS_DISTANCE)
  ds.values = (double**)malloc(sizeof(double*)*ds.n_dimensions);

  for(i = 0 ; i < ds.n_values; i++) {
    ds.values[i] =
      (double*)malloc(sizeof(double)*ds.n_dimensions);
  }
#elif defined(_YULE_DISTANCE)
  ds.values = (char**)malloc(sizeof(char*)*ds.n_dimensions);

  for(i = 0 ; i < ds.n_values; i++) {
    ds.values[i] =
      alloc_and_set_zero_binary_array(ds.n_dimensions);
  }
#endif
  
  rewind(in_file);
  
  for(i=0;i<shape.n_samples;i++) {
    fscanf(in_file,"%s", buffer);
    for(j=0;j<shape.n_features;j++) {
      
#if defined(_COS_DISTANCE)
      fscanf(in_file,"%lf", ds.values[i]+j);
#elif defined(_YULE_DISTANCE)
      fscanf(in_file,"%i", &bin_buffer);
      if(bin_buffer) {
	set_value_in_binary_array_at_index(ds.values[i],j);
      }
#endif      
    }
  }
  return(ds);
}

void free_dataset(dataset ds) {
  int i;
  for(i=0;i<ds.n_dimensions;i++) {
    free(ds.values[i]);
  }
  free(ds.values);
}
