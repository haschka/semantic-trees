#include<stdlib.h>

double* alloc_symmetric_upper_triange(int range) {
  double* m = (double*)malloc(sizeof(double)*range*(range+1)/2);
  return(m);
}

void free_symmetric_upper_triangle(double* m) {
  free(m);
}

double get_element_from_symmetric_upper_triangle_at_index(double* m,
							  int range,
							  int i, int j) {
  int swap;
  
  if(j<i) {
    swap = j;
    j = i;
    i = swap;
  }
  
  index = i*(2*range-i+1)/2+(j-i);

  return(m[index]);
}


void set_element_from_symmetric_upper_triangle_at_index(double v,
							double* m,
							int range,
							int i, int j) {
  int swap;
  
  if(j>i) {
    swap = j;
    j = i;
    i = swap;
  }

  index = i*(2*range-i+1)/2+(j-i);

  m[index] = value;
}
