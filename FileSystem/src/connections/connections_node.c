#include "connections_node.h"
#include "connections.h"

#include <commons/collections/dictionary.h>

pthread_mutex_t activeNodesLock;
pthread_mutex_t standbyNodesLock;
t_dictionary *activeNodesSockets;
t_dictionary *standbyNodesSockets;

void connections_node_initialize() {
	if (pthread_mutex_init(&activeNodesLock, NULL) != 0 || pthread_mutex_init(&standbyNodesLock, NULL) != 0) {
		log_error(mdfs_logger, "Error while trying to create new mutex");
		return;
	}
	activeNodesSockets = dictionary_create();
	standbyNodesSockets = dictionary_create();
}

void connections_node_shutdown() {
	pthread_mutex_destroy(&activeNodesLock);
	pthread_mutex_destroy(&standbyNodesLock);

	void desconectNode(int *nodeSocket) {
		socket_close(*nodeSocket);
		free(nodeSocket);
	}
	dictionary_destroy_and_destroy_elements(activeNodesSockets, (void*) desconectNode);
	dictionary_destroy_and_destroy_elements(standbyNodesSockets, (void*) desconectNode);
}

int connections_node_getNodeSocket(char *nodeId) {
	pthread_mutex_lock(&activeNodesLock);
	int *nodeSocket = dictionary_get(activeNodesSockets, nodeId);
	pthread_mutex_unlock(&activeNodesLock);

	return (nodeSocket ? *nodeSocket : -1);
}

void connections_node_setNodeSocket(char *nodeId, int socket) {
	int *socketAcceptedPtr = malloc(sizeof(int));
	*socketAcceptedPtr = socket;

	pthread_mutex_lock(&standbyNodesLock);
	dictionary_put(standbyNodesSockets, nodeId, socketAcceptedPtr);
	pthread_mutex_unlock(&standbyNodesLock);
}

void connections_node_removeNodeSocket(char *nodeId) {
	pthread_mutex_lock(&activeNodesLock);
	int *socket = dictionary_remove(activeNodesSockets, nodeId);
	pthread_mutex_unlock(&activeNodesLock);

	if (socket) {
		free(socket);
	}
}

void connections_node_activateNode(char *nodeId) {
	pthread_mutex_lock(&standbyNodesLock);
	int *standbySocketPtr = (int *) dictionary_remove(standbyNodesSockets, nodeId);
	pthread_mutex_unlock(&standbyNodesLock);

	if (standbySocketPtr) {
		pthread_mutex_lock(&activeNodesLock);
		dictionary_put(activeNodesSockets, nodeId, standbySocketPtr);
		pthread_mutex_unlock(&activeNodesLock);
	}
}

void connections_node_deactivateNode(char *nodeId) {
	pthread_mutex_lock(&activeNodesLock);
	int *activeSocketPtr = (int *) dictionary_remove(activeNodesSockets, nodeId);
	pthread_mutex_unlock(&activeNodesLock);

	if (activeSocketPtr) {
		pthread_mutex_lock(&standbyNodesLock);
		dictionary_put(standbyNodesSockets, nodeId, activeSocketPtr);
		pthread_mutex_unlock(&standbyNodesLock);
	}
}

int connections_node_getActiveConnectedCount() {
	pthread_mutex_lock(&activeNodesLock);
	return dictionary_size(activeNodesSockets);
	pthread_mutex_unlock(&activeNodesLock);
}

bool connections_node_isActiveNode(char *nodeId) {
	return connections_node_getNodeSocket(nodeId) != -1;
}

void connections_node_accept(int socketAccepted, char *clientIP) {

	void *buffer;
	size_t sBuffer = 0;
	e_socket_status status = socket_recv_packet(socketAccepted, &buffer, &sBuffer);

	if (status != SOCKET_ERROR_NONE) {
		return;
	}

	// NODE DESERIALZE ..
	uint8_t isNewNode;
	uint16_t blocksCount;
	uint16_t listenPort;
	uint16_t sName;
	char *nodeName;

	memcpy(&isNewNode, buffer, sizeof(isNewNode));

	memcpy(&blocksCount, buffer + sizeof(isNewNode), sizeof(blocksCount));
	blocksCount = ntohs(blocksCount);

	memcpy(&listenPort, buffer + sizeof(isNewNode) + sizeof(blocksCount), sizeof(listenPort));
	listenPort = ntohs(listenPort);

	memcpy(&sName, buffer + sizeof(isNewNode) + sizeof(blocksCount) + sizeof(listenPort), sizeof(sName));
	sName = ntohs(sName);
	nodeName = malloc(sizeof(char) * (sName + 1));
	memcpy(nodeName, buffer + sizeof(isNewNode) + sizeof(blocksCount) + sizeof(listenPort) + sizeof(sName), sName);
	nodeName[sName] = '\0';

	free(buffer);
	// ...

	//  Save the socket as a reference to this node.
	// TODO ver en que usar la ip. ( EL NODO TIENE QUE MANDAR PUERTO DE LISTEN.. )
	connections_node_setNodeSocket(nodeName, socketAccepted);

	log_info(mdfs_logger, "Node connected. Name: %s. listenPort %d. blocksCount %d. New: %s", nodeName, listenPort, blocksCount, isNewNode ? "true" : "false");
	filesystem_addNode(nodeName, blocksCount, (bool) isNewNode);

	free(nodeName);
}

bool connections_node_sendBlock(nodeBlockSendOperation_t *sendOperation) {

	int nodeSocket = connections_node_getNodeSocket(sendOperation->node->id);
	if (nodeSocket == -1) {
		return 0;
	}

	uint8_t command = NODE_COMMAND_SET_BLOCK;
	uint16_t numBlock = sendOperation->blockIndex;
	uint32_t sBlockData = strlen(sendOperation->block);

	size_t sBuffer = sizeof(command) + sizeof(numBlock) + sizeof(sBlockData) + sBlockData;

	uint16_t numBlockSerialized = htons(numBlock);
	uint32_t sBlockDataSerialized = htonl(sBlockData);

	void *buffer = malloc(sBuffer);
	memcpy(buffer, &command, sizeof(command));
	memcpy(buffer + sizeof(command), &numBlockSerialized, sizeof(numBlock));
	memcpy(buffer + sizeof(command) + sizeof(numBlock), &sBlockDataSerialized, sizeof(sBlockData));
	memcpy(buffer + sizeof(command) + sizeof(numBlock) + sizeof(sBlockData), sendOperation->block, sBlockData);

	e_socket_status status = socket_send_packet(nodeSocket, buffer, sBuffer);

	free(buffer);

	if (0 > status) {
		log_info(mdfs_logger, "Removing node %s because it was disconnected", sendOperation->node->id);
		connections_node_removeNodeSocket(sendOperation->node->id);
		return 0;
	}

	// TODO, hacer un recv y esperar espuesta OK (hacer que el nodo mande. )
	return (status == SOCKET_ERROR_NONE);
}

char* connections_node_getBlock(file_block_t *fileBlock) {

	int nodeSocket = connections_node_getNodeSocket(fileBlock->nodeId);
	if (nodeSocket == -1) {
		return NULL;
	}

	uint8_t command = NODE_COMMAND_GET_BLOCK;
	uint16_t numBlock = fileBlock->blockIndex;

	size_t sBuffer = sizeof(command) + sizeof(numBlock);

	uint16_t numBlockSerialized = htons(numBlock);

	void *buffer = malloc(sBuffer);
	memcpy(buffer, &command, sizeof(command));
	memcpy(buffer + sizeof(command), &numBlockSerialized, sizeof(numBlock));

	e_socket_status status = socket_send_packet(nodeSocket, buffer, sBuffer);

	free(buffer);

	if (status != SOCKET_ERROR_NONE) {
		return NULL;
	}

	// Wait for the response..

	buffer = NULL;
	sBuffer = 0;
	status = socket_recv_packet(nodeSocket, &buffer, &sBuffer);

	if (0 > status) {
		log_info(mdfs_logger, "Removing node %s because it was disconnected", fileBlock->nodeId);
		connections_node_removeNodeSocket(fileBlock->nodeId);
		return NULL;
	}

	char *block = malloc(sBuffer + 1);
	memcpy(block, buffer, sBuffer);
	block[sBuffer] = '\0';

	free(buffer);

	return block;
}
