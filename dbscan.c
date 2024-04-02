#include<unistd.h>
#include<stdio.h>
#include<stdlib.h>
#include<math.h>
#include<pthread.h>
#include<string.h>

#include"dataset.h"
#include"cluster.h"
#include"binary_array.h"
#include"dbscan.h"

typedef struct {
  split_set (*dbscanner) (dataset, float, int);
  dataset ds;
  float* epsilon_now;
  float epsilon_inc;
  int minpts;
  int* split_set_index;
  pthread_mutex_t* lock_stop;
  pthread_mutex_t* lock_eps;
  pthread_mutex_t* lock_split_sets;
  int* stop;
  split_set** split_sets;
  int* stopindex;
} thread_handler_adaptive_scan;

static inline double Yule_distance(char* a,char* b, size_t length) {
  size_t i;
  long long int in_a_not_b = 0;
  long long int in_a_in_b = 0;
  long long int not_a_in_b = 0;
  long long int not_a_not_b = 0;
  double a_at_i;
  double b_at_i;
  int a_true;
  int b_true;
  long long int R;
  
  for(i = 0;i<length;i++) {
    a_true = get_value_in_binary_array_at_index(a,i);
    b_true = get_value_in_binary_array_at_index(b,i);
  
    if ( a_true && b_true ) {
      in_a_in_b++;
    } else if ( a_true ) {
      in_a_not_b++;
    } else if ( b_true ) {
      not_a_in_b++;
    } else {
      not_a_not_b++;
    }
  }
  R = 2 * in_a_not_b * not_a_in_b;
  return((double) R /((double)in_a_in_b*(double)not_a_not_b));
}

static inline double cos_distance(double* a, double* b, size_t length) {
  size_t i;

  double sum_a = 0.0;
  double c_a = 0.0;
  double y_a,t_a;

  double sum_b = 0.0;
  double c_b = 0.0;
  double y_b,t_b;

  double sum_ab = 0.0;
  double c_ab = 0.0;
  double y_ab,t_ab;
  
  double a_value;
  double b_value;
  double a_times_b;
  double input_a, input_b, input_ab;

  for(i=0;i<length;i++) {
    a_value =  a[i];
    b_value =  b[i];
    
    input_ab = a_value*b_value;
    y_ab = input_ab - c_ab;
    t_ab = sum_ab + y_ab;
    c_ab = ( t_ab - sum_ab ) - y_ab;
    sum_ab = t_ab;

    input_a = a_value*a_value;
    y_a = input_a - c_a;
    t_a = sum_a + y_a;
    c_a = ( t_a - sum_a ) - y_a;
    sum_a = t_a;

    input_b = b_value*b_value;
    y_b = input_b - c_b;
    t_b = sum_b + y_b;
    c_b = ( t_b - sum_b ) - y_b;
    sum_b = t_b;
  }
  return(1-(sum_ab/(sqrt(sum_a)*sqrt(sum_b))));
}

static inline neighbors region_query(int point, float epsilon, dataset ds) {

  int i,j,k;
  neighbors nb;

  double distance;

#if defined (_COS_DISTANCE)
  double* a_point;
  double* b_point;
#elif defined (_YULE_DISTANCE)
  char* a_point;
  char* b_point;
#endif
  
  a_point = ds.values[point];
  
  nb.n_members = 0;
  for(i=0;i<ds.n_values;i++) {

    b_point = ds.values[i];

#if defined (_COS_DISTANCE)
    distance = cos_distance(a_point, b_point, ds.n_dimensions);
#elif defined (_YULE_DISTANCE)
    distance = Yule_distance(a_point, b_point, ds.n_dimensions);
#endif
  }
  
  if(distance <= epsilon) {
    nb.members[nb.n_members]=i;
    nb.n_members++;
  }
  return(nb);
}
    
    
static inline void expand_cluster(int point,
				  neighbors nb,
				  cluster* cl,
				  float epsilon,
				  int minpts,
				  char* visited,
				  int* cluster_member,
				  dataset ds) {

  int i,j;
  neighbors nb_of_nb;
  int * merge;
  int * right_in_left;
  int n_right_in_left;
  int merge_counter;
  int current_point;

  int add_to_unsorted_counter;
  int right_in_left_counter;

  int max_members_nb_nb_of_nb;
  int left;
  int right;
  int n_left;
  int n_right;

  neighbors nb_unsorted;

  nb_unsorted.members = (int*)malloc(sizeof(int)*ds.n_values);
  memset(nb_unsorted.members,0,sizeof(int)*ds.n_values);
  
  merge = (int*)malloc(ds.n_values*sizeof(int));

  memcpy(nb_unsorted.members,nb.members,sizeof(int)*nb.n_members);
  nb_unsorted.n_members = nb.n_members;

  cl->members = (int*)malloc(sizeof(int)*ds.n_values);
  cl->members[cl->n_members] = point;
  cluster_member[point] = 1;
  cl->n_members++;

  for(i=0;i < nb_unsorted.n_members;i++) {
    if(!get_value_in_binary_array_at_index(visited,
					   (size_t)nb_unsorted.members[i])) {
      set_value_in_binary_array_at_index(visited,
					 (size_t)nb_unsorted.members[i]);

      nb_of_nb = region_query(nb_unsorted.members[i], epsilon, ds);

      if (nb_of_nb.n_members >= minpts) {
	/*merge = (int*)malloc((nb.n_members+nb_of_nb.n_members)
	 *sizeof(int));*/

	right_in_left = (int*)malloc(nb_of_nb.n_members*sizeof(int));
	n_left = n_right = merge_counter = 0;
	n_right_in_left = 0;
	while(n_left < nb.n_members && n_right < nb_of_nb.n_members) {
	  left = nb.members[n_left];
	  right = nb_of_nb.members[n_right];
	  if( left < right ) {
	    merge[merge_counter] = left;
	    merge_counter++;
	    n_left++;
	  }
	  if( right < left ) {
	    merge[merge_counter] = right;
	    merge_counter++;
	    n_right++;
	  }
	  if( right == left ) {
	    merge[merge_counter] = left;
	    n_left++;
	    n_right++;
	    merge_counter++;
	    right_in_left[n_right_in_left] = left;
	    n_right_in_left++;
	  }
	}
	if(n_left != nb.n_members || n_right != nb_of_nb.n_members) {
	  if(n_left == nb.n_members) { /* Hit the left wall */
	    for(j=n_right;j<nb_of_nb.n_members;j++) {
	      merge[merge_counter] = nb_of_nb.members[j];
	      merge_counter++;
	    }
	  }
	  if(n_right == nb_of_nb.n_members) { /*Hit the right wall */
	    for(j=n_left;j<nb.n_members;j++) {
	      merge[merge_counter] = nb.members[j];
	      merge_counter++;
	    }
	  }
	}

	
	add_to_unsorted_counter = 0;
	right_in_left_counter = 0;
	/* if there are rights in left */
	if (n_right_in_left) {
	  for(j=0;j<nb_of_nb.n_members;j++) {

	    if(right_in_left[right_in_left_counter] != nb_of_nb.members[j]) {

	      nb_unsorted.members[nb_unsorted.n_members
				  +add_to_unsorted_counter] =
		nb_of_nb.members[j];

	      add_to_unsorted_counter++;

	    } else {
	      right_in_left_counter++;
	    }
	  }
	  /* if there are no rights in left */
	} else {
	  for(j=0;j<nb_of_nb.n_members;j++) {
	    nb_unsorted.members[nb_unsorted.n_members+j] = nb_of_nb.members[j];
	  }
	}
	free(right_in_left);
	free(nb_of_nb.members);
	nb_unsorted.n_members = merge_counter;

	  } else {
	free(nb_of_nb.members);
      }
    }
    if(!cluster_member[nb_unsorted.members[i]]) {
      /*      cl->members = (int*)realloc((void*)(cl->members)
	      ,sizeof(int)*(cl->n_members+1));*/
      cl->members[cl->n_members] = nb_unsorted.members[i];
      cl->n_members++;
      cluster_member[nb_unsorted.members[i]] = 1;

    }
  }
  cl->members=(int*)realloc((void*)(cl->members),sizeof(int)*(cl->n_members));

  free(nb_unsorted.members);
  free(nb.members);
  free(merge);
}

split_set dbscan(dataset ds, float epsilon, int minpts) {

  int i;
  neighbors nb;
  split_set ret_val;
  cluster* clusters = (cluster*)malloc(sizeof(cluster)*ds.n_values);
  char* visited = alloc_and_set_zero_binary_array(ds.n_values);
  int n_clusters = 0;
  int* cluster_member = (int*)malloc(sizeof(int)*ds.n_values);
  
  if (epsilon == -1) {
    printf("Warning @dbscan: epsilon value is: %f\n", epsilon);
    epsilon = 0.015;
    printf("Warning @dbscan: epsilon was changed to: %f\n", epsilon);
  }
  if (minpts == -1) {
    printf("Warning @dbscan: minpts value is: %i\n", minpts);
    minpts = 5;
    printf("Warning @dbscan: minpts was changed to: %i\n", minpts);
  }

  ret_val.clusters = (cluster*)malloc(sizeof(cluster)*ds.n_values);

  memset(cluster_member,0,sizeof(int)*ds.n_values);

  for(i=0;i<ds.n_values;i++) {
    //    clusters[i].members = NULL;
    clusters[i].n_members = 0;
    clusters[i].id = i;
  }

  for(i=0;i<ds.n_values;i++) {
    if(!get_value_in_binary_array_at_index(visited,(size_t)i)) {
      set_value_in_binary_array_at_index(visited,(size_t)i);
      nb = region_query(i, epsilon, ds);
      if ( minpts < nb.n_members ) {

	expand_cluster(i,nb,clusters+(n_clusters),epsilon,minpts,
		       visited,cluster_member,ds);
	n_clusters++;

      } else {

	free(nb.members);

      }
    }
  }
  
  ret_val.n_clusters = 0;
  for(i=0;i<n_clusters;i++) {
    //if(clusters[i].n_members) {
    memcpy(ret_val.clusters+ret_val.n_clusters,clusters+i,sizeof(cluster));
    ret_val.n_clusters++;
    //}
  }
  free(clusters);
  free(visited);
  free(cluster_member);
  
  return(ret_val);
}
  
void* adaptive_dbscan_thread(void* arg) {

  thread_handler_adaptive_scan* th = (thread_handler_adaptive_scan*)arg;

  dataset ds = th->ds;
  float epsilon;
  int index;

  split_set current_split_set;
  
  while(1) {

    if(th->stop[0]) {
      goto finish;
    }
    
    pthread_mutex_lock(th->lock_eps);
    
    epsilon = th->epsilon_now[0];
    th->epsilon_now[0] = th->epsilon_now[0] + th->epsilon_inc;
  
    pthread_mutex_unlock(th->lock_eps);

    current_split_set.n_clusters = 0;
    current_split_set.clusters = NULL;
    
    current_split_set = th->dbscanner(ds, epsilon, th->minpts);
    
    pthread_mutex_lock(th->lock_split_sets);
    index = th->split_set_index[0];
    th->split_set_index[0]++;
    th->split_sets[0] =
      (split_set*)realloc(th->split_sets[0],
			  sizeof(split_set)*th->split_set_index[0]);
    th->split_sets[0][index] = current_split_set;
    pthread_mutex_unlock(th->lock_split_sets);
    
    if(th->split_sets[0][index].n_clusters == 1) {
      pthread_mutex_lock(th->lock_stop);
      th->stopindex[0] = index;
      th->stop[0] = 1;
      pthread_mutex_unlock(th->lock_stop);
      goto finish;
    }
  }
 finish:
  return NULL;
}


void adaptive_dbscan(split_set (*dbscanner) (dataset,
					     float,
					     int),
		     dataset ds,
		     float epsilon_start,
		     float epsilon_inc,
		     int minpts,
		     char* split_files_prefix,
		     int n_threads) {

    int i,j,k;
  
  int initial_counter, count, eps_count;
  
  split_set* set_of_split_sets;
  cluster_connections** connections = NULL;
  cluster_connections* current_connection;

  cluster not_covered;

  float coverage;
  
  split_set new_split_set;

  char split_files[255];
  char buffer[20];

  pthread_t* threads = (pthread_t*)malloc(sizeof(pthread_t)*n_threads);
  pthread_mutex_t* lock_eps = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
  pthread_mutex_t* lock_stop =
    (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
  pthread_mutex_t* lock_split_sets =
    (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
  int * stop = (int*)malloc(sizeof(int));

  thread_handler_adaptive_scan* th =
    (thread_handler_adaptive_scan*)
    malloc(sizeof(thread_handler_adaptive_scan)*n_threads);

  printf("Performing adaptive clustering with parameters: \n"
	 "minpts: %i\n"
	 "epsilon_start: %f\n"
	 "epsilon_inc: %f\n", minpts, epsilon_start, epsilon_inc);
  
  set_of_split_sets = (split_set*)malloc(sizeof(split_set));

  set_of_split_sets[0] = dbscanner(ds, epsilon_start, minpts);

  printf("Initial set obtained with %i clusters\n",
	  set_of_split_sets[0].n_clusters);

  initial_counter = 0;
  while( set_of_split_sets[0].n_clusters == 1 ) {
    if (initial_counter == 20 || epsilon_start == 0) {
      printf("Error did not find a sufficient" 
	     " starting position in 20 tries \n");
      _exit(1);
    }
    epsilon_start = epsilon_start/2;
    printf("Trying new starting point \n");

    free_split_set_and_associated_clusters(set_of_split_sets[0]);

    set_of_split_sets[0] = dbscanner(ds, epsilon_start, minpts);

    initial_counter++;

  }

    printf("Sarting with %i clusters\n", set_of_split_sets[0].n_clusters);
  
  not_covered = data_not_in_clusters(set_of_split_sets[0], ds);

  if(not_covered.n_members > 0) {
	free(not_covered.members);
  }

  printf("Coverage at initial point: %f\n",
	 (float)1.f-(float)not_covered.n_members/(float)ds.n_values);

  if(set_of_split_sets[0].n_clusters == 1) {
    printf("All clusters fusioned in one step, decrease epsilon increment\n");
    _exit(1);
  }

  eps_count = 1;

  pthread_mutex_init(lock_stop,NULL);
  pthread_mutex_init(lock_eps,NULL);
  pthread_mutex_init(lock_split_sets,NULL);
  
  stop[0] = 0;

  th[0].dbscanner = dbscanner;
  th[0].ds = ds;
  th[0].epsilon_now = (float*)malloc(sizeof(float));
  th[0].epsilon_now[0] = epsilon_start+epsilon_inc;
  th[0].epsilon_inc = epsilon_inc;
  th[0].minpts = minpts;
  th[0].split_set_index = (int*)malloc(sizeof(int));
  th[0].split_set_index[0] = 1;
  th[0].lock_stop = lock_stop;
  th[0].lock_eps = lock_eps;
  th[0].lock_split_sets = lock_split_sets;
  th[0].stop = stop;
  th[0].split_sets = (split_set**)malloc(sizeof(split_set*));
  th[0].split_sets[0] = (split_set*)malloc(sizeof(split_set));

  memcpy(th[0].split_sets[0],set_of_split_sets,sizeof(split_set));

  th[0].stopindex = (int*)malloc(sizeof(int));
  th[0].stopindex[0];

  
  for(i=1;i<n_threads;i++) {
    memcpy(th+i,th,sizeof(thread_handler_adaptive_scan));
  }
  
  for(i=0;i<n_threads;i++) {
    pthread_create(threads+i, NULL,adaptive_dbscan_thread,th+i);
  }
  
  for(i=0;i<n_threads;i++) {
    pthread_join(threads[i],NULL);
  }

  pthread_mutex_destroy(lock_stop);
  pthread_mutex_destroy(lock_eps);

  count = 0;
  
  for(i=1;i<=th->stopindex[0];i++) {
    
    new_split_set = th->split_sets[0][i];
    if (new_split_set.n_clusters < set_of_split_sets[count].n_clusters) {
      
      not_covered = data_not_in_clusters(new_split_set, ds);
      
      printf("Layer %i has %i clusters\n",count+1, new_split_set.n_clusters);
      
      printf("  Coverage at layer %i: %f\n", count+1,
	     1.f-(float)not_covered.n_members/(float)ds.n_values);

      if(not_covered.n_members > 0) {
	free(not_covered.members);
      }

      printf("  Epsilon at layer %i: %f\n", count+1,
	     epsilon_start+(i+1)*(float)epsilon_inc);
	          
      current_connection = generate_split_set_relation(set_of_split_sets[count],
						       new_split_set);
      count++;
      set_of_split_sets = (split_set*)realloc(set_of_split_sets,
					      sizeof(split_set)*(count+1));
      connections =
	(cluster_connections**)realloc(connections,
				       sizeof(cluster_connections*)*
				       count);
      
      set_of_split_sets[count] = new_split_set;
      connections[count-1] = current_connection;
    }
  }

  printf("Connections found: \n");
  for(i=0;i<(count+1);i++) {
    
    sprintf(buffer,"%04d",i);
    memcpy(split_files,split_files_prefix,
	   strlen(split_files_prefix)+1);
    strcat(split_files,buffer);

    store_split_set(split_files, set_of_split_sets[i]);
  }

  for(i=count;i>0;i--) {
    printf("Layer %i:\n", i);
    for(j=0;j<set_of_split_sets[i].n_clusters;j++) {
      printf("  Cluster %i connected to: \n  ",j);
      for(k=0;k<connections[i-1][j].n_connections;k++) {
	printf("%i ", connections[i-1][j].connections[k]);
      }
      printf("\n");
    }
  }

  for(i=count;i>0;i--) {
    for(j=0;j<set_of_split_sets[i].n_clusters;j++) {
      free(connections[i-1][j].connections);
    }
    free(connections[i-1]);
  }
  free(connections);
  
  for(i=0;i<(count+1);i++) {
    free_split_set_and_associated_clusters(set_of_split_sets[i]);
  }
  free(set_of_split_sets);
}
  
