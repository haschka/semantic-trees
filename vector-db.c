#include<stdio.h>
#include<math.h>
#include<string.h>
#include<fcntl.h>
#include <strings.h>
#include<unistd.h>
#include<float.h>
#include<stdlib.h>
#include<limits.h>
#include<pthread.h>

#if defined(__AVX2__)
#include <immintrin.h>
#endif

#include"vector-db.h"
#include"cluster.h"
#include"binary_array.h"


typedef struct {
  double distance;
  size_t index;
} distances;

typedef struct {
  database* db;
  size_t* incrementor;
  double* matrix;
} thread_handle;

pthread_mutex_t distance_matrix_lock;

int d_comp(const void* a,const void *b) {
  distances* a_d = (distances*)a;
  distances* b_d = (distances*)b;
  if(a_d[0].distance < b_d[0].distance) {
    return -1;
  } else if (a_d[0].distance > b_d[0].distance) {
    return 1;
  }
  return 0;
}

static inline void closest_rotate(size_t* closest_distances,
				  size_t new_closest,
				  size_t n_closest) {

  int i;
  for(i=n_closest-2;i>=0;i--) {
    closest_distances[i+1] = closest_distances[i];
  }
  closest_distances[0] = new_closest;
}

void file_open_error(char* filename,char* operation) {
  fprintf(stderr,"Unable to open vector database file %s for %s\n",
	  filename, operation);
  _exit(1);
}

void write_db_to_disk(char* filename, database db) {

  char desc[4] = "VEC";

  int fd = open(filename,
		O_CREAT | O_WRONLY,
		S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP );

  size_t i;

  size_t write_vector_length;

  size_t current_text_length;

  char t;

  if(fd == -1) {
    file_open_error(filename,"writing");
  }
  
  if(db.type == BINARY_VECTOR) {
    write_vector_length = db.vector_length/8+1;
    t = 0;
  } else if(db.type == DOUBLE_VECTOR) {
    t = 1;
    write_vector_length = db.vector_length*sizeof(double);
  }

  write(fd,desc,sizeof(char)*4);
  write(fd,&t,sizeof(char));
  write(fd,&db.n_entries,sizeof(size_t));
  write(fd,&db.vector_length,sizeof(size_t));
  
  for(i=0;i<db.n_entries;i++) {
    write(fd,db.vector[i],write_vector_length);
  }
  for(i=0;i<db.n_entries;i++) {
    current_text_length = strlen(db.text[i]);
    write(fd,&current_text_length,sizeof(size_t));
    write(fd,db.text[i],current_text_length);
  }
  close(fd);
}

void print_db_entry_by_index(size_t idx, database db) {

  size_t i;

  if( idx < 0 || idx > db.n_entries-1 ) {
    fprintf(stderr,"Index %lu out of range!", idx);
  } else {
  
    printf("Vector:");
  	 
    if(db.type == DOUBLE_VECTOR) {
      for (i=0;i<db.vector_length; i++) {
	if (i%5 == 0) printf("\n");
	printf("% 12E6 ",((double*)(db.vector[idx]))[i]);
      }
    } else if(db.type == BINARY_VECTOR) {
      for (i=0;i<db.vector_length; i++) {
	if (i%79 == 0) printf("\n");
	printf("%i",get_value_in_binary_array_at_index(db.vector[idx],i));
      }
    }
    printf("\n");
    printf("Text:\n");
    printf("%s",db.text[idx]);
  }
}

void print_text_only_from_db_entry_by_index(size_t idx,
					    database db) {

  if( idx < 0 || idx > db.n_entries-1 ) {
    fprintf(stderr,"Index %lu out of range!", idx);
  } else {
    printf("%s",db.text[idx]);
    printf("\n");
  }
}
    
database read_db_from_disk(char* filename) {
  
  char desc[4] = "VEC";

  char* check_desc = (char*)malloc(sizeof(char)*4);
  
  int fd = open(filename,O_RDONLY );

  size_t i;

  size_t write_vector_length;

  size_t current_text_length;

  char t;

  database db;

  if(fd == -1) {
    file_open_error(filename,"reading");
  }

  memset(check_desc,0,sizeof(char)*4);  

  read(fd,check_desc,sizeof(char)*4);
  if(0 !=strcmp(desc,check_desc)) {
    fprintf(stderr,"Warning Database file to be load corrupted!");
  }
  
  read(fd,&t,sizeof(char));
  read(fd,&db.n_entries,sizeof(size_t));
  read(fd,&db.vector_length,sizeof(size_t));

  if(t == 0) {
    write_vector_length = db.vector_length/8+1;
    db.type = BINARY_VECTOR;
  } else if(t == 1) {    
    write_vector_length = db.vector_length*sizeof(double);
    db.type = DOUBLE_VECTOR;
  }

  db.vector = (char**)malloc(sizeof(char*)*db.n_entries);
  db.text = (char**)malloc(sizeof(char*)*db.n_entries);
  
  for(i=0;i<db.n_entries;i++) {
    if(db.type == BINARY_VECTOR) {
      posix_memalign((void**)(db.vector+i),32,
		     sizeof(char)*(write_vector_length
				   +(32-write_vector_length%32)));
      memset(db.vector[i],0,
	     write_vector_length+(32-write_vector_length%32));
    } else if(db.type == DOUBLE_VECTOR) {
      posix_memalign((void**)(db.vector+i),32,
		     write_vector_length+(32-write_vector_length%32));
      memset(db.vector[i],0,
	     write_vector_length+(32-write_vector_length%32));
      
    }
    read(fd,db.vector[i],write_vector_length);
  }
  
  for(i=0;i<db.n_entries;i++) {
    read(fd,&current_text_length,sizeof(size_t));
    db.text[i] = (char*)malloc(sizeof(char)*(current_text_length+1));
    read(fd,db.text[i],current_text_length);
    db.text[i][current_text_length]=0;
  }
  close(fd);
  return(db);
}

double cosine_distance(char* a, char* b, size_t vector_length) {

  double* a_d = (double*)a;
  double* b_d = (double*)b;

  __m256d a_in_b_vec = _mm256_setzero_pd();
  __m256d a_in_a_vec = _mm256_setzero_pd();
  __m256d b_in_b_vec = _mm256_setzero_pd();

  __m256d current_a;
  __m256d current_b;

  double double_buffer[4] __attribute__((aligned(32)));

  double a_in_b, a_in_a, b_in_b;
  
  int i,j, i_plus_j;

  vector_length+=(4-vector_length%4);
    
  for(i=0;i<vector_length;i+=4) {
    current_a = _mm256_load_pd(a_d+i);
    current_b = _mm256_load_pd(b_d+i);
    a_in_b_vec = _mm256_fmadd_pd(current_a,current_b,a_in_b_vec);
    a_in_a_vec = _mm256_fmadd_pd(current_a,current_a,a_in_a_vec);
    b_in_b_vec = _mm256_fmadd_pd(current_b,current_b,b_in_b_vec);
  }

  _mm256_store_pd(double_buffer, a_in_b_vec);
  a_in_b = double_buffer[0]+double_buffer[1]+double_buffer[2]+double_buffer[3];

  _mm256_store_pd(double_buffer, a_in_a_vec);
  a_in_a = double_buffer[0]+double_buffer[1]+double_buffer[2]+double_buffer[3];

  _mm256_store_pd(double_buffer, b_in_b_vec);
  b_in_b = double_buffer[0]+double_buffer[1]+double_buffer[2]+double_buffer[3];
  
  return(1-(a_in_b/(sqrt(a_in_a)*sqrt(b_in_b))));
}

double Yule_distance_scalar(char* a, char* b, size_t vector_length) {

  size_t i;
  long in_a_in_b = 0;
  long in_a_not_b = 0;
  long not_a_in_b = 0;
  long not_a_not_b = 0;

  int a_now, b_now;
  long R;
  double R_d_half, R_d, result;
  
  for(i=0;i<vector_length;i++) {
    a_now = get_value_in_binary_array_at_index(a,i);
    b_now = get_value_in_binary_array_at_index(b,i);

    if ( a_now == 1 && b_now == 1) in_a_in_b+=1;
    if ( a_now == 1 && b_now == 0) in_a_not_b+=1;
    if ( a_now == 0 && b_now == 1) not_a_in_b+=1;
    if ( a_now == 0 && b_now == 0) not_a_not_b+=1;
  }
  R = in_a_not_b*not_a_in_b;
  R_d_half = (double)R;
  R_d = R_d_half*2.;

  result = (R_d/((double)in_a_in_b*(double)not_a_not_b+R_d_half));
  return(result);
}

double Yule_distance(char* a,char* b,size_t vector_length) {

  int i;
  __m256i vec_a;
  __m256i vec_b;

  long in_a_in_b = 0;
  long in_a_not_b = 0;
  long not_a_in_b = 0;
  long not_a_not_b = 0;

  long n_bits = vector_length;

  size_t internal_vector_length = vector_length/8+1;
  
  __m256i vec_and_buffer, vec_not_a_and_b, vec_not_b_and_a;

  __m256i* current_address_a;
  __m256i* current_address_b;
  long long_buffer[4] __attribute__((aligned(32)));
  long count_buffer;

  long R;
  double result;
  double R_d, R_d_half;
  
  internal_vector_length+=internal_vector_length%32;

  current_address_a = (__m256i*)a;
  current_address_b = (__m256i*)b;
  
  for(i=0;i<internal_vector_length/32;i++) {
    vec_a = _mm256_load_si256(current_address_a+i);
    vec_b = _mm256_load_si256(current_address_b+i);
    
    vec_and_buffer = _mm256_and_si256(vec_a,vec_b);

    vec_not_a_and_b = _mm256_andnot_si256(vec_a,vec_b);
    vec_not_b_and_a = _mm256_andnot_si256(vec_b,vec_a);
    
    _mm256_store_si256((__m256i*)long_buffer,vec_and_buffer);
    
    count_buffer = _mm_popcnt_u64(long_buffer[0]);
    in_a_in_b += count_buffer;
    count_buffer = _mm_popcnt_u64(long_buffer[1]);
    in_a_in_b += count_buffer;
    count_buffer = _mm_popcnt_u64(long_buffer[2]);
    in_a_in_b += count_buffer;
    count_buffer = _mm_popcnt_u64(long_buffer[3]);
    in_a_in_b += count_buffer;

    _mm256_store_si256((__m256i*)long_buffer,vec_not_a_and_b);

    count_buffer = _mm_popcnt_u64(long_buffer[0]);
    not_a_in_b += count_buffer;
    count_buffer = _mm_popcnt_u64(long_buffer[1]);
    not_a_in_b += count_buffer;
    count_buffer = _mm_popcnt_u64(long_buffer[2]);
    not_a_in_b += count_buffer;
    count_buffer = _mm_popcnt_u64(long_buffer[3]);
    not_a_in_b += count_buffer;

    _mm256_store_si256((__m256i*)long_buffer,vec_not_b_and_a);
    
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
  
  R = in_a_not_b*not_a_in_b;
  R_d_half = (double)R;
  R_d = R_d_half*2.;

  result = (R_d/((double)in_a_in_b*(double)not_a_not_b+R_d_half));
  return(result);
  
}

void print_intra_distances_vdb(FILE*f,database db) {

  size_t i,j;

  double (*distance)(char*, char*, size_t);

  if(db.type == BINARY_VECTOR) {
    distance = &Yule_distance;
  } else if (db.type == DOUBLE_VECTOR) {
    distance = &cosine_distance;
  }
  
  for(i=0;i<db.n_entries;i++) {
    for(j=0;j<=i;j++) {
      fprintf(f,"%lu %lu %6.5E\n",i,j,(*distance)(db.vector[i],
						 db.vector[j],
						 db.vector_length));
    }
  }
}

void* calculate_distance_matrix_tread_handler(void* handle) {

  size_t i,j;

  thread_handle* h = (thread_handle*)handle; 
  
  database* db = h->db;
  double* matrix = h->matrix;
  
  size_t index;

  double (*distance)(char*,char*,size_t);

  if(db->type == BINARY_VECTOR) {
    distance = &Yule_distance;
  } else if (db->type == DOUBLE_VECTOR) {
    distance = &cosine_distance;
  }
  
 thread_continuation:

  pthread_mutex_lock(&distance_matrix_lock);
  j = h->incrementor[0];
  h->incrementor[0]++;
  pthread_mutex_unlock(&distance_matrix_lock);
  
  if( j < db->n_entries ) {
    for(i=0;i<=j;i++) {
      index = i*(2*db->n_entries-i+1)/2+(j*i);
      matrix[index] = (*distance)(db->vector[i],
				  db->vector[j],
				  db->vector_length);
    }
    goto thread_continuation;
  }
}

double* calculate_distance_matrix(database db, size_t n_threads) {

  size_t i,j;

  size_t incrementor = 0;

  pthread_t* threads = (pthread_t*)malloc(sizeof(pthread_t)*n_threads);
  
  thread_handle h;
  h.incrementor = &incrementor;
  h.matrix =
    (double*)malloc(sizeof(double)*(db.n_entries*(db.n_entries+1)/2));

  pthread_mutex_init(&distance_matrix_lock,NULL);

  for(i=0; i<n_threads;i++) {
    pthread_create(threads+i,
		   NULL,
		   calculate_distance_matrix_tread_handler,
		   &h);
  }

  for(i=0; i<n_threads;i++) {
    pthread_join(threads[i],NULL);
  }

  free(threads);
  
  return(h.matrix);
}
  

size_t* create_closest_distances(database db,
				 double(*distance)(char*,char*,size_t),
				 char* query, size_t n_closest) {
  
  int i;
  size_t* closest_distances = (size_t*)malloc(sizeof(size_t)*n_closest);

  distances* d = (distances*)malloc(sizeof(distances)*db.n_entries);
  
  char ** vectors = db.vector;
  size_t n_entries = db.n_entries;
  
  if(db.n_entries < n_closest) {
    fprintf(stderr,"Less database entries than closest distances searched!");
    _exit(1);
  }   

  for(i=0;i<db.n_entries;i++) {
    d[i].distance = distance(query,vectors[i],db.vector_length);
    d[i].index = i;
  }

  qsort(d,db.n_entries,sizeof(distances),d_comp);

  for(i=0;i<n_closest;i++) {
    closest_distances[i] = d[i].index;
  } 
  free(d);
  return(closest_distances);
}

void print_binary_embeddings(char* a, size_t vector_length) {

  size_t i;
  int v;
  printf("Binary embedding result: ");
  for(i=0;i<vector_length;i++) {
    v = get_value_in_binary_array_at_index(a,i);
    printf("%i",v);
  }
  printf("\n");
  
}

      
   

  
