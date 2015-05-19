#ifndef MONGO_FILE_H
#define MONGO_FILE_H

#include "../structs/file.h"

void mongo_file_checkInit();
int mongo_file_init();
void mongo_file_shutdown();
int mongo_file_save(file_t *file);
file_t* mongo_file_getById(char id[25]);
bool mongo_file_deleteFileByNameInDir(char *name, char parentId[25]);

#endif