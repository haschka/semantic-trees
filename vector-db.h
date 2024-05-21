typedef enum { BINARY_VECTOR, DOUBLE_VECTOR } vector_type;

typedef struct {
  char ** vector;
  char ** text;
  vector_type type;
  size_t vector_length;
  size_t n_entries;
} database;
