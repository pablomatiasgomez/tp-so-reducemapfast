#ifndef SRC_PLANNING_MAPPLANNING_H_
#define SRC_PLANNING_MAPPLANNING_H_

#include "../structs/job.h"

void jobMap(t_job *job);
void rePlanMap(t_job *job, uint32_t idMap);
char *getTime();

#endif
