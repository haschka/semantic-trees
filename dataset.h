typedef struct {
  size_t n_features;
  size_t n_samples;
} data_shape;

typedef struct {
  int n_values;
#if defined (_COS_DISTANCE)
  double** values;
#elif defined (_YULE_DISTANCE)
  char** values;
#endif
  int n_dimensions;
} dataset;

data_shape shape_from_embeddings_file(int infile);
dataset load_embeddings_from_file_into_dataset(FILE* in_file,
					       data_shape shape);

void free_dataset(dataset ds);
