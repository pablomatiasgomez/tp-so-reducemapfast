#ifndef NODE_H
#define NODE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <commons/config.h>
#include <commons/log.h>
#include <commons/string.h>

#define	BLOCK_SIZE	20 * 1024 * 1024 // 20 MB de bloques

/* TODO, que verga es esto?
typedef struct {
	int tipo;
	int id_proceso;
	int datosNumericos;
	char mensaje[16];
} t_mensaje;
*/

typedef struct {
	char *fsIp;
	uint16_t fsPort;
	uint16_t listenPort;
	char *binFilePath;
	char *tmpDir;
	bool newNode;
	char *name;
	uint16_t blocksCount;
} t_nodeCfg;

extern t_log* node_logger;
extern t_nodeCfg* node_config;

char* node_getBlock(uint16_t numBlock);
void node_freeBlock(char *blockStr);
void node_setBlock(uint16_t numBlock, char *blockData);

#endif