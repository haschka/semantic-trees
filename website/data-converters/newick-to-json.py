from Bio import Phylo
import json
import sys

def to_dict(clade):
    return {
        "name": clade.name,
        "children": [to_dict(c) for c in clade.clades] or None
    }

tree = Phylo.read(sys.argv[1], "newick")
json_string = json.dumps(to_dict(tree.root),indent=2)
json_string = json_string + '\n'
sys.stdout.write(json_string)
