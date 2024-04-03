mkdir bin

gcc -O2 -march=native -ftree-vectorize adaptive_clustering.c cluster.c dataset.c dbscan.c binary_array.c -pthread -D_COS_DISTANCE -D_POSIX_C_SOURCE -o bin/adaptive_text_cos -lm

gcc -O2 -march=native -ftree-vectorize adaptive_clustering.c cluster.c dataset.c dbscan.c binary_array.c -pthread -D_YULE_DISTANCE -D_POSIX_C_SOURCE -o bin/adaptive_text_yule -lm
