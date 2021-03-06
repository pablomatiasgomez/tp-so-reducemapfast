#include <stdlib.h>
#include <string.h>
#include <commons/string.h>

#include "dir.h"

bson_t* dir_getBSON(dir_t *dir) {
	bson_t *bson = bson_new();
	BSON_APPEND_UTF8(bson, "_id", dir->id);
	BSON_APPEND_UTF8(bson, "name", dir->name);
	BSON_APPEND_UTF8(bson, "parentId", dir->parentId);
	return bson;
}

dir_t* dir_getDirFromBSON(const bson_t *doc) {
	bson_iter_t iter;
	const bson_value_t *value;
	const char *key;
	dir_t *dir = dir_create();

	if (bson_iter_init(&iter, doc)) {
		while (bson_iter_next(&iter)) {
			key = bson_iter_key(&iter);
			value = bson_iter_value(&iter);

			if (strcmp(key, "_id") == 0) {
				strcpy(dir->id, value->value.v_utf8.str);
			} else if (strcmp(key, "name") == 0) {
				dir->name = strdup(value->value.v_utf8.str);
			} else if (strcmp(key, "parentId") == 0) {
				strcpy(dir->parentId, value->value.v_utf8.str);
			}
			/*
			 if (bson_iter_find(&iter, "_id")) strcpy(dir->id, bson_iter_utf8(&iter, NULL));
			 if (bson_iter_find(&iter, "name")) strcpy(dir->name, bson_iter_utf8(&iter, NULL));
			 if (bson_iter_find(&iter, "parentId")) strcpy(dir->parentId, bson_iter_utf8(&iter, NULL));
			 */
		}
	}

	return dir;
}

dir_t* dir_create() {
	dir_t* dir = malloc(sizeof(dir_t));
	dir->name = NULL;
	return dir;
}

void dir_free(dir_t* dir) {
	if (dir->name) {
		free(dir->name);
	}
	free(dir);
}

void freeSplits(char **splits) {
	char **auxSplit = splits;

	while (*auxSplit != NULL) {
		free(*auxSplit);
		auxSplit++;
	}

	free(splits);
}

char* getFileName(char *path) {
	char *fileName;
	char **dirNames = string_split(path, "/");
	int i = 0;
	while (dirNames[i]) {
		fileName = dirNames[i];
		i++;
	}
	fileName = strdup(fileName);
	freeSplits(dirNames);

	return fileName;
}