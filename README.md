# Semantic Tree Construction Tutorial

This repository contains code to build **hierarchical semantic trees**
from text datasets. The code works in conjunction with
[MNHN-Tree-Tools](https://treetools.haschka.net/).

If you make use of the code and/or datasets herein, please cite:
[Semantic Tree Inference on Text Corpa using a Nested Density Approach together with Large Language Model Embeddings](https://doi.org/10.48550/arXiv.2512.23471)

---

## Installation

Install the tools locally as follows.

### 1. Install dependencies and MNHN-Tree-Tools

```bash
sudo apt-get install git build-essential libpng-dev libsdl2-dev \
liblapack-dev libopenmpi-dev libpocl-dev ocl-icd-opencl-dev pocl-opencl-icd

git clone https://github.com/haschka/mnhn-tree-tools
cd mnhn-tree-tools
mkdir bin
make all
```

---

### 2. Install the `semantic-trees` add-on

```bash
git clone https://github.com/haschka/semantic-trees/
cd semantic-trees
mkdir bin
make all
```

---

### 3. Export paths (optional)

To simplify access to the compiled binaries, you may export the paths:

```bash
cd mnhn-tree-tools/bin
export PATH=$PATH:$PWD
cd ../../semantic-trees/bin
export PATH=$PATH:$PWD
```

---

## Running the Tools

### 1. Prerequisites

You should have a dataset of texts formatted as follows:

* The data must be in **JSONL** format.
* Each document must be stored on a single line.
* The text content must be provided under the key `resumes`.

A valid dataset therefore looks like:

```json
{ "resumes": "This is a text about something..." }
{ "resumes": "This is a text about something else..." }
```

---

### Hosting an Embedding Model

To build a vector database, you must host an embedding model. This can be
achieved either by running `llama.cpp` or by using one of the provided
Python scripts to launch an embedding server:

* `SFR.py` (SFR-Embedding-Mistral)
* `qwen3-embed-8b.py` (Qwen3-Embedding-8B)

Start the server with, for example:

```bash
python SFR.py
```

or

```bash
python qwen3-embed-8b.py
```

---

### Building the Vector Database

Once the embedding server is running, build the vector database:

```bash
build-theses-vector-db-from-server textdatabase.jsonl 127.0.0.1 8081 vectordatabase.vdb
```

---

### Creating a Pseudo-FASTA File

For compatibility with MNHN-Tree-Tools, it is convenient to create a
pseudo-FASTA file.

First, determine the number of records in the vector database:

```bash
show-vdb-details vectordatabase.vdb
```

Then generate the pseudo-FASTA file (replace `N` with the number of records):

```bash
for ((i=0;i<N;i++)); do
  echo ">seq_$i" >> /tmp/pseudofasta
  echo "ACGT" >> /tmp/pseudofasta
done
mv /tmp/pseudofasta .
```

---

## 2. Performing PCA on the Vector Database

To perform PCA on the vectors stored in the database, use the
`pca-from-vdb` tool:

```bash
pca-from-vdb vectordatabase.vdb pca-10-dim pca-ev 10 20
```

Where:

* `pca-10-dim` stores the projections onto the first 10 principal components,
* `pca-ev` stores the eigenvalues of the covariance matrix,
* `10` is the number of principal components,
* `20` is the number of available threads.

The PCA output can later be downscaled. For example, to reduce to two
dimensions:

```bash
awk '{print $1"\t"$2}' pca-10-dim > pca-2-dim
```

---

## 3. Adaptive Clustering on the PCA Projections

As an initial configuration, we use:

* ε = 0.01
* Δε = 0.00001
* `minpts` = 5

Finding suitable parameters can be challenging. In practice, identifying
a configuration that produces many clusters with approximately 30%
coverage is a good starting point.

Run adaptive clustering as follows:

```bash
mkdir layers
adaptive_cluster_PCA pseudofasta 0.01 0.000001 5 layers/L 20 2 pca-2-dim > logfile.log
```

Where:

* `layers/L` stores the clustering (*split-sets*) for each tree layer,
* `20` is the number of threads,
* `2` is the dimensionality of the PCA file.

The `logfile.log` reports:

* the number of clusters per layer,
* the dataset coverage,
* the parent–child relationships between clusters across layers.

---

## 4. Visualizing the Tree

First, generate a Newick file:

```bash
split_sets_to_newick 0 0 layers/L* > tree.dnd
```

Then visualize the tree using
[Newick Utilities](https://github.com/tjunier/newick_utils):

```bash
nw_display -sr -w 800 -i 'opacity:0' -l 'opacity:0' -b 'opacity:0' tree.dnd > tree.svg
```

Where:

* `-i` controls internal node annotations,
* `-l` controls leaf annotations,
* `-b` controls branch lengths (uniform in our trees).

Removing `opacity:0` enables annotation visualization. Increase `-w`
to avoid label overlap.

In this tutorial, internal nodes are labeled using the format
`LXXCYYNZZ`, where:

* `XX` is the layer number,
* `YY` is the cluster number within that layer,
* `ZZ` is the number of texts in the node.

---

## 5. Retrieving Texts for Specific Tree Nodes

Each tree node corresponds to a set of documents. To retrieve these,
first generate cluster index files:

```bash
mkdir clusters
for i in layers/L*; do
  split-set-to-indices $i clusters/$i-C
done
```

The `clusters` directory will contain files such as `L0004-C-000031`,
where:

* `0004` is the layer number,
* `000031` is the cluster number.

Each file lists the line numbers of the corresponding documents in the
original JSONL file.

To extract and aggregate the texts for each node:

```bash
mkdir texts
cd clusters
for i in *; do
  cat $i | print-vdb-texts vectordatabase.vdb > ../texts/$i
done
```

The resulting files can be used for downstream processing, such as
LLM-based annotation of tree nodes.
