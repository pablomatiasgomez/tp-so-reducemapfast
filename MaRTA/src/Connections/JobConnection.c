#include "Connection.h"
#include "../Planning/MapPlanning.h"
#include "../Planning/ReducePlanning.h"
#include "../structs/node.h"
#include <commons/string.h>

//**********************************************************************************//
//									JOB												//
//**********************************************************************************//

void *acceptJob(void * param) {
	pthread_mutex_lock(&McantJobs);
	cantJobs++;
	pthread_mutex_unlock(&McantJobs);
	int *socketAcceptedPtr = (int *) param;
	int jobSocket = *socketAcceptedPtr;
	free(socketAcceptedPtr);

	t_job *job = desserializeJob(jobSocket, cantJobs);
	job->socket = jobSocket;

	if (job->combiner)
		log_info(logger, "Begin Job: %d (Combiner)", job->id);
	else
		log_info(logger, "Begin Job: %d (No combiner)", job->id);

	planMaps(job);

	void recvListResults(t_list *list) {
		int i;
		int count = list_size(list);
		for (i = 0; i < count; i++)
			recvResult(job);
	}

	recvListResults(job->maps);

	if (job->combiner) {
		combinerPartialsReducePlanning(job);
		recvListResults(job->partialReduces);
		combinerFinalReducePlanning(job);
	} else
		noCombinerReducePlanning(job);

	recvResult(job);

	log_info(logger, "Finished Job: %d", job->id);
	sendDieOrder(job->socket, COMMAND_RESULT_OK);
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
		log_error(logger, "Job %d Died when deserializing", id);
		free(buffer);
		pthread_exit(NULL);
	}

	size_t scombiner = sizeof(bool);
	bool combiner;
	uint16_t sresultadoFinal;

	memcpy(&combiner, buffer, scombiner);
	memcpy(&sresultadoFinal, buffer + scombiner, sizeof(uint16_t));
	sresultadoFinal = ntohs(sresultadoFinal);
	char *resultadoFinal = malloc(sresultadoFinal + 1);
	memcpy(resultadoFinal, buffer + scombiner + sizeof(uint16_t), sresultadoFinal);
	resultadoFinal[sresultadoFinal] = '\0';
	size_t sfiles = sbuffer - scombiner - sizeof(uint16_t) - sresultadoFinal;
	char *stringFiles = malloc(sfiles + 1);
	memcpy(stringFiles, buffer + scombiner + sizeof(uint16_t) + sresultadoFinal, sfiles);
	stringFiles[sfiles] = '\0';

	t_job *job = CreateJob(id, combiner, resultadoFinal);

	stringsToPathFile(job->files, stringFiles);

	free(stringFiles);
	free(buffer);
	return job;
}

void recvResult(t_job *job) {
	void *buffer;
	size_t sbuffer = 0;
	if (0 > socket_recv_packet(job->socket, &buffer, &sbuffer)) {
		log_error(logger, "Job %d Died when reciving results", job->id);
		freeJob(job);
		pthread_exit(NULL);
		return;
	}
	uint8_t resultFrom;
	memcpy(&resultFrom, buffer, sizeof(uint8_t));
	switch (resultFrom) {
	case COMMAND_MAP:
		desserializeMapResult(buffer + sizeof(uint8_t), job);
		break;
	case COMMAND_REDUCE:
		desserializaReduceResult(buffer + sizeof(uint8_t), job);
		break;
	}
	free(buffer);
}

e_socket_status sendDieOrder(int socket, uint8_t result) {
	char order = COMMAND_MARTA_TO_JOB_DIE;
	size_t sOrder = sizeof(char);
	size_t sbuffer = sOrder + sizeof(uint8_t);
	void *buffer = malloc(sbuffer);
	buffer = memset(buffer, '\0', sbuffer);
	memcpy(buffer, &order, sOrder);
	memcpy(buffer + sOrder, &result, sizeof(uint8_t));
	e_socket_status status = socket_send_packet(socket, buffer, sbuffer);
	free(buffer);
	return status;
}
//**********************************MAP*********************************************//
e_socket_status serializeMapToOrder(int socket, t_map *map) {
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
	memcpy(buffer, &order, sOrder);
	memcpy(buffer + sOrder, &id, sIpMap);
	memcpy(buffer + sOrder + sIpMap, &snodeIP, sizeof(snodeIP));
	memcpy(buffer + sOrder + sIpMap + sizeof(snodeIP), map->nodeIP, snodeIP);
	memcpy(buffer + sOrder + sIpMap + sizeof(snodeIP) + snodeIP, &nodePort, snodePort);
	memcpy(buffer + sOrder + sIpMap + sizeof(snodeIP) + snodeIP + snodePort, &numBlock, snumBlock);
	memcpy(buffer + sOrder + sIpMap + sizeof(snodeIP) + snodeIP + snodePort + snumBlock, map->tempResultName, stempName);
	e_socket_status status = socket_send_packet(socket, buffer, sbuffer);
	free(buffer);
	return status;
}

void desserializeMapResult(void *buffer, t_job *job) {
	size_t sresult = sizeof(bool);
	size_t sidMap = sizeof(uint16_t);

	bool result;
	uint16_t idMap;

	memcpy(&result, buffer, sresult);
	memcpy(&idMap, buffer + sresult, sidMap);
	idMap = ntohs(idMap);

	log_trace(logger, "Map: %d Done -> Result: %d", idMap, result);
	bool findMap(t_map *map) {
		return isMap(map, idMap);
	}
	t_map *map = list_find(job->maps, (void *) findMap);

	if (result) {
		map->done = true;
		removeMapNode(map);
	} else {
		t_node *node = findNode(nodes, map->nodeName);
		node->active = 0;
		rePlanMap(job, map);
	}
}

//*********************************REDUCE*******************************************//
size_t totalTempsSize(t_list *temps) {
	size_t stemps = 0;
	void upgradeSize(t_temp * temp) {
		size_t snodeID = strlen(temp->nodeID);
		size_t snodeIP = strlen(temp->nodeIP) + 1;
		stemps += sizeof(snodeID) + snodeID + sizeof(snodeIP) + snodeIP + sizeof(uint16_t) + sizeof(char) * 60;
	}
	list_iterate(temps, (void *) upgradeSize);
	return stemps;
}

void serializeTemp(t_temp *temporal, void *buffer, size_t *sbuffer) {
	uint16_t snodeID = strlen(temporal->nodeID);
	uint16_t snodeIP = strlen(temporal->nodeIP);
	uint16_t nodePort = htons(temporal->nodePort);

	uint16_t serializedsnodeID = htons(snodeID);
	uint16_t serializedsnodeIP = htons(snodeIP);

	memcpy(buffer + *sbuffer, &serializedsnodeID, sizeof(snodeID));
	memcpy(buffer + *sbuffer + sizeof(snodeID), temporal->nodeID, snodeID);
	memcpy(buffer + *sbuffer + sizeof(snodeID) + snodeID, &serializedsnodeIP, sizeof(snodeIP));
	memcpy(buffer + *sbuffer + sizeof(snodeID) + snodeID + sizeof(snodeIP), temporal->nodeIP, snodeIP);
	memcpy(buffer + *sbuffer + sizeof(snodeID) + snodeID + sizeof(snodeIP) + snodeIP, &nodePort, sizeof(uint16_t));
	memcpy(buffer + *sbuffer + sizeof(snodeID) + snodeID + sizeof(snodeIP) + snodeIP + sizeof(uint16_t), temporal->tempName, sizeof(char) * 60);

	*sbuffer += sizeof(snodeID) + snodeID + sizeof(snodeIP) + snodeIP + sizeof(uint16_t) + sizeof(char) * 60;
}

e_socket_status serializeReduceToOrder(int socket, t_reduce *reduce) {
	char order = COMMAND_REDUCE;
	size_t sOrder = sizeof(char);
	size_t snodeIP = strlen(reduce->nodeIP) + 1;
	size_t snodePort = sizeof(uint16_t);
	size_t stempName = sizeof(char) * 60;

	uint16_t reduceID = htons(reduce->id);
	uint16_t nodePort = htons(reduce->nodePort);

	uint16_t countTemps = 0;
	size_t stemps = totalTempsSize(reduce->temps);
	size_t auxSize = 0;
	void *tempsBuffer = malloc(stemps);
	tempsBuffer = memset(tempsBuffer, '\0', stemps);
	void serializeTempsToBuffer(t_temp *temp) {
		serializeTemp(temp, tempsBuffer, &auxSize);
		countTemps++;
	}
	list_iterate(reduce->temps, (void *) serializeTempsToBuffer);

	countTemps = htons(countTemps);

	size_t sbuffer = sOrder + sizeof(reduceID) + sizeof(snodeIP) + snodeIP + snodePort + stempName + sizeof(uint16_t) + stemps;
	void *buffer = malloc(sbuffer);
	buffer = memset(buffer, '\0', sbuffer);
	memcpy(buffer, &order, sOrder);
	memcpy(buffer + sOrder, &reduceID, sizeof(reduceID));
	memcpy(buffer + sOrder + sizeof(reduceID), &snodeIP, sizeof(snodeIP));
	memcpy(buffer + sOrder + sizeof(reduceID) + sizeof(snodeIP), reduce->nodeIP, snodeIP);
	memcpy(buffer + sOrder + sizeof(reduceID) + sizeof(snodeIP) + snodeIP, &nodePort, snodePort);
	memcpy(buffer + sOrder + sizeof(reduceID) + sizeof(snodeIP) + snodeIP + snodePort, reduce->tempResultName, stempName);
	memcpy(buffer + sOrder + sizeof(reduceID) + sizeof(snodeIP) + snodeIP + snodePort + stempName, &countTemps, sizeof(uint16_t));
	memcpy(buffer + sOrder + sizeof(reduceID) + sizeof(snodeIP) + snodeIP + snodePort + stempName + sizeof(uint16_t), tempsBuffer, stemps);

	e_socket_status status = socket_send_packet(socket, buffer, sbuffer);
	free(tempsBuffer);
	free(buffer);
	return status;
}

void desserializaReduceResult(void *buffer, t_job *job) {
	size_t sresult = sizeof(bool);
	size_t sidReduce = sizeof(uint16_t);

	bool result;
	uint16_t idReduce;
	memcpy(&result, buffer, sresult);
	memcpy(&idReduce, buffer + sresult, sidReduce);
	idReduce = ntohs(idReduce);

	t_reduce *reduce;
	if (!idReduce)
		reduce = job->finalReduce;
	else {
		bool findReduce(t_reduce *reduce) {
			return isReduce(reduce, idReduce);
		}
		reduce = list_find(job->partialReduces, (void *) findReduce);
	}

	if (result) {
		reduce->done = 1;
		removeReduceNode(reduce);

	} else {
		t_node *node;
		node = findNode(nodes, reduce->finalNode);
		node->active = 0;
		void deactiveNode(t_temp *temp) {
			node = findNode(nodes, temp->nodeID);
			node->active = 0;
		}

		list_iterate(reduce->temps, (void *) deactiveNode);
		log_info(logger, "Job %d Failed: reduce failed");
		sendDieOrder(job->socket, COMMAND_RESULT_REDUCEFAILED);
		freeJob(job);
		pthread_exit(0);
	}

	log_trace(logger, "Reduce: %d Done -> Result: %d", idReduce, result);
}
