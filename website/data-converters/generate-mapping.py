import os
import json
import sys

DATA_DIR = sys.argv[1]
LABEL_DIR = sys.argv[2]
OUTPUT_FILE = sys.argv[3]

result = {}

for filename in os.listdir(DATA_DIR):
    data_path = os.path.join(DATA_DIR, filename)
    label_path = os.path.join(LABEL_DIR, filename)

    if not os.path.isfile(data_path):
        continue

    # Read article IDs
    with open(data_path, "r") as f:
        article_ids = [line.strip() for line in f if line.strip()]

    doc_count = len(article_ids)

    # Read label (if exists)
    label = None
    if os.path.isfile(label_path):
        with open(label_path, "r") as f:
            label = f.read().strip()

    # Build key from filename: L0014-C-000683 → L14C683N7
    parts = filename.split("-")
    key = (
        "L" + str(int(parts[0][1:])) +
        "C" + str(int(parts[2])) +
        "N" + str(doc_count)
    )

    result[key] = {
        "doc_count": doc_count,
        "article_ids": article_ids,
        "label": label
    }

# Write JSON
with open(OUTPUT_FILE, "w") as f:
    json.dump(result, f, indent=2)

print(f"Written {OUTPUT_FILE}")
