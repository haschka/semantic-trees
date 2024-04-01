gcc --std=c99 adaptive_clustering.c cluster.c dataset.c dbscan.c binary_array.c -pthread -D_COS_DISTANCE -D_POSIX_C_SOURCE -o adaptive_text_cos -lm
