/*
 * loop_scop.h
 *
 *  Created on: May 14, 2020
 *
 */
#include <isl/ctx.h>
#include <isl/schedule.h>
#include <isl/union_set.h>
#include <isl/union_map.h>
#include <pet.h>
#include "dapt_params.h"

#ifndef LOOP_SCOP_H_
#define LOOP_SCOP_H_

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct schedule_mapper_info{
	int scheduleIdx;
	isl_union_map *scheduleMapper;
	isl_union_map *scheduleMap;
} schedule_mapper_info;

typedef struct loop_scop{
	isl_ctx *ctx;
	isl_schedule *loopSchedule;
	isl_union_set *loopDomain;
	isl_union_map *loopMap;
	isl_schedule *loopislSchedule;
	isl_union_map *loopIslMap;
	isl_union_map *loopMapper;

	isl_union_map *loopReads;
	isl_union_map *loopWrites;

	isl_union_map* dep_raw;
	isl_union_map* dep_war;
	isl_union_map* dep_waw;

	isl_union_map* relations;
	isl_union_set* delta;

	char *loopReadWriteMapStr;
	int scheduleIdx;
	struct loop_scop *orygScop;

	dapt_params *daptParams;
} loop_scop;

typedef char id_name[8];

__isl_give loop_scop* loop_scop_extract_from_pet_scop(__isl_keep pet_scop *petScop, dapt_params *daptparams);
__isl_give loop_scop* loop_scop_from_schedule_mapper_info(__isl_keep loop_scop *orygScop, __isl_keep schedule_mapper_info *scheduleMapperInfo);
__isl_null loop_scop* loop_scop_loop_scop_free(__isl_take loop_scop *loopScop);
__isl_give id_name* loop_scop_get_id_names(__isl_keep loop_scop *loopScop, const char *id, int count);
void loop_scop_check_schedule_respects_deps( __isl_keep loop_scop *loopScop, __isl_keep isl_union_map *schedule);

void loop_scop_from_pet_debug_printf(__isl_take loop_scop *loopScop);
void loop_scop_norm_debug_printf(__isl_take loop_scop *loopScop);

#if defined(__cplusplus)
}
#endif

#endif /* LOOP_SCOP_H_ */
