#include "JobConnection.h"
#include <commons/collections/list.h>
#include <commons/string.h>
#include "../Planning/MapPlanning.h"
#include <arpa/inet.h>
#include "../MaRTA.h"

//**********************************************************************************//
//									JOB												//
//**********************************************************************************//
void *acceptJob(void * param) {
	cantJobs++; //TODO mutex this
	int *socketAcceptedPtr = (int *) param;
	int jobSocket = *socketAcceptedPtr;
	free(socketAcceptedPtr);

	t_job *job = desserializeJob(jobSocket, cantJobs);

	if (job->combiner)
		log_info(logger, "Iniciando Job: %d (Combiner)", job->id);
	else
		log_info(logger, "Iniciando Job: %d (No combiner)", job->id);

	t_map *map = CreateMap(1, 13, 5001, "NodoX", "127.0.0.1", "temporalMap1.txt");
	list_add(job->maps, map);

	e_socket_status socketStatus;
	serializeMapToOrder(jobSocket, map);
	socketStatus = recvResult(jobSocket, job);

	if (0 > socketStatus)
		log_error(logger, "Murio Job: %d", job->id);

	freeJob(job);
	return NULL;
}

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

t_job *desserializeJob(int socket, uint16_t id) {
	size_t sbuffer;
	void *buffer;
	e_socket_status status = socket_recv_packet(socket, &buffer, &sbuffer);
	if (0 > status) {
		log_error(logger, "Error at socket_recv in dessearializeJob");
		pthread_exit(NULL);
	}

	size_t scombiner = sizeof(bool);
	bool combiner;
	size_t sfiles = sbuffer - scombiner;
	char *stringFiles = malloc(sfiles + 1);

	memcpy(&combiner, buffer, scombiner);
	memcpy(stringFiles, (buffer + scombiner), sfiles);
	stringFiles[sfiles] = '\0';

	t_job *job = CreateJob(id, combiner);
	stringsToPathFile(job->files, stringFiles);

	free(stringFiles);
	free(buffer);
	return job;
}

e_socket_status recvResult(int socket, t_job *job) {
	void *buffer;
	size_t sbuffer = 0;
	e_socket_status status = socket_recv_packet(socket, &buffer, &sbuffer);
	if (0 > status)
		return status;
	uint8_t resultFrom;
	memcpy(&resultFrom, buffer, sizeof(uint8_t));
	int i;
	for (i = 0; i < sbuffer; i++)
		printf("%d \n", *(uint8_t *) (buffer + i));
	printf("\n\n %d \n\n", resultFrom);
	fflush(stdout);
	switch(resultFrom){
	case COMMAND_MAP:
		desserializeMapResult(buffer + sizeof(uint8_t), job);
		break;
	case COMMAND_REDUCE:
		desserializaReduceResult(buffer + sizeof(uint8_t), job);
		break;
	}
	free(buffer);
	return status;
}

void sendDieOrder(int socket) {
	char order = COMMAND_MARTA_TO_JOB_DIE;
	size_t sOrder = sizeof(char);
	size_t sbuffer = sOrder;
	void *buffer = malloc(sbuffer);
	buffer = memset(buffer, '\0', sbuffer);
	memcpy(buffer, &order, sOrder);
	socket_send_packet(socket, buffer, sbuffer);
	free(buffer);
}
//**********************************MAP*********************************************//
void serializeMapToOrder(int socket, t_map *map) {
	uint8_t order = COMMAND_MAP;
	size_t sOrder = sizeof(uint8_t);
	size_t sIpMap = sizeof(uint16_t);
	size_t snumBlock = sIpMap;
	size_t snodePort = sizeof(uint16_t);
	size_t snodeIP = strlen(map->nodeIP) + 1;
	size_t stempName = sizeof(char) * 60;
	size_t sbuffer = sOrder + sIpMap + sizeof(snodeIP) + snodeIP + snodePort + snumBlock + stempName;

	uint16_t id = htons(map->id);
	uint16_t numBlock = htons(map->numBlock);
	uint16_t nodePort = htons(map->nodePort);

	void *buffer = malloc(sbuffer);
	buffer = memset(buffer, '\0', sbuffer);
	memcpy(buffer, &order, sOrder);
	memcpy(buffer + sOrder, &id, sIpMap);
	memcpy(buffer + sOrder + sIpMap, &snodeIP, sizeof(snodeIP));
	memcpy(buffer + sOrder + sIpMap + sizeof(snodeIP), map->nodeIP, snodeIP);
	memcpy(buffer + sOrder + sIpMap + sizeof(snodeIP) + snodeIP, &nodePort, snodePort);
	memcpy(buffer + sOrder + sIpMap + sizeof(snodeIP) + snodeIP + snodePort, &numBlock, snumBlock);
	memcpy(buffer + sOrder + sIpMap + sizeof(snodeIP) + snodeIP + snodePort + snumBlock, map->tempResultName, stempName);
	socket_send_packet(socket, buffer, sbuffer);
	free(buffer);
}

void desserializeMapResult(void *buffer, t_job *job) {
	size_t sresult = sizeof(bool);
	size_t sidMap = sizeof(uint16_t);

	bool result;
	uint16_t idMap;

	memcpy(&result, buffer, sresult);
	memcpy(&idMap, buffer + sresult, sidMap);
	idMap = ntohs(idMap);

	log_trace(logger, "\nMap: %d result: %d\n", idMap, result);
	bool findMap(t_map *map) {
		return isMap(map, idMap);
	}
	t_map *map = list_find(job->maps, (void *) findMap);

	if (result) {
		map->done = true;
	} else {
		rePlanMap(job, map);
	}
}
//*********************************REDUCE*******************************************//
size_t totalTempsSize(t_list *temps) {
	size_t stemps = 0;
	void upgradeSize(t_temp * temp) {
		size_t snodeIP = strlen(temp->nodeIP) + 1;
		stemps += sizeof(uint16_t) + sizeof(snodeIP) + snodeIP + sizeof(uint16_t) + sizeof(char) * 60;
	}
	list_iterate(temps, (void *) upgradeSize);
	return stemps;
}

void serializeTemp(t_temp *temporal, void *buffer, size_t *sbuffer) {
	size_t snodeIP = strlen(temporal->nodeIP) + 1;
	uint16_t originMap = htons(temporal->originMap);
	uint16_t nodePort = htons(temporal->nodePort);

	memcpy(buffer + *sbuffer, &originMap, sizeof(uint16_t));
	memcpy(buffer + *sbuffer + sizeof(uint16_t), &snodeIP, sizeof(snodeIP));
	memcpy(buffer + *sbuffer + sizeof(uint16_t) + sizeof(snodeIP), temporal->nodeIP, snodeIP);
	memcpy(buffer + *sbuffer + sizeof(uint16_t) + sizeof(snodeIP) + snodeIP, &nodePort, sizeof(uint16_t));
	memcpy(buffer + *sbuffer + sizeof(uint16_t) + sizeof(snodeIP) + snodeIP + sizeof(uint16_t), temporal->tempName, sizeof(char) * 60);

	*sbuffer += sizeof(uint16_t) + sizeof(snodeIP) + snodeIP + sizeof(uint16_t) + sizeof(char) * 60;
}

void serializeReduceToOrder(int socket, t_reduce *reduce) {
	char order = COMMAND_REDUCE;
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
	buffer = memset(buffer, '\0', sbuffer);
	memcpy(buffer, &order, sOrder);
	memcpy(buffer + sOrder, &snodeIP, sizeof(snodeIP));
	memcpy(buffer + sOrder + sizeof(snodeIP), reduce->nodeIP, snodeIP);
	memcpy(buffer + sOrder + sizeof(snodeIP) + snodeIP, &nodePort, snodePort);
	memcpy(buffer + sOrder + sizeof(snodeIP) + snodeIP + snodePort, reduce->tempResultName, stempName);
	memcpy(buffer + sOrder + sizeof(snodeIP) + snodeIP + snodePort + stempName, &countTemps, sizeof(uint16_t));
	memcpy(buffer + sOrder + sizeof(snodeIP) + snodeIP + snodePort + stempName + sizeof(uint16_t), tempsBuffer, stemps);

	socket_send_packet(socket, buffer, sbuffer);
	free(tempsBuffer);
	free(buffer);
}

void desserializaReduceResult(void *buffer, t_job *job) {
	size_t sresult = sizeof(bool);
	size_t sidReduce = sizeof(uint16_t);

	bool result;
	uint16_t idReduce;
	char failedTemp[60];
	memset(failedTemp, '\0', sizeof(char) * 60);

	memcpy(&result, buffer, sresult);
	memcpy(&idReduce, buffer + sresult, sidReduce);
	memcpy(&failedTemp, buffer + sresult + sidReduce, sizeof(char) * 60);
	idReduce = ntohs(idReduce);

	log_trace(logger, "\nReduce: %d result: %d\n", idReduce, result);

	if (result) {
		if (!idReduce)
			job->finalReduce->done = 1;
		else {
			bool findReduce(t_reduce *reduce) {
				return isReduce(reduce, idReduce);
			}
			t_reduce *reduce = list_find(job->partialReduces, (void *) findReduce);
			reduce->done = 1;
		}

	} else {
		//TODO RePlanReduce
		printf("%s\n", failedTemp);
	}
}