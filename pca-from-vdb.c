#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <pthread.h>
#include <string.h>

#include "vector-db.h"

#ifdef __AVX__

#include <immintrin.h>

#endif

void dsyev_(char *, char *, int *, double *, int *, double *, double *, int *,
            int *);

typedef struct {
  size_t n_features;
  size_t n_samples;
} data_shape;

typedef struct {
  double* eigenvalues;
  double* eigenvectors;
} eigen_space;

typedef struct {
  double *data;
  double *means;
  data_shape *shape;
  double* matrix;
  size_t* incrementor;
  size_t* dimensions;
  eigen_space* space;
} thread_handle;

pthread_mutex_t lock;

double* data_from_vdb(database vdb, data_shape shape) {

  size_t i,j;
  
  double* in_data =
    (double*)malloc(sizeof(double)*shape.n_features*shape.n_samples);

  char* current_sample;
  double* current_sample_d;
  
  for(i=0;i<shape.n_samples;i++) {
    current_sample = vdb.vector[i];
    current_sample_d = (double*)current_sample;
    for(j=0;j<shape.n_features;j++) {
      in_data[i*shape.n_features+j] = current_sample_d[j];
    }
    free(current_sample);
  }
  free(vdb.vector);
  return(in_data);
}

double* means_from_data(double* in_data, data_shape s) {

  size_t i, j;
  double sum;
  double* means = (double*)malloc(sizeof(double)*s.n_features);
  for( i = 0 ; i < s.n_features; i++ ) {
    sum = 0;
    for( j = 0 ; j < s.n_samples; j++ ) {
      sum += in_data[j*s.n_features+i];
    }
    means[i] = sum;
    means[i] /= (double)s.n_samples;
  }
  return(means);
}


data_shape shape_from_vector_db(database vdb) {

  data_shape shape;

  if (vdb.type == BINARY_VECTOR) {
    fprintf(stderr,
	    "Only Double Vector - Vector Databases are supported for"
	    "PCA calculations.");
    _exit(1);
  }
  
  shape.n_features = vdb.vector_length;
  shape.n_samples = vdb.n_entries;
  return(shape);
}

void* feature_covariance_matrix_thread_handler(void* handle) {

  size_t i,j,k, k_four;

  thread_handle* th = (thread_handle*)handle;
  
  size_t n_samples = th->shape->n_samples;
  size_t n_features = th->shape->n_features;

  double sum;

  double* data = th->data;
  double* means = th->means;
  double* c_matrix = th->matrix;
  
 thread_continuition:
  
  pthread_mutex_lock(&lock);
  i=th->incrementor[0];
  th->incrementor[0]++;
  pthread_mutex_unlock(&lock);

#ifdef __AVX__
  
  __m256d c = _mm256_set1_pd(0.);
  __m256d y;
  __m256d t;
  __m256d input_v;
  __m256d sum_v = _mm256_set1_pd(0.);
  __m256d input;

  __m256d means_i_v;
  __m256d means_j_v;
  __m256d data_i_v;
  __m256d data_j_v;
  
  double data_i[4] __attribute__((aligned(32)));
  double data_j[4] __attribute__((aligned(32)));

  double sum_buffer[4] __attribute__((aligned(32)));

#else
  
  double c;
  double y;
  double t;

#endif
  
  if(i<n_features) {

#ifdef __AVX__
    
    means_i_v = _mm256_set1_pd(means[i]);    

#endif
    
    for(j = 0; j <= i; j++) {

      sum = 0;

#ifdef __AVX__
      
      c = _mm256_set1_pd(0.);
      sum_v = _mm256_set1_pd(0.);
      
      means_j_v = _mm256_set1_pd(means[j]);
            
      for(k = 0; k < n_samples/4; k++) {
	k_four = 4*k;

	data_j[0] = data[ k_four   *n_features+j];
	data_j[1] = data[(k_four+1)*n_features+j];
	data_j[2] = data[(k_four+2)*n_features+j];
	data_j[3] = data[(k_four+3)*n_features+j];

	data_i[0] = data[ k_four   *n_features+i];
	data_i[1] = data[(k_four+1)*n_features+i];
	data_i[2] = data[(k_four+2)*n_features+i];
	data_i[3] = data[(k_four+3)*n_features+i];

	data_j_v = _mm256_load_pd(data_j);
	data_i_v = _mm256_load_pd(data_i);
	
	input = _mm256_sub_pd(data_j_v,means_j_v);
	input = _mm256_mul_pd(input,_mm256_sub_pd(data_i_v,means_i_v));

	y = _mm256_sub_pd(input,c);
	t = _mm256_add_pd(sum_v,y);
	c = _mm256_sub_pd(_mm256_sub_pd(t,sum_v),y);
	sum_v = t;
	
       /*sum += 
          (data[k*n_features+j]-means[j])*(data[k*n_features+i]-means[i]);*/

      }

      _mm256_store_pd(sum_buffer,sum_v);

      for(k = 0;k<4;k++) {
	sum+=sum_buffer[k];
      }

      // scalar padding without kahan as not needed for small sums
      if(!n_samples%4 && n_samples > 0) { 
	for(k = n_samples-(n_samples%4);k< n_samples;k++) {
	  sum+=
	    (data[k*n_features+j]-means[j])*(data[k*n_features+i]-means[i]);
	}
      }

#else
      c = 0.0;
      for(k = 0;k< n_samples;k++) {
	y = (data[k*n_features+j]-means[j])*(data[k*n_features+i]-means[i]);
	y = y - c;
	t = sum + y;
	c = ( t - sum ) - y;
	sum = t;
      }
      
#endif
      c_matrix[i*n_features+j] = c_matrix[j*n_features+i] = sum/n_samples;
    }
    goto thread_continuition; 
  }
}


double* feature_covariance_matrix(double* data, double* means, data_shape s,
				  size_t n_threads) {

  size_t i,j,k;

  size_t incrementor=0;
  pthread_t* threads = (pthread_t*)malloc(sizeof(pthread_t)*n_threads);
  
  double sum;

  double* c_matrix = (double*)malloc(sizeof(double)*s.n_features*s.n_features);

  thread_handle th;

  th.data = data;
  th.means = means;
  th.shape = &s;
  th.matrix = c_matrix;
  th.incrementor = &incrementor;
  
  pthread_mutex_init(&lock,NULL);
  
  for(i=0;i<n_threads;i++) {
    pthread_create(threads+i, NULL,
		   feature_covariance_matrix_thread_handler,
		   &th);
  }

  for(i=0;i<n_threads;i++) {
    pthread_join(threads[i], NULL);
  }
  free(threads);
  
  pthread_mutex_destroy(&lock);
  
  return(c_matrix); 
}

eigen_space eigen_from_corr(double* corr, data_shape s) {
  
  eigen_space ei;

  char JOBZ = 'V';
  char ULPO = 'U';
  int N = (int)s.n_features;
  double*A = (double*)malloc(sizeof(double)*s.n_features*s.n_features);
  int LDA = (int)s.n_features;
  double*W = (double*)malloc(sizeof(double)*s.n_features);
  double*WORK;
  int LWORK;
  int INFO;
  
  memcpy(A, corr, sizeof(double)*s.n_features*s.n_features);
  
  LWORK = 3*N;
  WORK = (double*)malloc(sizeof(double)*LWORK);

  dsyev_(&JOBZ, &ULPO, &N, A, &LDA, W, WORK, &LWORK, &INFO);

  /*  if (INFO) {
      printf("Error in diagonalization routine INFO=%ls \n", &INFO);
      _exit(1);
      }
  */ 
  ei.eigenvectors = A;
  ei.eigenvalues = W;

  free(WORK);

  return(ei);
}

void* obtain_projections_thread_handler(void* handle) {

  size_t i,j,k;
  thread_handle* th=(thread_handle*)handle;

  size_t dimensions = th->dimensions[0];

  size_t n_samples = th->shape->n_samples;
  size_t n_features = th->shape->n_features;

  double* projections = th->matrix;
  double* data = th->data;
  size_t position;
  eigen_space ei = th->space[0];
  
 thread_continuition:
  
  pthread_mutex_lock(&lock);
  j = th->incrementor[0];
  th->incrementor[0]++;
  pthread_mutex_unlock(&lock);
  
  if(j < n_samples) {
    for(i = 0; i < dimensions; i++) {
      position 	= j*dimensions+i;
      projections[position] = 0;
      for( k = 0 ; k < n_features; k++ ) {
	projections[position]+=
	  data[j*n_features+k]
	  *ei.eigenvectors[(n_features-1-i)*n_features+k];
      }
    }
    goto thread_continuition;
  }
}
  

double* obtain_projections(size_t dimensions, double* data,
			   eigen_space ei, data_shape s, size_t n_threads) {

  size_t incrementor = 0;
  size_t i,j,k;
  int position;
  
  double* projections = (double*)malloc(sizeof(double)*dimensions*s.n_samples);
  pthread_t* threads = (pthread_t*)malloc(sizeof(pthread_t)*n_threads);

  thread_handle th;

  th.shape = &s;
  th.dimensions = &dimensions;
  th.incrementor = &incrementor;
  th.matrix = projections;
  th.data = data;
  th.space = &ei;
  
  pthread_mutex_init(&lock, NULL);
  
  for(i=0; i<n_threads;i++) {
    pthread_create(threads+i, NULL,
		   obtain_projections_thread_handler,
		   &th);
  }

  for(i=0;i<n_threads;i++) {
    pthread_join(threads[i],NULL);
  }

  free(threads);
 
  return(projections);
}

void print_projections(FILE* f, double* projections,
		       size_t dimensions, data_shape s) {

  size_t i,j;
  
  for(j = 0; j < s.n_samples; j++) {
    for(i = 0; i < dimensions-1; i++) {
      fprintf(f,"%lf\t", projections[j*dimensions+i]);
    }
    fprintf(f,"%lf\n", projections[j*dimensions+(dimensions-1)]);
  }
}

void print_eigenvalues(FILE* f, data_shape s, eigen_space ei) {

  size_t i;

  for(i = 0; i < s.n_features; i++) {
    fprintf(f,"%lf\n",ei.eigenvalues[i]);
  }

}
  
int main(int argc, char** argv) {

  FILE* proj_file;
  FILE* eig_file;

  database vdb;
  
  double* in_data;
  double* normalized_data;
  double* corr_matrix;
  double* projections;
  double* corr;
  double* means;

  data_shape shape;
  eigen_space ei;

  size_t dimensions;
  size_t n_threads;

  if(argc < 5) {

    printf("Arguments are: \n"
	   " [FILE-in ] Vector Database to apply PCA on \n"
	   " [FILE-out] File to write vectors projected onto"
	   "principal components to\n"
	   " [FILE-out] Eigenvalue Spectrum of the PCA \n"
	   " [Integer ] Dimensions - How many principal components shall be"
	   "used for the projection\n"
	   " [Integer ] \n");
    return(1);

  }

  sscanf(argv[4], "%lu", &dimensions);
  sscanf(argv[5], "%lu", &n_threads);

  vdb = read_db_from_disk(argv[1]);

  shape = shape_from_vector_db(vdb);

  in_data = data_from_vdb(vdb,shape);
  
  printf("Read in data complete: shape: features %lu samples %lu\n",
	 shape.n_features, shape.n_samples);

  means = means_from_data(in_data, shape);

  printf("Means complete \n");

  corr = feature_covariance_matrix(in_data, means, shape, n_threads);

  free(means);

  printf("Covariance complete\n");
  
  ei = eigen_from_corr(corr, shape);
  free(corr);

  projections =  obtain_projections(dimensions, in_data,
				    ei, shape, n_threads);

  free(in_data);

  eig_file = fopen(argv[3], "w");
  if(eig_file == NULL) {
    fprintf(stderr, "Failed to open file! Unable to write eigenvalues!\n");
  } else {
    print_eigenvalues(eig_file, shape, ei);
    fclose(eig_file);
  }

  free(ei.eigenvalues);
  free(ei.eigenvectors);
  
  proj_file = fopen(argv[2], "w");
  if(proj_file == NULL) {
    fprintf(stderr, "Failed to open file! Unable to write projections!\n");
  } else {
    print_projections(proj_file, projections, dimensions, shape);
    fclose(proj_file);
  }
  free(projections);

  return(0);
}
