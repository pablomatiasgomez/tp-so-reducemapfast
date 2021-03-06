#include "job.h"
#include "node.h"

t_copy *CreateCopy(char *nodeName, uint16_t numBlock) {
	t_copy *copy = malloc(sizeof(t_copy));
	copy->nodeName = strdup(nodeName);
	copy->numBlock = numBlock;
	return copy;
}

char* getTime() {
	time_t rawtime;
	struct tm * timeinfo;
	time(&rawtime);
	timeinfo = localtime(&rawtime);

	char * time = asctime(timeinfo);
	int i;
	for (i = 0; i < 24; i++)
		if (time[i] == ' ' || time[i] == ':')
			time[i] = '-';
	time[24] = '\0';

	return time;
}

void setTempMapName(char tempMapName[60], uint16_t mapID, uint16_t jobID) {
	char resultName[60];
	memset(resultName, '\0', sizeof(char) * 60);
	strcat(resultName, getTime());
	strcat(resultName, "-Job");
	char idJob[4];
	sprintf(idJob, "%i", jobID);
	strcat(resultName, idJob);
	strcat(resultName, "-Map");
	char numMap[4];
	sprintf(numMap, "%i", mapID);
	strcat(resultName, numMap);
	strcat(resultName, ".txt");
	strcpy(tempMapName, resultName);
}

t_map *CreateMap(uint16_t id, uint16_t numBlock, uint16_t nodePort,
		char *nodeName, char *nodeIP, uint16_t jobID) {
	t_map *map = malloc(sizeof(t_map));
	map->id = id;
	map->numBlock = numBlock;
	map->nodePort = nodePort;
	map->nodeName = strdup(nodeName);
	map->nodeIP = strdup(nodeIP);
	memset(map->tempResultName, '\0', sizeof(char) * 60);
	setTempMapName(map->tempResultName, map->id, jobID);
	map->done = false;
	return map;
}

t_temp *CreateTemp(char *nodeID, char *nodeIP, uint16_t nodePort,
		uint16_t originMap, char tempName[60]) {
	t_temp *temp = malloc(sizeof(t_temp));
	temp->nodeID = nodeID;
	temp->nodeIP = strdup(nodeIP);
	temp->nodePort = nodePort;
	temp->originMap = originMap;
	memset(temp->tempName, '\0', sizeof(char) * 60);
	strcpy(temp->tempName, tempName);
	return temp;
}

t_file *CreateFile(char *path) {
	t_file *file = malloc(sizeof(t_file));
	file->path = strdup(path);
	file->blocks = list_create();
	return file;
}

void setTempReduceName(char tempResultName[60], uint16_t reduceID,
		uint16_t jobID) {
	char resultName[60];
	memset(resultName, '\0', sizeof(char) * 60);
	strcat(resultName, getTime());
	strcat(resultName, "-Job");
	char idJob[4];
	sprintf(idJob, "%i", jobID);
	strcat(resultName, idJob);
	strcat(resultName, "-Red");
	char numReduce[4];
	sprintf(numReduce, "%i", reduceID);
	strcat(resultName, numReduce);
	strcat(resultName, ".txt");
	strcpy(tempResultName, resultName);
}

t_reduce *CreateReduce(uint16_t id, char *nodeName, char *nodeIP,
		uint16_t nodePort, uint16_t jobID) {
	t_reduce *reduce = malloc(sizeof(t_reduce));
	reduce->id = id;
	reduce->finalNode = strdup(nodeName);
	reduce->nodeIP = strdup(nodeIP);
	reduce->nodePort = nodePort;
	memset(reduce->tempResultName, '\0', sizeof(char) * 60);
	setTempReduceName(reduce->tempResultName, reduce->id, jobID);
	reduce->temps = list_create();
	return reduce;
}

t_reduce *initReduce() {
	t_reduce *reduce = malloc(sizeof(t_reduce));
	reduce->finalNode = NULL;
	reduce->nodeIP = NULL;
	reduce->temps = list_create();
	memset(reduce->tempResultName, '\0', sizeof(char) * 60);
	return reduce;
}

void setFinalReduce(t_reduce *reduce, char *nodeName, char *nodeIP,
		uint16_t nodePort, uint16_t jobID) {
	reduce->id = 0;
	reduce->finalNode = strdup(nodeName);
	reduce->nodeIP = strdup(nodeIP);
	reduce->nodePort = nodePort;
	setTempReduceName(reduce->tempResultName, reduce->id, jobID);
}

t_job *CreateJob(uint16_t id, bool combiner, char *resultadoFinal) {
	t_job *job = malloc(sizeof(t_job));
	job->id = id;
	job->combiner = combiner;
	job->resultFile = strdup(resultadoFinal);
	job->files = list_create();
	job->partialReduces = list_create();
	job->finalReduce = initReduce();
	job->maps = list_create();
	job->mapsDone = 0;
	return job;
}

void freeCopy(t_copy *copy) {
	if (copy->nodeName) {
		free(copy->nodeName);
	}
	free(copy);
}

void freeCopies(t_list *copies) {
	list_destroy_and_destroy_elements(copies, (void *) freeCopy);
}

void freeFile(t_file *file) {
	if (file->path) {
		free(file->path);
	}

	list_destroy_and_destroy_elements(file->blocks, (void *) freeCopies);
	free(file);
}

void freeTemp(t_temp *temp) {
	if (temp->nodeIP) {
		free(temp->nodeIP);
	}
	free(temp);
}

void freeReduce(t_reduce *reduce) {
	if (reduce->finalNode != NULL) {
		free(reduce->finalNode);
	}
	if (reduce->nodeIP != NULL) {
		free(reduce->nodeIP);
	}
	list_destroy_and_destroy_elements(reduce->temps, (void *) freeTemp);
	free(reduce);
}

void freeMap(t_map *map) {
	if (map->nodeName) {
		free(map->nodeName);
	}
	if (map->nodeIP) {
		free(map->nodeIP);
	}
	free(map);
}

void freeJob(t_job *job) {
	list_iterate(job->maps, (void *) removeMapNode);
	list_destroy_and_destroy_elements(job->files, (void *) freeFile);
	list_destroy_and_destroy_elements(job->maps, (void *) freeMap);
	list_destroy_and_destroy_elements(job->partialReduces, (void *) freeReduce);
	freeReduce(job->finalReduce);
	free(job->resultFile);
	free(job);
}

bool isMap(t_map *map, uint16_t idMap) {
	return map->id == idMap;
}

bool mapIsDone(t_map *map) {
	return map->done;
}

bool isReduce(t_reduce *reduce, uint16_t idReduce) {
	return reduce->id == idReduce;
}
