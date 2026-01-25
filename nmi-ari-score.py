#!/usr/bin/env python3

import sys
import os
import json
import re
from collections import defaultdict

import numpy as np
from sklearn.metrics import adjusted_rand_score, normalized_mutual_info_score


def load_ground_truth(json_file, label_key):
    """
    Load ground-truth labels from a JSONL file.
    Returns a list indexed by line number.
    """
    labels = []
    with open(json_file, "r", encoding="utf-8") as f:
        for i, line in enumerate(f):
            obj = json.loads(line)
            if label_key not in obj:
                raise KeyError(
                    f"Label key '{label_key}' missing in JSONL line {i}"
                )
            labels.append(obj[label_key])
    return labels


def parse_clusters(clusters_dir):
    """
    Parse cluster files.
    Returns:
      layers[layer_id][doc_index] = cluster_id
    """
    pattern = re.compile(r"L(\d+)-C-(\d+)$")
    layers = defaultdict(dict)

    for fname in os.listdir(clusters_dir):
        match = pattern.match(fname)
        if not match:
            continue

        layer = int(match.group(1))
        cluster = int(match.group(2))
        cluster_label = f"L{layer:04d}_C{cluster:06d}"

        path = os.path.join(clusters_dir, fname)
        with open(path, "r") as f:
            for line in f:
                idx = int(line.strip())
                if idx in layers[layer]:
                    raise ValueError(
                        f"Document {idx} appears multiple times "
                        f"in layer {layer}"
                    )
                layers[layer][idx] = cluster_label

    if not layers:
        raise RuntimeError("No valid cluster files found.")

    return layers


def compute_scores(layers, ground_truth):
    """
    Compute ARI and NMI per layer.
    """
    results = []

    for layer in sorted(layers.keys()):
        doc_to_cluster = layers[layer]

        y_true = []
        y_pred = []

        for doc_idx, cluster_id in doc_to_cluster.items():
            if doc_idx >= len(ground_truth):
                raise IndexError(
                    f"Document index {doc_idx} out of range for JSONL file"
                )
            y_true.append(ground_truth[doc_idx])
            y_pred.append(cluster_id)

        if len(set(y_pred)) < 2:
            ari = float("nan")
            nmi = float("nan")
        else:
            ari = adjusted_rand_score(y_true, y_pred)
            nmi = normalized_mutual_info_score(
                y_true, y_pred, average_method="arithmetic"
            )

        results.append((layer, len(y_true), ari, nmi))

    return results


def main():
    if len(sys.argv) != 4:
        print(
            "Usage: python tree_ari_nmi.py "
            "JSON_FILE CLUSTERS_DIR LABEL_KEY"
        )
        sys.exit(1)

    json_file = sys.argv[1]
    clusters_dir = sys.argv[2]
    label_key = sys.argv[3]

    print("Loading ground-truth labels...")
    ground_truth = load_ground_truth(json_file, label_key)

    print("Parsing cluster files...")
    layers = parse_clusters(clusters_dir)

    print("Computing ARI / NMI per layer...\n")
    results = compute_scores(layers, ground_truth)

    print("Layer\tDocs\tARI\t\tNMI")
    print("-" * 50)
    for layer, n_docs, ari, nmi in results:
        print(
            f"{layer:04d}\t{n_docs}\t"
            f"{ari:.4f}\t{nmi:.4f}"
        )


if __name__ == "__main__":
    main()
