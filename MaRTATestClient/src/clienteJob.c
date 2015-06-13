#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <commons/collections/list.h>
#include <arpa/inet.h>
#include "../../utils/socket.h"

void serializeJobToMaRTA(int fd, bool combiner, t_list *files);
void desserializeMapOrder(void *buffer, size_t sbuffer);
void recvOrder(int fd);
int fd;

int main() {
	t_list *files = list_create();
	list_add(files, "sarasa");
	list_add(files, "sarasa");
	list_add(files, "sarasa");
	list_add(files, "sarasa");
	list_add(files, "sarasa");

	fd = socket_connect("127.0.0.1", 30000);
	int hand = socket_handshake_to_server(fd, HANDSHAKE_MARTA, HANDSHAKE_JOB);
	if (!hand) {
		printf("Error al conectar\n");
		return EXIT_FAILURE;
	}
	bool combiner = false;
	serializeJobToMaRTA(fd, combiner, files);
	list_destroy(files);
	recvOrder(fd);
	return EXIT_SUCCESS;
}

size_t lengthStringList(t_list *stringList) {
	size_t length = 0;
	void totalLength(char *string) {
		length += (strlen(string) + 1);
	}
	list_iterate(stringList, (void *) totalLength);
	return sizeof(char) * length;
}

void serializeJobToMaRTA(int fd, bool combiner, t_list *files) {
	size_t filesLength = lengthStringList(files);
	char *stringFiles = malloc(filesLength);
	strcpy(stringFiles, "");
	void listToString(char *file) {
		strcat(stringFiles, file);
		strcat(stringFiles, " ");
	}
	list_iterate(files, (void *) listToString);
	stringFiles[filesLength - 1] = '\0';

	size_t scombiner = sizeof(combiner);
	size_t sbuffer = scombiner + filesLength;
	void *buffer = malloc(sbuffer);
	combiner = htonl(combiner);
	memcpy(buffer, &combiner, scombiner);
	memcpy((buffer + scombiner), stringFiles, filesLength);

	socket_send_packet(fd, buffer, sbuffer);
	free(stringFiles);
	free(buffer);
}

void recvOrder(int fd) {
	void *buffer;
	size_t sbuffer = 0;
	socket_recv_packet(fd, &buffer, &sbuffer);
	uint16_t order = 0;
	size_t sOrder = sizeof(char);
	memcpy(&order, buffer, sOrder);
	printf("\nRECVORDER: %c\n", order);
	if (order == 'm')
		desserializeMapOrder(buffer + sOrder, sbuffer - sOrder);
}

void desserializeMapOrder(void *buffer, size_t sbuffer) {
	size_t sIdMap = sizeof(uint32_t);
	size_t snumblock = sIdMap;
	size_t snodePort = sizeof(uint16_t);
	size_t stempName = sizeof(char) * 60;
	size_t snodeIP;

	uint32_t idMap;
	char* nodeIP;
	uint16_t nodePort;
	uint32_t numBlock;
	char tempResultName[60];

	memcpy(&idMap, buffer, sIdMap);
	memcpy(&snodeIP, buffer + sIdMap, sizeof(size_t));
	nodeIP = malloc(snodeIP);
	memcpy(nodeIP, buffer + sIdMap + sizeof(size_t), snodeIP);
	memcpy(&nodePort, buffer + sIdMap + sizeof(size_t) + snodeIP, snodePort);
	memcpy(&numBlock, buffer + sIdMap + sizeof(size_t) + snodeIP + snodePort, snumblock);
	memcpy(tempResultName, buffer + sIdMap + sizeof(size_t) + snodeIP + snodePort + snumblock, stempName);

	idMap = ntohl(idMap);
	nodePort = ntohs(nodePort);
	numBlock = ntohl(numBlock);

	//Test TODO: Adaptar a las estructuras de Job
	printf("\n%d\n", idMap);
	printf("%s\n", nodeIP);
	printf("%d\n", nodePort);
	printf("%d\n", numBlock);
	printf("%s\n", tempResultName);
	fflush(stdout);
	//End
}