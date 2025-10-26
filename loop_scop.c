/*
 * loop_scop.c
 *
 *  Created on: May 14, 2020
 *
 */

#include <stdlib.h>
#include <string.h>
#include "loop_scop.h"
#include "tool_isl.h"
#include "tool_debug.h"

static void loop_scop_init(__isl_keep loop_scop *loopScop);
static void calculate_relations_and_delta(__isl_keep loop_scop *loopScop);
static void calculate_islSchedule(__isl_keep loop_scop *loopScop);

__isl_give  loop_scop* loop_scop_extract_from_pet_scop(__isl_keep pet_scop *petScop, dapt_params *daptparams){
	loop_scop *loopScop = malloc(sizeof(loop_scop));

	loop_scop_init(loopScop);

	loopScop->loopSchedule = pet_scop_get_schedule(petScop);
	loopScop->loopDomain = isl_schedule_get_domain(loopScop->loopSchedule);
	loopScop->loopMap = isl_schedule_get_map(loopScop->loopSchedule);
	//loopScop->loopMap = isl_union_map_intersect_domain(loopScop->loopMap, isl_union_set_copy(loopScop->loopDomain));
	loopScop->loopMap = isl_union_map_gist_domain(loopScop->loopMap, isl_union_set_copy(loopScop->loopDomain));
	loopScop->loopReads = pet_scop_get_may_reads(petScop);
	loopScop->loopWrites = pet_scop_get_may_writes(petScop);

	loopScop->ctx = isl_union_map_get_ctx(loopScop->loopMap);
	loopScop->daptParams = daptparams;

	calculate_relations_and_delta(loopScop);
	calculate_islSchedule(loopScop);

	return loopScop;
}

__isl_give loop_scop *loop_scop_from_schedule_mapper_info(__isl_keep loop_scop *orygScop, __isl_keep schedule_mapper_info *scheduleMapperInfo){
	loop_scop *loopScop = malloc(sizeof(loop_scop));

	loop_scop_init(loopScop);
	loopScop->scheduleIdx = scheduleMapperInfo->scheduleIdx;
	loopScop->loopDomain = isl_union_map_domain(isl_union_map_reverse(isl_union_map_intersect_domain(isl_union_map_copy(scheduleMapperInfo->scheduleMapper), isl_union_set_copy(orygScop->loopDomain))));
	loopScop->loopMap = isl_union_map_apply_range(isl_union_map_reverse(isl_union_map_copy(scheduleMapperInfo->scheduleMapper)),isl_union_map_copy(scheduleMapperInfo->scheduleMap));
	//loopScop->loopMap = isl_union_map_intersect_domain(loopScop->loopMap, isl_union_set_copy(loopScop->loopDomain));
	//loopScop->loopMap = isl_union_map_gist_domain(loopScop->loopMap, isl_union_set_copy(loopScop->loopDomain));
	loopScop->loopReads = isl_union_map_apply_range(isl_union_map_reverse(isl_union_map_copy(scheduleMapperInfo->scheduleMapper)),isl_union_map_copy(orygScop->loopReads));
	loopScop->loopWrites = isl_union_map_apply_range(isl_union_map_reverse(isl_union_map_copy(scheduleMapperInfo->scheduleMapper)),isl_union_map_copy(orygScop->loopWrites));

	loopScop->ctx = orygScop->ctx;
	loopScop->orygScop = orygScop;
	loopScop->loopMapper = isl_union_map_copy(scheduleMapperInfo->scheduleMapper);
	loopScop->daptParams = orygScop->daptParams;

	calculate_relations_and_delta(loopScop);

	return loopScop;
}

__isl_null loop_scop* loop_scop_loop_scop_free(__isl_take loop_scop *loopScop){
	if(!loopScop != 0) return 0;

	isl_schedule_free(loopScop->loopSchedule);
	isl_union_set_free(loopScop->loopDomain);
	isl_union_map_free(loopScop->loopMap);
	isl_schedule_free(loopScop->loopislSchedule);
	isl_union_map_free(loopScop->loopIslMap);
	isl_union_map_free(loopScop->loopMapper);

	isl_union_map_free(loopScop->loopReads);
	isl_union_map_free(loopScop->loopWrites);

	isl_union_map_free(loopScop->dep_raw);
	isl_union_map_free(loopScop->dep_waw);
	isl_union_map_free(loopScop->dep_war);

	isl_union_map_free(loopScop->relations);
	isl_union_set_free(loopScop->delta);

	free(loopScop->loopReadWriteMapStr);

	free(loopScop);

	return 0;
}

__isl_give id_name* loop_scop_get_id_names(__isl_keep loop_scop *loopScop, const char *id, int count){
	id_name* retList =(id_name*)malloc(count*sizeof(id_name));

	if(loopScop->orygScop != 0)
		loopScop = loopScop->orygScop;

	if(loopScop->loopReadWriteMapStr == 0){
		char *mapStr = 0;
		isl_printer *printer = isl_printer_to_str(loopScop->ctx);

		mapStr = isl_union_map_to_str(loopScop->loopWrites);
		printer = isl_printer_print_str(printer, mapStr);
		free(mapStr);

		mapStr = isl_union_map_to_str(loopScop->loopReads);
		printer = isl_printer_print_str(printer, mapStr);
		free(mapStr);

		loopScop->loopReadWriteMapStr = isl_printer_get_str(printer);

		isl_printer_free(printer);
	}

	for(int i = 0; i<count; i++)
	{
		int idx = -1;
		sprintf(retList[i], "%s%i", id, i);
		while(strstr(loopScop->loopReadWriteMapStr, retList[i])){
			idx += 1;
			sprintf(retList[i],"%s%i_%i", id, i, idx);
		}
	}

	return retList;
}

void loop_scop_check_schedule_respects_deps( __isl_keep loop_scop *loopScop, __isl_keep isl_union_map *schedule){

	isl_union_map *scheduleBefore = isl_union_map_lex_lt_union_map(isl_union_map_copy(schedule),isl_union_map_copy(schedule));

	isl_debug_printf("\n#%s\n", "######################################################################");

	isl_debug_printf("\n#%s -> ", "Does global schedule respects oryginal loop RaW deps?");
	if(isl_union_map_is_subset(loopScop->dep_raw, scheduleBefore) == isl_bool_true){
		isl_debug_printf("%s\n", "True");
	}
	else{
		isl_debug_printf("%s\n", "False");
		loopScop->daptParams->dapt_respects_deps = isl_bool_false;
	}
	isl_debug_printf("\n#%s -> ", "Does global schedule respects oryginal loop WaW deps?");
	if(isl_union_map_is_subset(loopScop->dep_waw, scheduleBefore) == isl_bool_true){
		isl_debug_printf("%s\n", "True");
	}
	else{
		isl_debug_printf("%s\n", "False");
		loopScop->daptParams->dapt_respects_deps = isl_bool_false;
	}
	isl_debug_printf("\n#%s -> ", "Does global schedule respects oryginal loop WaR deps?");
	if(isl_union_map_is_subset(loopScop->dep_war, scheduleBefore) == isl_bool_true){
		isl_debug_printf("%s\n", "True");
	}
	else{
		isl_debug_printf("%s\n", "False");
		loopScop->daptParams->dapt_respects_deps = isl_bool_false;
	}
	isl_debug_printf("\n#%s\n", "######################################################################");

	isl_union_map_free(scheduleBefore);
}

void loop_scop_from_pet_debug_printf(__isl_take loop_scop *loopScop){
	if(!loopScop) return;
	isl_debug_printf("\n#%s\n", "######################################################################");
	isl_debug_printf("\n#%s\n", "oryginal loop:");
	isl_debug_printf_schedule("\n#schedule:\n%s\n", loopScop->loopSchedule);
	isl_debug_printf_union_set("\n#domain:\n%s\n", loopScop->loopDomain);
	isl_debug_printf_union_map("\n#schedule map:\n%s\n", loopScop->loopMap);
	isl_debug_printf_schedule("\n#isl schedule:\n%s\n", loopScop->loopislSchedule);
	isl_debug_printf_union_map("\n#isl schedule map:\n%s\n", loopScop->loopIslMap);
	isl_debug_printf_union_map("\n#writes:\n%s\n", loopScop->loopWrites);
	isl_debug_printf_union_map("\n#reads:\n%s\n", loopScop->loopReads);
	isl_debug_printf_union_map("\n#dep_raw:\n%s\n", loopScop->dep_raw);
	isl_debug_printf_union_map("\n#dep_waw:\n%s\n", loopScop->dep_waw);
	isl_debug_printf_union_map("\n#dep_war:\n%s\n", loopScop->dep_war);
	isl_debug_printf_union_map("\n#relations:\n%s\n", loopScop->relations);
	isl_debug_printf_union_set("\n#delta:\n%s\n", loopScop->delta);
	isl_debug_printf("\n#%s\n", "######################################################################");
}

void loop_scop_norm_debug_printf(__isl_take loop_scop *loopScop){
	if(!loopScop) return;
	isl_debug_printf("\n#%s\n", "######################################################################");
	isl_debug_printf_int("\n#normalized loop (%d):\n", loopScop->scheduleIdx);
	isl_debug_printf_union_map("\n#mapper:\n%s\n", loopScop->loopMapper);
	isl_debug_printf_union_set("\n#domain:\n%s\n", loopScop->loopDomain);
	isl_debug_printf_union_map("\n#schedule map:\n%s\n", loopScop->loopMap);
	isl_debug_printf_union_map("\n#writes:\n%s\n", loopScop->loopWrites);
	isl_debug_printf_union_map("\n#reads:\n%s\n", loopScop->loopReads);
	isl_debug_printf_union_map("\n#dep_raw:\n%s\n", loopScop->dep_raw);
	isl_debug_printf_union_map("\n#dep_waw:\n%s\n", loopScop->dep_waw);
	isl_debug_printf_union_map("\n#dep_war:\n%s\n", loopScop->dep_war);
	isl_debug_printf_union_map("\n#relations:\n%s\n", loopScop->relations);
	isl_debug_printf_union_set("\n#delta:\n%s\n", loopScop->delta);
	isl_debug_printf("\n#%s\n", "######################################################################");
}

//######################################################################################################

static void loop_scop_init(__isl_keep loop_scop *loopScop){
	loopScop->ctx = 0;
	loopScop->loopSchedule = 0;
	loopScop->loopDomain = 0;
	loopScop->loopMap = 0;
	loopScop->loopislSchedule = 0;
	loopScop->loopIslMap = 0;
	loopScop->loopMapper = 0;

	loopScop->loopReads = 0;
	loopScop->loopWrites = 0;

	loopScop->dep_raw = 0;
	loopScop->dep_war = 0;
	loopScop->dep_waw = 0;

	loopScop->relations = 0;
	loopScop->delta = 0;

	loopScop->loopReadWriteMapStr = 0;

	loopScop->orygScop = 0;
}

static void calculate_relations_and_delta(__isl_keep loop_scop *loopScop){
	isl_union_map * before = isl_union_map_lex_lt_union_map(isl_union_map_copy(loopScop->loopMap),isl_union_map_copy(loopScop->loopMap));

	loopScop->dep_raw = isl_union_map_intersect(
					isl_union_map_apply_range(isl_union_map_copy(loopScop->loopWrites),isl_union_map_reverse(isl_union_map_copy(loopScop->loopReads))),
					isl_union_map_copy(before));
	loopScop->dep_raw = isl_union_map_coalesce(loopScop->dep_raw);

	loopScop->dep_waw = isl_union_map_intersect(
					isl_union_map_apply_range(isl_union_map_copy(loopScop->loopWrites),isl_union_map_reverse(isl_union_map_copy(loopScop->loopWrites))),
					isl_union_map_copy(before));
	loopScop->dep_waw = isl_union_map_coalesce(loopScop->dep_waw);

	loopScop->dep_war = isl_union_map_intersect(
					isl_union_map_apply_range(isl_union_map_copy(loopScop->loopReads),isl_union_map_reverse(isl_union_map_copy(loopScop->loopWrites))),
					isl_union_map_copy(before));
	loopScop->dep_war = isl_union_map_coalesce(loopScop->dep_war);

	loopScop->relations = isl_union_map_union(isl_union_map_copy(loopScop->dep_raw), isl_union_map_copy(loopScop->dep_war));
	loopScop->relations = isl_union_map_union(loopScop->relations, isl_union_map_copy(loopScop->dep_waw));
	loopScop->relations = isl_union_map_coalesce(loopScop->relations);

	if(isl_union_map_is_empty(loopScop->relations)==isl_bool_false)	{
		isl_union_map* relations = 0;
		isl_union_map_foreach_map_in_map(loopScop->relations, clear_tuple_names_in_map, &relations);
		relations = isl_union_map_project_out_all_params(relations);
		loopScop->delta = isl_union_map_deltas(relations);
	}
	else{
		loopScop->delta = isl_union_set_empty(isl_union_map_get_space(loopScop->relations));
	}

	loopScop->delta = isl_union_set_coalesce(loopScop->delta);
	loopScop->delta = isl_union_set_remove_divs(loopScop->delta);

	isl_union_map_free(before);
}

static void calculate_islSchedule(__isl_keep loop_scop *loopScop){
	loopScop->loopislSchedule = isl_union_set_compute_schedule(isl_union_set_copy(loopScop->loopDomain), isl_union_map_copy(loopScop->relations),isl_union_map_copy(loopScop->relations));
	loopScop->loopIslMap = isl_schedule_get_map(loopScop->loopislSchedule);
}
