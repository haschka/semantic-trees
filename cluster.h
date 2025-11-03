
typedef struct {
  int * members;  /*!< integer containing the number of each sample that is
		   *   part of this cluster */
  int n_members;  /*!< number of samples represented in this cluster */
  int id;         /*!< unique cluster id in a set of clusters */
} cluster;

/*! \brief Structure storing multiple clusters of a dataset to be found by
 *         clustering algorithms such as dbscan.
 */
typedef struct {
  int n_clusters; /*!< number of clusters to be contained in this set */
  cluster* clusters; /*!< the clusters contained within this set */
} split_set;

/*! \brief Structure for storing connections between clusters, i.e. to build
 *         dodendograms
 */
typedef struct {
  int* connections;  /*!< array holding the connections for a single cluster */
  int n_connections; /*!< the number of connections for a single cluster */
} cluster_connections;

void create_cluster_files(char* prefix, split_set s, database ds);

split_set read_split_set(char* filename);
void store_split_set(char* filename, split_set s);
void write_split_set_to_db_indices(char* prefix, split_set s);
void write_split_set_as_vdbs(char* prefix, split_set s, database db);


cluster_connections*
generate_split_set_relation(split_set ancient, split_set new);

cluster data_not_in_clusters(split_set s, database ds);

void free_split_set_and_associated_clusters(split_set s);
