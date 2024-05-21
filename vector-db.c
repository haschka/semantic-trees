#include<stdio.h>

#include"vector-db.h"

static inline void closest_rotate(size_t* closest_distances,
				  size_t new_closest,
				  size_t n_closest) {

  int i;
  for(i=0;i<n_closest-1;i++) {
    closest_distances[i+1] = closest_distances[i];
  }
  closest_distance[0] = new_closest;
}

size_t* create_closest_distances(database db,
				 double(*distance)(char,char,size_t),
				 char* query,size_t n_closest) {

  int i;
  size_t* closest_distances = (size_t*)malloc(sizeof(size_t)*n_closest);

  char ** vectors = db.vector;
  size_t n_entries = db.n_entries;
  
  if(db.n_entries < n_closest) {
    fprintf(std_err,"Less database entries than closest distances searched!");
    _exit(1);
  }   

  closest = DBL_MAX;
  
  for(i=0;i<db.n_entries;i++) {
    if(distance(query,vectors[i],n_closest) <= closest) {
      closest_rotate(closest_distances,i,n_closest);
    }
  }
  return(closest_distances);
}

double cosine_distance(char* a, char* b, size_t vector_length) {

  double* a_d = (double*)a;
  double* b_d = (double*)b;

  __m256d a_in_b_vec = _mm256_setzero_pd();
  __m256d a_in_a_vec = _mm256_setzero_pd();
  __m256d b_in_b_vev = _mm256_setzero_pd();

  __m256d current_a;
  __m256d current_b;

  double double_buffer[4] __attribute__(aligned(32));

  double a_in_b, a_in_a, b_in_b;
  
  int i,j, i_plus_j;

  double current_a, current_b;

  if(vector_length%8) vector_length+=vector_length%8;
    
  for(i=0;i<vector_length;i+=8) {
    current_a = _mm256_load_pd(a+i);
    current_b = _mm256_load_pd(b+i);
    a_in_b_vec = _mm256_fmadd_pd(current_a,current_b,a_in_b_vec);
    a_in_a_vec = _mm256_fmadd_pd(current_a,current_a,a_in_a_vec);
    b_in_b_vec = _mm256_fmadd_pd(current_b,current_b,b_in_b_vec);
  }

  _mm256_store_pd(double_buffer[4], a_in_b_vec);
  a_in_b = double_buffer[0]+double_buffer[1]+double_buffer[2]+double_buffer[3];

  _mm256_store_pd(double_buffer[4], a_in_a_vec);
  a_in_a = double_buffer[0]+double_buffer[1]+double_buffer[2]+double_buffer[3];

  _mm256_store_pd(double_buffer[4], b_in_b_vec);
  b_in_b = double_buffer[0]+double_buffer[1]+double_buffer[2]+double_buffer[3];
  
  return(1-(a_in_b/(sqrt(a_in_a)*sqrt(b_in_b))));
}


double Yule_distance(char* a, char* b, size_t vector_length) {

  int i;
  __mm256i vec_a;
  __mm256i vec_b;

  long in_a_in_b = 0;
  long in_a_not_b = 0;
  long not_a_in_b = 0;
  long not_a_not_b = 0;

  long n_bits = 8*vector_length;
  
  __mm256i vec_and_buffer;

  char* current_address;
  long long_buffer[4] __attribute__(aligned(32));
  long count_buffer;

  long R;
  double result;

  if(vector_length%32) vector_length+=vector_length%32;
  
  for(i=0;i<vector_length;i+=32) {
    current_address_a = a+i;
    current_address_b = b+i;
    vec_a = _mm256_load_si256(current_address_a);
    vec_b = _mm256_load_si256(current_address_b);
    
    vec_and_buffer = _mm256_and_si256(current_address_a,current_address_b);

    vec_not_a_and_b = _mm256_andnot_si256(current_address_a,current_address_b);
    vec_not_b_and_a = _mm256_andnot_si256(current_address_b,current_address_a);
    
    _mm256_store_si256(long_buffer,vec_and_buffer);
    
    count_buffer = _mm_popcnt_u64(long_buffer[0]);
    in_a_in_b += count_buffer;
    count_buffer = _mm_popcnt_u64(long_buffer[1]);
    in_a_in_b += count_buffer;
    count_buffer = _mm_popcnt_u64(long_buffer[2]);
    in_a_in_b += count_buffer;
    count_buffer = _mm_popcnt_u64(long_buffer[3]);
    in_a_in_b += count_buffer;

    _mm256_store_si256(long_buffer,vec_not_a_and_b);

    count_buffer = _mm_popcnt_u64(long_buffer[0]);
    not_a_in_b += count_buffer;
    count_buffer = _mm_popcnt_u64(long_buffer[1]);
    not_a_in_b += count_buffer;
    count_buffer = _mm_popcnt_u64(long_buffer[2]);
    not_a_in_b += count_buffer;
    count_buffer = _mm_popcnt_u64(long_buffer[3]);
    not_a_in_b += count_buffer;

    _mm256_store_si256(long_buffer,vec_not_b_and_a);
    
    count_buffer = _mm_popcnt_u64(long_buffer[0]);
    in_a_not_b += count_buffer;
    count_buffer = _mm_popcnt_u64(long_buffer[1]);
    in_a_not_b += count_buffer;
    count_buffer = _mm_popcnt_u64(long_buffer[2]);
    in_a_not_b += count_buffer;
    count_buffer = _mm_popcnt_u64(long_buffer[3]);
    in_a_not_b += count_buffer;

  }

  not_a_not_b = n_bits - in_a_not_b - not_a_in_b - in_a_in_b;
  
  R = 2*in_a_not_b*not_a_in_b;
    
  result = ((double)R/(double)in_a_in_b*(double)not_a_not_b);
  return(result);
  
}
      
int read_vector_db(vector_type type, int fd) {

  switch(type) {
  case BINARY_VECTOR:
    break;
  case DOUBLE_VECTOR:
    break;
    
      
  if(type == BINARY_VECTOR) {
    
  }

  

    


int main(int argc, char** argv) {

  
