/*
 * node.h
 *
 *  Created on: 23/6/2015
 *      Author: utnso
 */

#ifndef SRC_STRUCTS_NODE_H_
#define SRC_STRUCTS_NODE_H_

#include <commons/collections/list.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <commons/string.h>
#include <stdlib.h>
#include <commons/collections/list.h>
#include <commons/string.h>
#include <arpa/inet.h>

typedef struct {
	uint16_t mapID;
	char *ip_nodo;
	uint16_t port_nodo;
	uint16_t numBlock;
	char *tempResultName;
} t_map;

typedef struct{
	uint16_t reduceID;
	char *ip_nodo;
	uint16_t port_nodo;
	char *tempResultName;
	uint32_t sizetmps;
	void *buffer_tmps;
} t_reduce;


/**************** METODOS ***************/
/* Nodo */
void serializeMap(int sock, t_map* map);
char* getMapReduceRoutine(char* pathFile);
void serializeReduce(int sock_nodo, t_reduce* reduce);


#endif /* SRC_STRUCTS_NODE_H_ */
