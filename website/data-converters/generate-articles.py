import json
import sys

INPUT_FILE = sys.argv[1]
OUTPUT_FILE = sys.argv[2]

result = {}

with open(INPUT_FILE, "r", encoding="utf-8") as f:
    for idx, line in enumerate(f):
        line = line.strip()
        if not line:
            continue

        article = json.loads(line)

        result[str(idx)] = {
            "title": article.get("title", ""),
            "content": article.get("resumes", "")
        }

# Write output JSON
with open(OUTPUT_FILE, "w", encoding="utf-8") as f:
    json.dump(result, f, indent=2, ensure_ascii=False)

print(f"Converted {len(result)} articles → {OUTPUT_FILE}")
