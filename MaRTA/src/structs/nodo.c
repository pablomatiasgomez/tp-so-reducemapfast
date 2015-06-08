#include <stdbool.h>
#include <string.h>
#include <commons/string.h>
#include "nodo.h"
#include <stdlib.h>

bool nodeByName(t_node *nodo, char nombre[25]) {
	return string_equals_ignore_case(nodo->name, nombre);
}

int workLoad(t_list *maps, t_list *reduces) {
	return list_size(maps) + list_size(reduces);
}

void freeNode(t_node *nodo) {
	list_destroy(nodo->maps);
	list_destroy(nodo->reduces);
	free(nodo);
}
