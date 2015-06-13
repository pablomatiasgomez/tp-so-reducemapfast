#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <commons/collections/list.h>
#include <commons/string.h>
#include "../../../utils/socket.h"
#include "serialize.h"
#include <arpa/inet.h>

void stringsToPathFile(t_list *list, char *string) {
	char **splits = string_split(string, " ");
	char **auxSplit = splits;

	while (*auxSplit != NULL) {
		t_file *file = CreateFile(*auxSplit);
		list_add(list, file);
		free(*auxSplit);
		auxSplit++;
	}

	free(splits);
}

t_job *desserealizeJob(int fd, uint32_t id) {
	size_t sbuffer;
	void *buffer;
	socket_recv_packet(fd, &buffer, &sbuffer);

	size_t scombiner = sizeof(bool);
	bool combiner;
	size_t sfiles = sbuffer - scombiner;
	char *stringFiles = malloc(sfiles);

	memcpy(&combiner, buffer, scombiner);
	memcpy(stringFiles, (buffer + scombiner), sfiles);

	t_job *job = CreateJob(id, combiner);
	stringsToPathFile(job->files, stringFiles);

	free(stringFiles);
	free(buffer);
	return job;
}

void serializeMapToOrder(int fd, t_map *map) {
	char order = 'm';
	size_t sIpMap = sizeof(uint32_t);
	size_t snumBlock = sIpMap;
	size_t snodePort = sizeof(uint16_t);
	size_t sOrder = sizeof(char);
	size_t snodeIP = strlen(map->nodeIP) + 1;
	size_t stempName = sizeof(char) * 60;
	size_t sbuffer = sOrder + sIpMap + sizeof(snodeIP) + snodeIP + snodePort + snumBlock + stempName;

	uint32_t id = htonl(map->id);
	uint32_t numBlock = htonl(map->numBlock);
	uint16_t nodePort = htons(map->nodePort);

	void *buffer = malloc(sbuffer);
	memcpy(buffer, &order, sOrder);
	memcpy(buffer + sOrder, &id, sIpMap);
	memcpy(buffer + sOrder + sIpMap, &snodeIP, sizeof(snodeIP));
	memcpy(buffer + sOrder + sIpMap + sizeof(snodeIP), map->nodeIP, snodeIP);
	memcpy(buffer + sOrder + sIpMap + sizeof(snodeIP) + snodeIP, &nodePort, snodePort);
	memcpy(buffer + sOrder + sIpMap + sizeof(snodeIP) + snodeIP + snodePort, &numBlock, snumBlock);
	memcpy(buffer + sOrder + sIpMap + sizeof(snodeIP) + snodeIP + snodePort + snumBlock, map->tempResultName, stempName);
	socket_send_packet(fd, buffer, sbuffer);
	free(buffer);
}

size_t totalTempsSize(t_list *temps) {
	size_t stemps = 0;
	void upgradeSize(t_temp * temp) {
		size_t snodeIP = strlen(temp->nodeIP) + 1;
		stemps += sizeof(uint32_t) + sizeof(snodeIP) + snodeIP + sizeof(uint16_t) + sizeof(char) * 60;
	}
	list_iterate(temps, (void *) upgradeSize);
	return stemps;
}

void serializeTemp(t_temp *temporal, void *buffer, size_t *sbuffer) {
	size_t snodeIP = strlen(temporal->nodeIP) + 1;
	uint32_t originMap = htonl(temporal->originMap);
	uint16_t nodePort = htons(temporal->nodePort);

	memcpy(buffer + *sbuffer, &originMap, sizeof(uint32_t));
	memcpy(buffer + *sbuffer + sizeof(uint32_t), &snodeIP, sizeof(snodeIP));
	memcpy(buffer + *sbuffer + sizeof(uint32_t) + sizeof(snodeIP), temporal->nodeIP, snodeIP);
	memcpy(buffer + *sbuffer + sizeof(uint32_t) + sizeof(snodeIP) + snodeIP, &nodePort, sizeof(uint16_t));
	memcpy(buffer + *sbuffer + sizeof(uint32_t) + sizeof(snodeIP) + snodeIP + sizeof(uint16_t), temporal->tempName, sizeof(char) * 60);

	*sbuffer += sizeof(uint32_t) + sizeof(snodeIP) + snodeIP + sizeof(uint16_t) + sizeof(char) * 60;
}

void serializeReduceToOrder(int fd, t_reduce *reduce) {
	char order = 'r';
	size_t sOrder = sizeof(char);
	size_t snodeIP = strlen(reduce->nodeIP) + 1;
	size_t snodePort = sizeof(uint16_t);
	size_t stempName = sizeof(char) * 60;

	uint16_t nodePort = htons(reduce->nodePort);

	uint16_t countTemps = 0;
	size_t stemps = totalTempsSize(reduce->temps);
	size_t auxSize = 0;
	void *tempsBuffer = malloc(stemps);
	void serializeTempsToBuffer(t_temp *temp) {
		serializeTemp(temp, tempsBuffer, &auxSize);
		countTemps++;
	}
	list_iterate(reduce->temps, (void *) serializeTempsToBuffer);
	countTemps = htons(countTemps);

	size_t sbuffer = sOrder + sizeof(snodeIP) + snodeIP + snodePort + stempName + sizeof(uint16_t) + stemps;
	void *buffer = malloc(sbuffer);
	memcpy(buffer, &order, sOrder);
	memcpy(buffer + sOrder, &snodeIP, sizeof(snodeIP));
	memcpy(buffer + sOrder + sizeof(snodeIP), reduce->nodeIP, snodeIP);
	memcpy(buffer + sOrder + sizeof(snodeIP) + snodeIP, &nodePort, snodePort);
	memcpy(buffer + sOrder + sizeof(snodeIP) + snodeIP + snodePort, reduce->tempResultName, stempName);
	memcpy(buffer + sOrder + sizeof(snodeIP) + snodeIP + snodePort + stempName, &countTemps, sizeof(uint16_t));
	memcpy(buffer + sOrder + sizeof(snodeIP) + snodeIP + snodePort + stempName + sizeof(uint16_t), tempsBuffer, stemps);

	socket_send_packet(fd, buffer, sbuffer);
	free(tempsBuffer);
	free(buffer);
}
