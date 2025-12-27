# Genealogy of Knowledge - Document Clustering Visualization

Interactive visualization of hierarchical document clustering using D3.js.

## Visualizations

### 1. Circular Tree (Default)
- **File:** `index.html`
- Interactive radial dendrogram with full sidebar
- Click nodes to view associated articles
- Customizable: radius, node size, line width, labels
- Features: label deduplication, color coding by category

### 2. Tree of Life View
- **File:** `tree-of-life.html`
- Phylogenetic-style radial tree with step links
- Color-coded by top-level branches
- Hover labels to highlight ancestry path
- Clean, scientific aesthetic

## Datasets

Three datasets are included in `data/`:

| Dataset | Description | Size |
|---------|-------------|------|
| TU Wien ScholarWorks | Academic publications from TU Wien | ~30k docs |
| AUB ScholarWorks | Academic publications from AUB | ~15k docs |
| 20 Newsgroups | Classic text classification dataset | ~20k docs |
| French Theses (Sample) | Sampled branch from French thesis corpus | ~3k docs |

## Deployment

### Render Static Site
1. Point Render to the `website/` folder
2. Set publish directory to `website`
3. No build command needed

### Local Development
```bash
cd website
python3 -m http.server 8080
# Visit http://localhost:8080
```

## File Structure

```
website/
├── index.html          # Main circular tree visualization
├── tree-of-life.html   # Alternative phylogenetic view
├── data/
│   ├── *-tree.json     # Hierarchical tree structure
│   ├── *-mapping.json  # Cluster metadata (labels, doc counts)
│   └── *-articles.json # Article content for sidebar
└── README.md
```

## Data Format

### tree.json
Nested JSON with `name` and `children` properties:
```json
{
  "name": "root",
  "children": [
    {"name": "cluster1", "children": [...]},
    {"name": "leaf_node"}
  ]
}
```

### mapping.json
Maps node names to metadata:
```json
{
  "node_name": {
    "label": "Human readable label",
    "doc_count": 150,
    "article_ids": ["id1", "id2", ...],
    "annotations": {"category1": 100, "category2": 50}
  }
}
```

### articles.json
Article content keyed by ID:
```json
{
  "article_id": {
    "title": "Article Title",
    "content": "Article text...",
    "authors": "Author1||Author2",
    "date": "2024-01-01",
    "annotation": "Category"
  }
}
```

## Controls

### Circular Tree
- **Ctrl+Scroll:** Zoom in/out
- **Drag:** Pan around
- **Click node:** View articles in sidebar
- **Sliders:** Adjust radius, node size, line width, label settings

### Tree of Life
- **Scroll:** Zoom
- **Drag:** Pan
- **Hover label:** Highlight ancestry path
