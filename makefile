CC=gcc
#CFLAGS=-O2 -march=arrowlake-s -mabm -mno-cldemote -mno-kl -mno-pconfig -mno-sgx -mno-widekl -mshstk -mwbnoinvd --param=l1-cache-line-size=64 --param=l1-cache-size=32 --param=l2-cache-size=30720 -ftree-vectorize -fomit-frame-pointer -Wno-unused-result -flto
CFLAGS=-O2 -g -march=native -ftree-vectorize -fomit-frame-pointer -Wno-unused-result -flto
#CFLAGS=-g -mavx2 -mfma 

MATH=-lm
JSON=-ljson-c
CURL=-lcurl
READLINE=-lreadline
PTHREAD=-pthread
LAPACK=-llapack

all: bin/create-tokenizer bin/build-vector-db bin/build-vector-db-from-server \
     bin/rag-with-vdb-yule bin/beir-corpus-test bin/embedding-from-server-cli \
     bin/rag-conversation bin/rag-with-vdb-cos-client \
     bin/build-vector-db-theses bin/build-theses-vector-db-from-server \
     bin/adaptive-text-cos bin/adaptive-text-yule \
     bin/show-vdb-details bin/print-vdb-distances \
     bin/pca-from-vdb bin/print-vdb-entry bin/print-vdb-texts \
     bin/yule-distance-test bin/split-set-to-vdbs bin/split-set-to-indices \
     bin/prompt-request bin/number_of_clusters_in_split_set

local_resolve.o: local_resolve.c local_resolve.h
	$(CC) $(CFLAGS) -c local_resolve.c -o local_resolve.o

curl_helpers.o: curl_helpers.c curl_helpers.h
	$(CC) $(CFLAGS) -c curl_helpers.c -o curl_helpers.o

binary_array.o: binary_array.c binary_array.h
	$(CC) $(CFLAGS) -c binary_array.c -o binary_array.o

word_list.o: word_list.c wordlist.h binary_array.h
	$(CC) $(CFLAGS) -c word_list.c -o word_list.o

vector-db.o: vector-db.c vector-db.h binary_array.h
	$(CC) $(CFLAGS) -c vector-db.c -o vector-db.o

load-texts.o: load-texts.c load-texts.h
	$(CC) $(CFLAGS) -c load-texts.c -o load-texts.o

cluster.o: cluster.c cluster.h vector-db.h binary_array.h
	$(CC) $(CFLAGS) -c cluster.c -o cluster.o

dbscan-cos.o: dbscan.c cluster.h dbscan.h binary_array.h \
              vector-db.h
	$(CC) $(CFLAGS) -D_COS_DISTANCE -c dbscan.c -o dbscan-cos.o

dbscan-yule.o: dbscan.c cluster.h dbscan.h binary_array.h \
               vector-db.h 
	$(CC) $(CFLAGS) -D_YULE_DISTANCE -c dbscan.c -o dbscan-yule.o 

embedding-from-server.o: embedding-from-server.c embedding-from-server.h
	$(CC) $(CFLAGS) -c embedding-from-server.c -o embedding-from-server.o

bin/create-tokenizer: create-tokenizer.c binary_array.o word_list.o wordlist.h
	$(CC) $(CFLAGS) create-tokenizer.c binary_array.o word_list.o \
 -o bin/create-tokenizer

bin/build-vector-db: build-vector-db.c vector-db.o word_list.o load-texts.o \
                     binary_array.o binary_array.h vector-db.h wordlist.h \
                     load-texts.h 
	$(CC) $(CFLAGS) build-vector-db.c vector-db.o word_list.o load-texts.o \
 binary_array.o \
 -o bin/build-vector-db $(MATH) $(JSON)

bin/build-vector-db-theses: build-theses-vector-db.c vector-db.o word_list.o \
                            load-texts.o binary_array.o binary_array.h \
                            wordlist.h load-texts.h
	$(CC) $(CFLAGS) build-theses-vector-db.c vector-db.o word_list.o \
 load-texts.o binary_array.o \
 -o bin/build-vector-db-theses $(MATH) $(JSON)

bin/rag-with-vdb-yule: multirag.c word_list.o vector-db.o vector-db.h \
                       wordlist.h binary_array.o binary_array.h \
                       local_resolve.o local_resolve.h \
                       curl_helpers.o curl_helpers.h
	$(CC) -D_RAG_WITH_YULE $(CFLAGS) multirag.c vector-db.o \
 word_list.o binary_array.o local_resolve.o curl_helpers.o \
 -o bin/rag-with-vdb-yule $(JSON) $(CURL) $(READLINE) $(MATH)

bin/rag-with-vdb-cos-client: multirag.c vector-db.o vector-db.h binary_array.o \
                             binary_array.h local_resolve.o local_resolve.h \
                             curl_helpers.o curl_helpers.h \
                             embedding-from-server.o embedding-from-server.h
	$(CC) -D_RAG_WITH_COS_SERVER $(CFLAGS) multirag.c vector-db.o \
 binary_array.o local_resolve.o curl_helpers.o embedding-from-server.o \
 -o bin/rag-with-vdb-cos-client $(JSON) $(CURL) $(READLINE) $(MATH)

bin/rag-conversation: multirag.c word_list.o vector-db.o vector-db.h \
	              wordlist.h binary_array.o binary_array.h \
                      local_resolve.o local_resolve.h \
                      curl_helpers.o curl_helpers.h
	$(CC) $(CFLAGS) multirag.c vector-db.o \
 word_list.o binary_array.o local_resolve.o curl_helpers.o \
 -o bin/rag-conversation $(JSON) $(CURL) $(READLINE) $(MATH)

bin/beir-corpus-test: beir-corpus-test.c vector-db.o vector-db.h word_list.o \
                      wordlist.h binary_array.o binary_array.h
	$(CC) $(CFLAGS) beir-corpus-test.c vector-db.o word_list.o \
 binary_array.o \
 -o bin/beir-corpus-test $(JSON) $(MATH)

bin/embedding-from-server-cli: embedding-from-server.o \
                               embedding-from-server.h local_resolve.o \
                               local_resolve.h curl_helpers.o curl_helpers.h
	$(CC) $(CFLAGS) embedding-from-server-cli.c embedding-from-server.o \
 local_resolve.o curl_helpers.o \
 -o bin/embedding-from-server-cli $(JSON) $(CURL) $(MATH)

bin/build-vector-db-from-server: build-vector-db-from-server.c \
                                 embedding-from-server.o \
                                 embedding-from-server.h local_resolve.o \
                                 local_resolve.h curl_helpers.o curl_helpers.h \
                                 vector-db.h vector-db.o \
                                 binary_array.h binary_array.o \
                                 load-texts.h load-texts.o 
	$(CC) $(CFLAGS) build-vector-db-from-server.c embedding-from-server.o \
 local_resolve.o curl_helpers.o vector-db.o binary_array.o load-texts.o \
 -o bin/build-vector-db-from-server $(JSON) $(CURL) $(MATH)

bin/build-theses-vector-db-from-server: build-theses-vector-db-from-server.c \
                                        embedding-from-server.o \
                                        embedding-from-server.h \
                                        local_resolve.o local_resolve.h \
                                        curl_helpers.o curl_helpers.h \
                                        vector-db.h vector-db.o \
                                        binary_array.h binary_array.o \
                                        load-texts.o load-texts.h
	$(CC) $(CFLAGS) build-theses-vector-db-from-server.c \
 embedding-from-server.o local_resolve.o curl_helpers.o \
 vector-db.o binary_array.o load-texts.o \
 -o bin/build-theses-vector-db-from-server $(JSON) $(CURL) $(MATH)

bin/show-vdb-details: get-vdb-details.c \
                      vector-db.h vector-db.o binary_array.o binary_array.h 
	$(CC) $(CFLAGS) get-vdb-details.c vector-db.o binary_array.o \
 $(MATH) -o bin/show-vdb-details 

bin/adaptive-text-cos: adaptive_clustering.c dbscan-cos.o dbscan.h \
                       cluster.o cluster.h vector-db.c vector-db.o \
                       binary_array.o binary_array.h
	$(CC) $(CFLAGS) adaptive_clustering.c dbscan-cos.o cluster.o \
 vector-db.o binary_array.o -D_COS_DISTANCE \
 -o bin/adaptive-text-cos $(PTHREAD) $(MATH)

bin/adaptive-text-yule: adaptive_clustering.c dbscan-yule.o dbscan.h \
                        cluster.o cluster.h vector-db.c vector-db.o \
                        binary_array.o binary_array.h 
	$(CC) $(CFLAGS) adaptive_clustering.c dbscan-yule.o cluster.o \
 vector-db.o binary_array.o -D_YULE_DISTANCE \
 -o bin/adaptive-text-yule $(PTHREAD) $(MATH)

bin/print-vdb-distances: print-vdb.c vector-db.o vector-db.h \
                         binary_array.o binary_array.h
	$(CC) $(CFLAGS) print-vdb.c vector-db.o binary_array.o \
 -o bin/print-vdb-distances $(MATH)

bin/pca-from-vdb: pca-from-vdb.c vector-db.o vector-db.h \
                  binary_array.o binary_array.h
	$(CC) $(CFLAGS) pca-from-vdb.c vector-db.o binary_array.o \
 -o bin/pca-from-vdb $(MATH) $(PTHREAD) $(LAPACK)

bin/print-vdb-entry: print-vdb-entry.c vector-db.o vector-db.h \
                     binary_array.o binary_array.h
	$(CC) $(CFLAGS) print-vdb-entry.c vector-db.o binary_array.o \
 -o bin/print-vdb-entry $(MATH)

bin/print-vdb-texts: print-vector-db-texts.c vector-db.o vector-db.h \
                     binary_array.o binary_array.h
	$(CC) $(CFLAGS) print-vector-db-texts.c vector-db.o binary_array.o \
 -o bin/print-vdb-texts $(MATH)

bin/yule-distance-test: yule-distance-test.c vector-db.o vector-db.h \
                        binary_array.o binary_array.h
	$(CC) $(CFLAGS) yule-distance-test.c vector-db.o binary_array.o \
 -o bin/yule-distance-test $(MATH)

bin/split-set-to-vdbs: split_set_to_vdbs.c vector-db.o vector-db.h \
                       cluster.o cluster.h binary_array.o binary_array.h
	$(CC) $(CFLAGS) split_set_to_vdbs.c vector-db.o cluster.o \
 binary_array.o \
 -o bin/split-set-to-vdbs $(MATH)

bin/split-set-to-indices: split_set_to_indices.c vector-db.o vector-db.h \
                          cluster.o cluster.h binary_array.o binary_array.h
	$(CC) $(CFLAGS) split_set_to_indices.c vector-db.o cluster.o \
 binary_array.o \
 -o bin/split-set-to-indices $(MATH)

bin/prompt-request: prompt-request.c local_resolve.h curl_helpers.h \
                    local_resolve.o curl_helpers.o
	$(CC) $(CFLAGS) prompt-request.c local_resolve.o curl_helpers.o \
 -o bin/prompt-request $(CURL) $(JSON)

bin/number_of_clusters_in_split_set: number_of_clusters_in_split_set.c \
                                     cluster.h cluster.o binary_array.o \
                                     binary_array.h vector-db.o vector-db.h
	$(CC) $(CFLAGS) number_of_clusters_in_split_set.c \
 vector-db.o cluster.o binary_array.o \
 -o bin/number_of_clusters_in_split_set $(MATH)

clean:
	rm *.o bin/*

