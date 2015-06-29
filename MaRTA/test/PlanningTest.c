#include <string.h>
#include <stdlib.h>
#include <commons/collections/list.h>
#include "../src/MaRTA.h"
#include "../src/structs/node.h"
#include "../src/structs/job.h"
#include "../src/Planning/MapPlanning.h"
#include "../src/Planning/ReducePlanning.h"

t_node* node1;
t_node* node2;
t_node* node3;
t_node* node4;
t_node* node5;

t_copy *copy1;
t_copy *copy2;
t_copy *copy3;
t_list *copies1;

t_copy *copy2_1;
t_copy *copy2_2;
t_copy *copy2_3;
t_list *copies2;

t_copy *copy3_1;
t_copy *copy3_2;
t_copy *copy3_3;
t_copy *copy3_4;
t_copy *copy3_5;
t_list *copies3;

t_copy *copy4_1;
t_copy *copy4_2;
t_copy *copy4_3;
t_list *copies4;

t_copy *unavailableCopy;
t_list *unavailableCopies;

t_file *file;
t_file *file2;
t_file *file3;
t_file *notAvailableFile;

t_job *job;
t_job *job2;

void planningTestSetup() {

	//Creacion Nodos
	node1 = CreateNode(1, "IP Nodo1", 3001, "Node1");
	list_add(node1->maps, (void *) 1);
	list_add(node1->maps, (void *) 1);
	list_add(node1->maps, (void *) 1);
	list_add(node1->maps, (void *) 1);
	list_add(node1->reduces, (void *) "datos.txt");

	node2 = CreateNode(1, "IP Nodo2", 3002, "Node2");
	list_add(node2->maps, (void *) 1);
	list_add(node2->reduces, (void *) "datos.txt");

	node3 = CreateNode(0, "IP Nodo3", 3003, "Node3");
	node4 = CreateNode(1, "IP Nodo4", 3004, "Node4");
	node5 = CreateNode(1, "IP Nodo5", 3005, "Node5");

	list_add(nodes, node1);
	list_add(nodes, node2);
	list_add(nodes, node3);
	list_add(nodes, node4);
	list_add(nodes, node5);
	//Fin Nodos

	//Creacion Job 1
	copy1 = CreateCopy("Node3", "IP Nodo3", 3003, 2);
	copy2 = CreateCopy("Node2", "IP Nodo4", 3004, 919);
	copy3 = CreateCopy("Node5", "IP Nodo5", 3005, 227);

	copies1 = list_create();
	list_add(copies1, (void *) copy1);
	list_add(copies1, (void *) copy2);
	list_add(copies1, (void *) copy3);

	copy2_1 = CreateCopy("Node3", "IP Nodo3", 3003, 7);
	copy2_2 = CreateCopy("Node5", "IP Nodo5", 3005, 307);
	copy2_3 = CreateCopy("Node4", "IP Nodo4", 3004, 13);

	copies2 = list_create();
	list_add(copies2, (void *) copy2_1);
	list_add(copies2, (void *) copy2_2);
	list_add(copies2, (void *) copy2_3);

	copy3_1 = CreateCopy("Node1", "IP Nodo1", 3001, 17);
	copy3_2 = CreateCopy("Node3", "IP Nodo3", 3003, 421);
	copy3_3 = CreateCopy("Node5", "IP Nodo5", 3005, 23);
	copy3_4 = CreateCopy("Node2", "IP Nodo2", 3002, 29);
	copy3_5 = CreateCopy("Node4", "IP Nodo4", 3004, 821);

	copies3 = list_create();
	list_add(copies3, (void *) copy3_1);
	list_add(copies3, (void *) copy3_2);
	list_add(copies3, (void *) copy3_3);
	list_add(copies3, (void *) copy3_4);
	list_add(copies3, (void *) copy3_5);

	file = CreateFile("sarasa.txt");
	list_add(file->blocks, copies1);
	list_add(file->blocks, copies2);

	file2 = CreateFile("pepe.txt");
	list_add(file2->blocks, copies3);

	job = CreateJob(42, false);
	list_add(job->files, file2);
	list_add(job->files, file);
	//Fin Job 1

	//Creacion Job 2
	copy4_1 = CreateCopy("Node3", "IP Nodo3", 3003, 7);
	copy4_2 = CreateCopy("Node5", "IP Nodo5", 3005, 307);
	copy4_3 = CreateCopy("Node4", "IP Nodo4", 3004, 13);

	copies4 = list_create();
	list_add(copies4, (void *) copy4_1);
	list_add(copies4, (void *) copy4_2);
	list_add(copies4, (void *) copy4_3);

	unavailableCopy = CreateCopy("Node3", "IP Nodo3", 3003, 821);

	unavailableCopies = list_create();
	list_add(unavailableCopies, (void *) unavailableCopy);

	file3 = CreateFile("QuierePeroNoPuede.txt");
	list_add(file3->blocks, copies4);

	notAvailableFile = CreateFile("Inexistente.txt");
	list_add(notAvailableFile->blocks, unavailableCopies);

	job2 = CreateJob(23, false);
	job2->finalReduce->finalNode = strdup("");
	job2->finalReduce->nodeIP = strdup("");
	list_add(job2->files, file3);
	list_add(job2->files, notAvailableFile);
	//Fin Job 2
}

void planningTestFree() {
	list_iterate(nodes, (void *) showTasks);
	freeJob(job);
	freeJob(job2);
}

void jobMapTest() {
	printf("**************************jobMapTest****************************\n");
	job->finalReduce->finalNode = strdup("");
	job->finalReduce->nodeIP = strdup("");
	jobMap(job);
	jobMap(job2);
	printf("****************************************************************\n");
}

void RePlanTest() {
	printf("**************************RePlanTest****************************\n");
	printf("************************jobMapPlanning**************************\n");
	jobMap(job);
	job->finalReduce->finalNode = strdup("");
	job->finalReduce->nodeIP = strdup("");
	node5->active = 0;
	printf("************************RePlanningMap(1)*************************\n");
	t_map *map = list_get(job->maps, 1);
	rePlanMap(job, map);
	printf("****************************************************************\n");
}

void noCombinerPlanTest() {
	printf("************************ReducePlanTest**************************\n");
	printf("************************jobMapPlanning**************************\n");
	jobMap(job);
	jobMap(job2);
	printf("********************noCombinerReducePlanning*********************\n");
	noCombinerReducePlanning(job);
	printf("****************************************************************\n");
	printf("Esperados: (4-1) (1-1) (0-0) (1-0) (2-1)\n");
}

void combinerPartialsReducePlanTest() {
	printf("************************ReducePlanTest**************************\n");
	printf("************************jobMapPlanning**************************\n");
	jobMap(job);
	printf("******************combinerPartialsReducePlanning*****************\n");
	combinerPartialsReducePlanning(job);
	printf("****************************************************************\n");
}

void combinerPlanTest() {
	printf("************************ReducePlanTest**************************\n");
	printf("************************jobMapPlanning**************************\n");
	jobMap(job);
	jobMap(job2);
	printf("*********************combinerReducePlanning*********************\n");
	combinerPartialsReducePlanning(job);
	combinerFinalReducePlanning(job);
	printf("****************************************************************\n");
	printf("Esperados: (4-1) (1-1) (0-0) (1-2) (2-1)\n");
}
