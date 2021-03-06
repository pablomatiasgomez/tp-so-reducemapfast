#include "connections_node.h"
#include "connections.h"

#include <commons/collections/dictionary.h>

pthread_mutex_t activeNodesLock;
pthread_mutex_t standbyNodesLock;
pthread_mutex_t nodexMutexLock;
t_dictionary *activeNodesSockets;
t_dictionary *standbyNodesSockets;
t_dictionary *nodesMutex;

void* connections_node_checkAlive(void *param);

void connections_node_initialize() {
	if (pthread_mutex_init(&activeNodesLock, NULL) != 0 || pthread_mutex_init(&standbyNodesLock, NULL) != 0 || pthread_mutex_init(&nodexMutexLock, NULL) != 0) {
		log_error(mdfs_logger, "Error while trying to create new mutex");
		return;
	}
	activeNodesSockets = dictionary_create();
	standbyNodesSockets = dictionary_create();
	nodesMutex = dictionary_create();
}

void connections_node_shutdown() {
	void disconnectNode(node_connection_t *nodeConnection) {
		socket_close(nodeConnection->socket);
		connections_node_connection_free(nodeConnection);
	}
	pthread_mutex_lock(&activeNodesLock);
	dictionary_destroy_and_destroy_elements(activeNodesSockets, (void*) disconnectNode);
	pthread_mutex_lock(&standbyNodesLock);
	dictionary_destroy_and_destroy_elements(standbyNodesSockets, (void*) disconnectNode);

	void freeNodeMutex(pthread_mutex_t *nodeMutex) {
		pthread_mutex_destroy(nodeMutex);
		free(nodeMutex);
	}
	pthread_mutex_lock(&nodexMutexLock);
	dictionary_destroy_and_destroy_elements(nodesMutex, (void*) freeNodeMutex);

	pthread_mutex_destroy(&activeNodesLock);
	pthread_mutex_destroy(&standbyNodesLock);
	pthread_mutex_destroy(&nodexMutexLock);
}

pthread_mutex_t* connections_node_getNodeMutex(char *nodeId) {
	pthread_mutex_lock(&nodexMutexLock);
	pthread_mutex_t *nodeMutex = dictionary_get(nodesMutex, nodeId);
	pthread_mutex_unlock(&nodexMutexLock);
	if (!nodeMutex) {
		nodeMutex = malloc(sizeof(pthread_mutex_t));
		pthread_mutex_init(nodeMutex, NULL);
		pthread_mutex_lock(&nodexMutexLock);
		dictionary_put(nodesMutex, nodeId, nodeMutex);
		pthread_mutex_unlock(&nodexMutexLock);
	}
	return nodeMutex;
}

/*
 * Returns the connection of the node checking in active and standby connections.
 */
node_connection_t* connections_node_getNodeConnection(char *nodeId) {
	pthread_mutex_lock(&activeNodesLock);
	node_connection_t *nodeConnection = dictionary_get(activeNodesSockets, nodeId);
	pthread_mutex_unlock(&activeNodesLock);

	if (!nodeConnection) {
		pthread_mutex_lock(&standbyNodesLock);
		nodeConnection = dictionary_get(standbyNodesSockets, nodeId);
		pthread_mutex_unlock(&standbyNodesLock);
	}

	return nodeConnection;
}

node_connection_t* connections_node_getActiveNodeConnection(char *nodeId) {
	pthread_mutex_lock(&activeNodesLock);
	node_connection_t *nodeConnection = dictionary_get(activeNodesSockets, nodeId);
	pthread_mutex_unlock(&activeNodesLock);

	return nodeConnection;
}

void connections_node_removeActiveNodeConnection(char *nodeId) {
	pthread_mutex_lock(&activeNodesLock);
	node_connection_t *nodeConnection = dictionary_remove(activeNodesSockets, nodeId);
	pthread_mutex_unlock(&activeNodesLock);

	connections_node_connection_free(nodeConnection);
}

void connections_node_setAcceptedNodeConnection(char *nodeId, node_connection_t *nodeConnection) {
	// Remove it if it was "active"
	connections_node_removeActiveNodeConnection(nodeId);

	pthread_mutex_lock(&standbyNodesLock);
	node_connection_t *nodeConnectionOld = dictionary_remove(standbyNodesSockets, nodeId);
	dictionary_put(standbyNodesSockets, nodeId, nodeConnection);
	pthread_mutex_unlock(&standbyNodesLock);

	connections_node_connection_free(nodeConnectionOld);
}

bool connections_node_activateNode(char *nodeId) {
	pthread_mutex_lock(&standbyNodesLock);
	node_connection_t *nodeConnection = dictionary_remove(standbyNodesSockets, nodeId);
	pthread_mutex_unlock(&standbyNodesLock);

	if (nodeConnection) {
		pthread_mutex_lock(&activeNodesLock);
		dictionary_put(activeNodesSockets, nodeId, nodeConnection);
		pthread_mutex_unlock(&activeNodesLock);
		return 1;
	} else {
		return 0;
	}
}

bool connections_node_deactivateNode(char *nodeId) {
	pthread_mutex_lock(&activeNodesLock);
	node_connection_t *nodeConnection = dictionary_remove(activeNodesSockets, nodeId);
	pthread_mutex_unlock(&activeNodesLock);

	if (nodeConnection) {
		pthread_mutex_lock(&standbyNodesLock);
		dictionary_put(standbyNodesSockets, nodeId, nodeConnection);
		pthread_mutex_unlock(&standbyNodesLock);
		return 1;
	} else {
		return 0;
	}
}

int connections_node_getActiveConnectedCount() {
	pthread_mutex_lock(&activeNodesLock);
	int size = dictionary_size(activeNodesSockets);
	pthread_mutex_unlock(&activeNodesLock);

	return size;
}

bool connections_node_isActiveNode(char *nodeId) {
	return (bool) connections_node_getActiveNodeConnection(nodeId);
}

void* connections_node_accept(void *param) {
	node_connection_t *nodeConnection = (node_connection_t *) param;

	void *buffer;
	size_t sBuffer = 0;
	e_socket_status status = socket_recv_packet(nodeConnection->socket, &buffer, &sBuffer);

	if (status != SOCKET_ERROR_NONE) {
		return NULL;
	}

	// NODE DESERIALZE ..
	uint8_t isNewNode;
	uint16_t blocksCount;
	uint16_t listenPort;
	uint16_t sNodeId;
	char *nodeId;

	memcpy(&isNewNode, buffer, sizeof(isNewNode));

	memcpy(&blocksCount, buffer + sizeof(isNewNode), sizeof(blocksCount));
	blocksCount = ntohs(blocksCount);

	memcpy(&listenPort, buffer + sizeof(isNewNode) + sizeof(blocksCount), sizeof(listenPort));
	listenPort = ntohs(listenPort);

	memcpy(&sNodeId, buffer + sizeof(isNewNode) + sizeof(blocksCount) + sizeof(listenPort), sizeof(sNodeId));
	sNodeId = ntohs(sNodeId);
	nodeId = malloc(sizeof(char) * (sNodeId + 1));
	memcpy(nodeId, buffer + sizeof(isNewNode) + sizeof(blocksCount) + sizeof(listenPort) + sizeof(sNodeId), sNodeId);
	nodeId[sNodeId] = '\0';

	free(buffer);
	// ...

	//  Save the connection as a reference to this node and create the mutex
	nodeConnection->listenPort = listenPort;
	connections_node_setAcceptedNodeConnection(nodeId, nodeConnection);

	log_info(mdfs_logger, "Node connected. Name: %s. listenPort %d. blocksCount %d. New: %s", nodeId, listenPort, blocksCount, isNewNode ? "true" : "false");
	filesystem_addNode(nodeId, blocksCount, (bool) isNewNode);

	// Creates a new thread to check if the node is still alive..
	pthread_t nodeAliveCheckerTh;
	if (pthread_create(&nodeAliveCheckerTh, NULL, (void *) connections_node_checkAlive, (void *) nodeId)) {
		free(nodeId);
		log_error(mdfs_logger, "Error while trying to create new thread: nodeAliveCheckerTh");
	}
	pthread_detach(nodeAliveCheckerTh);

	return NULL;
}

bool connections_node_sendBlock(nodeBlockSendOperation_t *sendOperation) {
	uint8_t command = COMMAND_FS_TO_NODE_SET_BLOCK;
	uint16_t numBlock = sendOperation->blockIndex;

	size_t sBuffer = sizeof(command) + sizeof(numBlock);
	uint16_t numBlockSerialized = htons(numBlock);

	void *buffer = malloc(sBuffer);
	memcpy(buffer, &command, sizeof(command));
	memcpy(buffer + sizeof(command), &numBlockSerialized, sizeof(numBlock));

	// send the data in other packet.
	size_t sBlockData = strlen(sendOperation->block);

	pthread_mutex_t *nodeMutex = connections_node_getNodeMutex(sendOperation->node->id);
	pthread_mutex_lock(nodeMutex);
	node_connection_t *nodeConnection = connections_node_getActiveNodeConnection(sendOperation->node->id);
	if (!nodeConnection) {
		free(buffer);
		pthread_mutex_unlock(nodeMutex);
		return 0;
	}

	log_info(mdfs_logger, "Going to SET block %d to node %s.", sendOperation->blockIndex, sendOperation->node->id);

	e_socket_status status = socket_send_packet(nodeConnection->socket, buffer, sBuffer);
	if (status == SOCKET_ERROR_NONE) {
		status = socket_send_packet(nodeConnection->socket, sendOperation->block, sBlockData);
	}
	free(buffer);
	if (0 > status) {
		log_info(mdfs_logger, "Removing node %s because it was disconnected", sendOperation->node->id);
		connections_node_removeActiveNodeConnection(sendOperation->node->id);
		pthread_mutex_unlock(nodeMutex);
		return 0;
	}
	pthread_mutex_unlock(nodeMutex);

	return (status == SOCKET_ERROR_NONE);
}

char* connections_node_getBlock(file_block_t *fileBlock) {
	uint8_t command = COMMAND_FS_TO_NODE_GET_BLOCK;
	uint16_t numBlock = fileBlock->blockIndex;

	size_t sBuffer = sizeof(command) + sizeof(numBlock);

	uint16_t numBlockSerialized = htons(numBlock);

	void *buffer = malloc(sBuffer);
	memcpy(buffer, &command, sizeof(command));
	memcpy(buffer + sizeof(command), &numBlockSerialized, sizeof(numBlock));

	pthread_mutex_t *nodeMutex = connections_node_getNodeMutex(fileBlock->nodeId);
	pthread_mutex_lock(nodeMutex);
	node_connection_t *nodeConnection = connections_node_getActiveNodeConnection(fileBlock->nodeId);
	if (!nodeConnection) {
		free(buffer);
		pthread_mutex_unlock(nodeMutex);
		return NULL;
	}

	log_info(mdfs_logger, "Going to GET block %d from node %s.", fileBlock->blockIndex, fileBlock->nodeId);

	e_socket_status status = socket_send_packet(nodeConnection->socket, buffer, sBuffer);
	free(buffer);
	if (0 > SOCKET_ERROR_NONE) {
		log_info(mdfs_logger, "Removing node %s because it was disconnected", fileBlock->nodeId);
		connections_node_removeActiveNodeConnection(fileBlock->nodeId);
		pthread_mutex_unlock(nodeMutex);
		return NULL;
	}

	// Wait for the response..

	buffer = NULL;
	sBuffer = 0;
	status = socket_recv_packet(nodeConnection->socket, &buffer, &sBuffer);
	if (0 > status) {
		log_info(mdfs_logger, "Removing node %s because it was disconnected", fileBlock->nodeId);
		connections_node_removeActiveNodeConnection(fileBlock->nodeId);
		pthread_mutex_unlock(nodeMutex);
		return NULL;
	}
	pthread_mutex_unlock(nodeMutex);

	char *block = realloc(buffer, sBuffer + 1);
	block[sBuffer] = '\0';
	return block;
}

char* connections_node_getFileContent(char *nodeId, char *tmpFileName, size_t *tmpFileLength) {
	// Request the tmp file content.
	uint8_t command = COMMAND_NODE_GET_TMP_FILE_CONTENT;
	uint32_t sTmpName = strlen(tmpFileName);

	size_t sBuffer = sizeof(command) + sizeof(sTmpName) + sTmpName;
	uint32_t sTmpNameSerialized = htonl(sTmpName);

	void *buffer = malloc(sBuffer);
	memcpy(buffer, &command, sizeof(command));
	memcpy(buffer + sizeof(command), &sTmpNameSerialized, sizeof(sTmpName));
	memcpy(buffer + sizeof(command) + sizeof(sTmpName), tmpFileName, sTmpName);

	pthread_mutex_t *nodeMutex = connections_node_getNodeMutex(nodeId);
	pthread_mutex_lock(nodeMutex);
	node_connection_t *nodeConnection = connections_node_getActiveNodeConnection(nodeId);
	if (!nodeConnection) {
		free(buffer);
		pthread_mutex_unlock(nodeMutex);
		return NULL;
	}

	log_info(mdfs_logger, "Going to GET tmpFileContent '%s' from node %s.", tmpFileName, nodeId);

	e_socket_status status = socket_send_packet(nodeConnection->socket, buffer, sBuffer);
	free(buffer);
	if (0 > status) {
		log_info(mdfs_logger, "Removing node %s because it was disconnected", nodeId);
		connections_node_removeActiveNodeConnection(nodeId);
		pthread_mutex_unlock(nodeMutex);
		return NULL;
	}

	buffer = NULL;
	status = socket_recv_packet(nodeConnection->socket, &buffer, tmpFileLength);
	if (0 > status) {
		log_info(mdfs_logger, "Removing node %s because it was disconnected", nodeId);
		connections_node_removeActiveNodeConnection(nodeId);
		pthread_mutex_unlock(nodeMutex);
		return NULL;
	}
	pthread_mutex_unlock(nodeMutex);

	return buffer;
}

void* connections_node_checkAlive(void *param) {
	char *nodeIdPtr = (char *) param;
	char nodeId[strlen(nodeIdPtr) + 1];
	strcpy(nodeId, nodeIdPtr);
	free(nodeIdPtr);

	uint8_t buffer = COMMAND_NODE_CHECK_ALIVE;
	size_t sBuffer = sizeof(buffer);

	pthread_mutex_t *nodeMutex = connections_node_getNodeMutex(nodeId);

	while (1) {
		pthread_mutex_lock(nodeMutex);
		node_connection_t *nodeConnection = connections_node_getNodeConnection(nodeId);
		if (!nodeConnection) {
			pthread_mutex_unlock(nodeMutex);
			return NULL;
		}

		e_socket_status status = socket_send_packet(nodeConnection->socket, &buffer, sBuffer);

		if (0 > status) {
			log_info(mdfs_logger, "Removing node %s because it was disconnected", nodeId);
			connections_node_removeActiveNodeConnection(nodeId);
			// TODO sacarlo de standby tmb..
			pthread_mutex_unlock(nodeMutex);
			return NULL;
		}
		pthread_mutex_unlock(nodeMutex);

		usleep(1 * 1000 * 1000); // 1 seconds
	}
}

node_connection_t* connections_node_connection_create(int socket, char *ip) {
	node_connection_t *nodeConnection = malloc(sizeof(node_connection_t));
	nodeConnection->ip = strdup(ip);
	nodeConnection->socket = socket;
	return nodeConnection;
}

void connections_node_connection_free(node_connection_t *nodeConnection) {
	if (nodeConnection) {
		if (nodeConnection->ip) {
			free(nodeConnection->ip);
		}
		free(nodeConnection);
	}
}
