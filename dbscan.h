typedef struct {
  int * members; /*!< the features in this neighbourhood */
  int n_members; /*!< number of features in this neighborhood */
} neighbors;

split_set dbscan(dataset ds, float epsilon, int minpts);
void adaptive_dbscan(split_set (*dbscanner) (dataset,
					     float,
					     int),
		     dataset ds,
		     float epsilon_start,
		     float epsilon_inc,
		     int minpts,
		     char* split_files_prefix,
		     int n_threads);
