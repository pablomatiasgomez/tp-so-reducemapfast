#ifndef MONGO_H
#define MONGO_H

#include <commons/collections/list.h>
#include <bcon.h>
#include <bson.h>
#include <mongoc.h>
#include <stdio.h>
#include <stdlib.h>

// TODO replace.
#define ID_SIZE sizeof(char) * 25

mongoc_client_t* mongo_getClient();
void mongo_generateId();
void mongo_shutdown();

void mongo_createIndexIfAbsent(mongoc_collection_t *collection, char *name, const bson_t *keys, bool unique);

int mongo_saveDoc(bson_t *doc, mongoc_collection_t *collection);

t_list* mongo_getByQuery(bson_t *query, void* (*parser)(const bson_t*) , mongoc_collection_t *collection);
const bson_t* mongo_getDocById(char id[], mongoc_collection_t *collection);
const bson_t* mongo_getDocByQuery(bson_t *query, mongoc_collection_t *collection);

bool mongo_deleteDocByQuery(bson_t *query, mongoc_collection_t *collection);

void mongo_update(bson_t *query, bson_t *update, mongoc_collection_t *collection);

//bson_t** mongo_getAll();

#endif
