#include <stdlib.h>
#include <string.h>

#include "file.h"

bson_t* file_getBSON(file_t *file) {
	bson_t *bson = bson_new();
	BSON_APPEND_UTF8(bson, "_id", file->id);
	BSON_APPEND_UTF8(bson, "name", file->name);
	BSON_APPEND_INT32(bson, "size", file->size);
	BSON_APPEND_UTF8(bson, "parentId", file->parentId);
	return bson;
}

file_t* file_getFileFromBSON(const bson_t *doc) {
	bson_iter_t iter;
	const bson_value_t *value;
	const char *key;
	file_t *file = file_create();

	if (bson_iter_init(&iter, doc)) {
		while (bson_iter_next(&iter)) {
			key = bson_iter_key(&iter);
			value = bson_iter_value(&iter);

			if (strcmp(key, "_id") == 0) {
				strcpy(file->id, value->value.v_utf8.str);
			} else if (strcmp(key, "name") == 0) {
				strcpy(file->name, value->value.v_utf8.str);
			} else if (strcmp(key, "size") == 0) {
				file->size = value->value.v_int32;
			} else if (strcmp(key, "parentId") == 0) {
				strcpy(file->parentId, value->value.v_utf8.str);
			}
		}
	}

	return file;
}

file_t* file_create() {
	file_t* file = malloc(sizeof(file_t));
	file->name = malloc(sizeof(char) * 512);
	file->parentId = malloc(sizeof(char) * 25);
	return file;
}

void file_free(file_t *file) {
	free(file->name);
	free(file->parentId);
	free(file);
}
