#!/usr/bin/env python3

import json
import sys
from pathlib import Path

def die(msg):
    print(f"ERROR: {msg}", file=sys.stderr)
    sys.exit(1)

if len(sys.argv) != 3:
    die("Usage: python build_label_splitset.py JSON_FILE LABEL_KEY")

json_file = Path(sys.argv[1])
label_key = sys.argv[2]

if not json_file.exists():
    die(f"JSON file does not exist: {json_file}")

label_to_id = {}
labels_per_doc = []
next_id = 0

with json_file.open("r", encoding="utf-8") as f:
    for line_number, line in enumerate(f, start=1):
        line = line.strip()
        if not line:
            die(f"Empty line at {line_number}")

        try:
            obj = json.loads(line)
        except json.JSONDecodeError as e:
            die(f"JSON parse error at line {line_number}: {e}")

        if label_key not in obj:
            die(f"Missing label key '{label_key}' at line {line_number}")

        label = obj[label_key]

        # Normalize labels to strings for consistent hashing
        label_str = str(label)

        if label_str not in label_to_id:
            label_to_id[label_str] = next_id
            next_id += 1

        labels_per_doc.append(label_to_id[label_str])

if not labels_per_doc:
    die("No documents found in JSONL file")

# Output files
labels_file = Path("labels.txt")
label_list_file = Path("label_list.txt")

with labels_file.open("w", encoding="utf-8") as f:
    for lid in labels_per_doc:
        f.write(f"{lid}\n")

with label_list_file.open("w", encoding="utf-8") as f:
    for lid in sorted(set(labels_per_doc)):
        f.write(f"{lid}\n")

print("✔ Successfully generated split_set label files")
print(f"  Documents      : {len(labels_per_doc)}")
print(f"  Unique labels  : {len(label_to_id)}")
print(f"  labels.txt     : per-document numeric labels")
print(f"  label_list.txt : label universe")
