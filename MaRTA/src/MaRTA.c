#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <commons/config.h>
#include <commons/collections/list.h>
#include "structs/node.h"
#include "structs/job.h"
#include "Serialize/serialize.h"
#include "MaRTA.h"
#include "../../utils/socket.h"
#include "../test/PlanningTest.h"
#include "../test/SerializeTest.h"

typedef struct {
	uint16_t listenPort;
	char *fsIP;
	uint16_t fsPort;
} t_configMaRTA;

t_configMaRTA *cfgMaRTA;
t_log *logger;
t_list *nodes;

int fdListener;
int fdAccepted;
bool exitMaRTA;
uint16_t cantJobs;

int initConfig(char* configFile);
void freeMaRTA();

int main(int argc, char *argv[]) {

	logger = log_create("MaRTA.log", "MaRTA", 1, log_level_from_string("TRACE"));
	exitMaRTA = false;
	cantJobs = 0;
	if (argc != 2) {
		log_error(logger, "Missing config file");
		freeMaRTA();
		return EXIT_FAILURE;
	}
	if (!initConfig(argv[1])) {
		log_error(logger, "Config failed");
		freeMaRTA();
		return EXIT_FAILURE;
	}

	nodes = list_create();

	//planningTestSetup();
	//noCombinerPlanTest();
	//combinerPlanTest();
	//planningTestFree();

	fdListener = socket_listen(cfgMaRTA->listenPort);
	seializeCompleteJobTest();

	list_destroy_and_destroy_elements(nodes, (void *) freeNode);
	freeMaRTA();
	return EXIT_SUCCESS;
}

int initConfig(char* configFile) {

	t_config* _config;
	int failure = 0;

	int getConfigInt(char *property) {
		if (config_has_property(_config, property)) {
			return config_get_int_value(_config, property);
		}

		failure = 1;
		log_error(logger, "Config not found for key %s", property);
		return -1;
	}

	char* getCongifString(char *property) {
		if (config_has_property(_config, property)) {
			return config_get_string_value(_config, property);
		}

		failure = 1;
		log_error(logger, "Config not found for key %s", property);
		return "";
	}

	_config = config_create(configFile);

	cfgMaRTA = malloc(sizeof(t_configMaRTA));

	log_info(logger, "Loading config...");

	cfgMaRTA->listenPort = getConfigInt("PUERTO_LISTEN");
	cfgMaRTA->fsIP = strdup(getCongifString("IP_FILE_SYSTEM"));
	cfgMaRTA->fsPort = getConfigInt("PUERTO_FILE_SYSTEM");

	if (!failure) {
		log_info(logger, "Port to listen: %d", cfgMaRTA->listenPort);
		log_info(logger, "FileSystem IP: %s", cfgMaRTA->fsIP);
		log_info(logger, "FileSystem Port: %d", cfgMaRTA->fsPort);
	}

	config_destroy(_config);
	return !failure;
}

void freeMaRTA() {
	if (cfgMaRTA->fsIP) {
		free(cfgMaRTA->fsIP);
	}
	free(cfgMaRTA);
	log_destroy(logger);
}
