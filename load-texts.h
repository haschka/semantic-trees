char** texts_from_files(char** files, int n_files);

char** cut_texts_in_subtexts(char* text,
			     size_t text_length,
			     size_t requested_length,
			     size_t* n_texts);

char** cut_texts_in_subtexts_with_overlap(char* text,
					  size_t text_length,
					  size_t requested_length,
					  size_t* n_texts);

char** extract_texts_from_jsonl_file(char* key, char* file,
				     size_t* n);
