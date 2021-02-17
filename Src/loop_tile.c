/*
 * loop_tile.c
 *
 *  Created on: May 15, 2020
 *
 */

#include <string.h>
#include <isl/id.h>

#include "loop_tile.h"
#include "tool_isl.h"
#include "tool_debug.h"
#include "tool_string.h"

typedef struct analyze_dim_info{
	int analyzeDim;
	loop_tile_info* loopTile;
} analyze_dim_info;

//#############################################################################################################################################

static isl_bool loop_tile_continue_search(__isl_keep loop_tile_info *loopTile, int *hyperplaneStartsFrom);
static void loop_tile_init(__isl_take loop_tile_info* loopTile);
static void loop_tile_read_dim_info_and_set_params_bounds(__isl_keep loop_tile_info* loopTile);
static isl_stat loop_tile_read_dim_info_size(__isl_take isl_set *set, void *user);
static isl_stat loop_tile_read_dim_info_info(__isl_take isl_map *map, void *user);
static void loop_tile_read_delta_info(__isl_keep loop_tile_info* loopTile);
static isl_stat loop_tile_read_delta_set(__isl_take isl_set *set, void *user);
static __isl_give isl_union_set* loop_tile_get_delta_corrector(__isl_keep loop_tile_info* loopTile);
static void loop_tile_create_tile_equations(__isl_keep loop_tile_info* loopTile);
static isl_stat loop_tile_create_tile_equations_point(__isl_take isl_point *pnt, void *user);
static void loop_tile_create_hyperplanes(__isl_keep loop_tile_info* loopTile);
static isl_bool loop_tile_create_diamond_hyperplanes_for_dim(__isl_keep loop_tile_info* loopTile, int dim);

static isl_bool loop_tile_create_parallel_free_hyperplane(__isl_keep loop_tile_info* loopTile, int dim);
static void loop_tile_create_parallel_free_hyperplane_test(__isl_keep loop_tile_info* loopTile);

static void loop_tile_create_hyperplane_for_dim(__isl_keep loop_tile_info* loopTile, int dim);
static isl_bool loop_tile_check_hyperplane(__isl_keep loop_tile_info* loopTile, __isl_keep isl_set *hyperplane);
static void loop_tile_add_hyperplane(__isl_keep loop_tile_info* loopTile, __isl_keep isl_set *hyperplane);
static void loop_tile_create_spaces(__isl_keep loop_tile_info* loopTile);
static isl_stat loop_tile_space_from_hyperplane_set(__isl_take isl_set *set, void *user);
static isl_stat loop_tile_space_from_hyperplanes(__isl_take isl_point *pnt, void *user);
static void loop_tile_space_to_space_mapper(__isl_keep loop_tile_info* loopTile, int spaceInfoPos);
static void loop_tile_calculate_tile_scop(__isl_keep loop_tile_info* loopTile);
static isl_stat loop_tile_create_tile_scop_mapper(__isl_take isl_set *set, void *user);
static isl_stat loop_tile_calculate_tile_delta(__isl_take isl_set *set, void *user);
static void loop_tile_read_tile_delta_info(__isl_keep loop_tile_info* loopTile);
static isl_stat loop_tile_read_tile_delta_set(__isl_take isl_set *set, void *user);
static void loop_tile_read_tile_delta_info(__isl_keep loop_tile_info* loopTile);
static void loop_tile_create_wf_equations(__isl_keep loop_tile_info* loopTile);
static isl_stat loop_tile_create_wf_equations_point(__isl_take isl_point *pnt, void *user);
static void loop_tile_calculate_wf_hyperplane(__isl_keep loop_tile_info* loopTile);

static void loop_tile_create_parallel_free_schedule(__isl_keep loop_tile_info* loopTile);
static isl_stat loop_tile_parallel_free_schedule_for_map(__isl_take isl_map *map, void *user);
static __isl_give isl_union_map * loop_tile_space_to_parallel_free_schedule(__isl_take isl_union_map *wafefrontScheduleMap, __isl_keep loop_tile_info* loopTile, const char *tupleName, char *mapToStr, id_name wfName);

static void loop_tile_create_wafefront_schedule(__isl_keep loop_tile_info* loopTile);
static isl_stat loop_tile_wafefront_schedule_for_map(__isl_take isl_map *map, void *user);

static __isl_give isl_union_map * loop_tile_space_to_wafefront_schedule(__isl_take isl_union_map *wafefrontScheduleMap, __isl_keep loop_tile_info* loopTile, const char *tupleName, char *mapToStr, id_name wfName);

static void loop_tile_create_wafefront_code(__isl_keep loop_tile_info* loopTile);
static void loop_tile_create_approximated_relations(__isl_keep loop_tile_info* loopTile);
static isl_stat loop_tile_create_approximated_relations_for_set(__isl_take isl_set *set, void *user);
static void loop_tile_create_approximated_relations_for_x_set(int value, int pos, __isl_keep loop_tile_info* loopTile);


//#############################################################################################################################################

__isl_give loop_tile_list* loop_tile_list_from_loop_scop_list(loop_scop_list *loopScopList){
	loop_scop *loopScop = loopScopList->loopScops[0]->orygScop;
	loop_tile_list *loopTileList = malloc(sizeof(loop_tile_list));

	loopTileList->count = loopScopList->count;
	loopTileList->loopsTile = malloc(loopTileList->count*sizeof(loop_tile_info));
	loopTileList->wafefrontTileSchedule = 0;

	for(int i=0; i<loopTileList->count; i++){
		loopTileList->loopsTile[i] = loop_tile_from_loop_scop(loopScopList->loopScops[i]);
	}

	for(int i=0; i<loopTileList->count; i++){
		if(!loopTileList->wafefrontTileSchedule){
			loopTileList->wafefrontTileSchedule = isl_union_map_copy(loopTileList->loopsTile[i]->wafefrontTileSchedule);
		}
		loopTileList->wafefrontTileSchedule = isl_union_map_coalesce(isl_union_map_union(loopTileList->wafefrontTileSchedule, isl_union_map_copy(loopTileList->loopsTile[i]->wafefrontTileSchedule)));
	}

	isl_debug_printf("\n#%s\n", "######################################################################");
	isl_debug_printf_union_map("\n#global schedule:\n%s\n", loopTileList->wafefrontTileSchedule);
	isl_debug_printf("\n#%s\n", "######################################################################");

	loop_scop_check_schedule_respects_deps(loopScop, loopTileList->wafefrontTileSchedule);

	return loopTileList;
}

__isl_null loop_tile_list* loop_tile_list_free(loop_tile_list *loopTileList){
	if(!loopTileList) return 0;

	for(int i=0; i<loopTileList->count; i++){
		loop_tile_free(loopTileList->loopsTile[i]);
	}
	free(loopTileList->loopsTile);
	isl_union_map_free(loopTileList->wafefrontTileSchedule);

	return 0;
}

__isl_give loop_tile_info* loop_tile_from_loop_scop(__isl_keep loop_scop* loopScop){
	loop_tile_info *loopTile = 0;
	int hyperplaneStartsFrom = 0;

	isl_debug_printf("\n#%s\n", "######################################################################");
	isl_debug_printf_int("\n#normalized loop (%d):\n", loopScop->scheduleIdx);
	isl_debug_printf("\n%s\n", "#loop tile start");

	do{
		loop_tile_free(loopTile);
		loopTile = malloc(sizeof(loop_tile_info));
		loop_tile_init(loopTile);

		loopTile->scheduleIdx = loopScop->scheduleIdx;
		loopTile->daptParams = loopScop->daptParams;
		loopTile->loopScop = loopScop;
		loopTile->hyperplaneStartsFrom = hyperplaneStartsFrom;

		loop_tile_read_dim_info_and_set_params_bounds(loopTile);


		loop_tile_read_delta_info(loopTile);
		loop_tile_create_tile_equations(loopTile);
		loop_tile_create_hyperplanes(loopTile);
		if(loopTile->hyperplaneCheckInfo != 0){
			loop_tile_create_spaces(loopTile);
			loop_tile_calculate_tile_scop(loopTile);
			loop_tile_read_tile_delta_info(loopTile);
			loop_tile_create_wf_equations(loopTile);
			loop_tile_calculate_wf_hyperplane(loopTile);
		}
	}
	while(loop_tile_continue_search(loopTile, &hyperplaneStartsFrom)==isl_bool_true);

	if(loopTile->parallelFreeExists == isl_bool_true){
		loop_tile_create_parallel_free_schedule(loopTile);
	}
	else{
		loop_tile_create_wafefront_schedule(loopTile);
	}

	loop_tile_create_wafefront_code(loopTile);

	loop_tile_create_approximated_relations(loopTile);

	isl_debug_printf("\n%s\n", "#loop tile stop");
	isl_debug_printf("\n#%s\n", "######################################################################");

	return loopTile;
}

__isl_null loop_tile_info* loop_tile_free(__isl_take loop_tile_info* loopTile){
	if(!loopTile) return 0;

	free(loopTile->domainParams);

	free(loopTile->loopDimInfo);
	if(loopTile->hyperplaneCheckInfo){
		for(int i=0; i<loopTile->loopDimSize;i++){
			free(loopTile->hyperplaneCheckInfo->statementList[i]);
		}
		free(loopTile->hyperplaneCheckInfo->statementList);
		free(loopTile->hyperplaneCheckInfo);
	}
	if(loopTile->spaceInfo){
		for(int i=0; i<loopTile->hyperplaneDimSize;i++){
			if(loopTile->spaceInfo[i].vector){
				free(loopTile->spaceInfo[i].vector);
			}
		}
		free(loopTile->spaceInfo);
	}

	isl_union_set_free(loopTile->deltaSetToScan);
	isl_set_list_free(loopTile->spaceInfoHyperplanesList);
	isl_map_free(loopTile->spaceInfoMapper);
	isl_union_map_free(loopTile->tileMapper);
	isl_union_set_free(loopTile->tileDelta);
	isl_union_set_free(loopTile->tileDeltaSetToScan);
	isl_set_free(loopTile->wafefrontHyperplane);
	isl_union_map_free(loopTile->wafefrontScheduleMap);
	isl_union_map_free(loopTile->wafefrontTileSchedule);

	isl_id_list_free(loopTile->iterators);

	isl_union_map_free(loopTile->approximatedRelationsTpl);
	isl_union_map_free(loopTile->approximatedRelations);

	free(loopTile);

	return 0;
}

//#############################################################################################################################################

static isl_bool loop_tile_continue_search(__isl_keep loop_tile_info *loopTile, int *hyperplaneStartsFrom){
	int noOrderDimCount = 0;

	if(loopTile->hyperplaneCheckInfo!=0){
		if(loopTile->hyperplaneCheckInfo->lastId > 1 && isl_set_is_empty(loopTile->wafefrontHyperplane)==isl_bool_false ) return isl_bool_false;
	}

	if(*hyperplaneStartsFrom==0){
		for(int i=0; i<loopTile->loopDimSize; i++){
			if(loopTile->loopDimInfo[i].isOrder == isl_bool_false) break;
			*hyperplaneStartsFrom +=1;
		}
	}

	*hyperplaneStartsFrom +=1;
	for(int i=*hyperplaneStartsFrom; i<loopTile->loopDimSize; i++){
		if(loopTile->loopDimInfo[i].isOrder == isl_bool_false) break;
		*hyperplaneStartsFrom +=1;
	}

	for(int i=*hyperplaneStartsFrom; i<loopTile->loopDimSize; i++){
		if(loopTile->loopDimInfo[i].isOrder == isl_bool_false) noOrderDimCount +=1;
	}

	if(noOrderDimCount < 2) return isl_bool_false;

	isl_debug_printf("\n#%s\n", "######################################################################");
	isl_debug_printf("\n#%s\n", "continue search for hyperplanes");
	isl_debug_printf("\n#%s\n", "######################################################################");

	return isl_bool_true;
}

static void loop_tile_init(__isl_take loop_tile_info* loopTile){
	loopTile->scheduleIdx = 0;
	loopTile->loopDimSize = 0;
	loopTile->hyperplaneDimSize = 0;
	loopTile->hyperplaneStartsFrom = 0;

	loopTile->domainParams = 0;

	loopTile->loopScop = 0;
	loopTile->daptParams = 0;
	loopTile->loopDimInfo = 0;
	loopTile->hyperplaneCheckInfo = 0;
	loopTile->spaceInfo = 0;

	loopTile->deltaSetToScan = 0;
	loopTile->spaceInfoHyperplanesList = 0;
	loopTile->spaceInfoMapper = 0;
	loopTile->tileMapper = 0;
	loopTile->tileDelta = 0;
	loopTile->tileDeltaSetToScan = 0;
	loopTile->parallelFreeExists = isl_bool_false;
	loopTile->wafefrontHyperplane = 0;
	loopTile->wafefrontScheduleMap = 0;
	loopTile->wafefrontTileSchedule = 0;

	loopTile->iterators = 0;

	loopTile->approximatedRelationsTpl = 0;
	loopTile->approximatedRelations = 0;
}

static void loop_tile_read_dim_info_and_set_params_bounds(__isl_keep loop_tile_info* loopTile){
	isl_debug_printf("\n%s\n", "#dim info analyze start");

	char *loopDomainStr = isl_union_set_to_str(loopTile->loopScop->loopDomain);
	isl_union_map *loopReverseMap = isl_union_map_reverse(isl_union_map_copy(loopTile->loopScop->loopMap));

	loopTile->domainParams = str_substring(loopDomainStr, "[", "] -> {");

	isl_union_set_foreach_set_in_set(loopTile->loopScop->loopDomain, loop_tile_read_dim_info_size, loopTile);
	isl_union_map_foreach_map_in_map(loopReverseMap,loop_tile_read_dim_info_info, loopTile);

	isl_union_map_free(loopReverseMap);
	free(loopDomainStr);

	isl_debug_printf("\n%s\n", "#dim info analyze stop");
}

static isl_stat loop_tile_read_dim_info_size(__isl_take isl_set *set, void *user){
	loop_tile_info* loopTile = (loop_tile_info*)user;

	loopTile->loopDimSize = isl_set_dim(set, isl_dim_set);

	loopTile->loopDimInfo = malloc(loopTile->loopDimSize*sizeof(loop_tile_info));

	for(int i=0; i<loopTile->loopDimSize; i++){
		loopTile->loopDimInfo[i].dimName = isl_set_get_dim_name(set, isl_dim_set, i);
		loopTile->loopDimInfo[i].isOrder = isl_bool_true;
		loopTile->loopDimInfo[i].isNoLowerBound = isl_bool_false;
		loopTile->loopDimInfo[i].isNoUpperBound = isl_bool_false;
	}

	isl_set_free(set);

	return isl_stat_error;
}

static isl_stat loop_tile_read_dim_info_info(__isl_take isl_map *map, void *user){
	loop_tile_info* loopTile = (loop_tile_info*)user;
	const char *dimName = 0;

	for(int i=0; i<loopTile->loopDimSize; i++){
		dimName = isl_map_get_dim_name(map,isl_dim_in, i);
		if(dimName!=0)
			loopTile->loopDimInfo[i].isOrder = isl_bool_false;
	}

	isl_map_free(map);

	return isl_stat_ok;
}

static void loop_tile_read_delta_info(__isl_keep loop_tile_info* loopTile){
	isl_debug_printf("\n%s\n", "#delta analyze start");

	isl_union_set *delta = isl_union_set_coalesce(isl_union_set_intersect(isl_union_set_copy_by_str(loopTile->loopScop->delta), loop_tile_get_delta_corrector(loopTile)));

	if(isl_union_set_is_empty(loopTile->loopScop->delta)==isl_bool_true){
		loopTile->deltaSetToScan = isl_union_set_read_from_str(loopTile->loopScop->ctx, "{}");
	}else{
		analyze_dim_info analyzeDimInfo;

		analyzeDimInfo.analyzeDim = 0;
		analyzeDimInfo.loopTile = loopTile;

		isl_union_set_foreach_set_in_set(delta, loop_tile_read_delta_set, &analyzeDimInfo);
	}

	isl_debug_printf_union_set("\n#delta:\n%s\n", delta);

	for(int i=0; i<loopTile->hyperplaneStartsFrom; i++){
		isl_debug_printf("\n#dim [%s]:", loopTile->loopDimInfo[i].dimName);
		isl_debug_printf(" %s", "is wavefront order");
		isl_debug_printf("\n%s", "");
	}

	for(int i=loopTile->hyperplaneStartsFrom; i<loopTile->loopDimSize; i++){
		isl_debug_printf("\n#dim [%s]:", loopTile->loopDimInfo[i].dimName);
		if(loopTile->loopDimInfo[i].isOrder == isl_bool_true){
			isl_debug_printf(" %s", "is order");
		} else
		if(loopTile->loopDimInfo[i].isNoLowerBound == isl_bool_true || loopTile->loopDimInfo[i].isNoUpperBound == isl_bool_true ){
			isl_debug_printf(" %s", "is parameter");
			if(loopTile->loopDimInfo[i].isNoLowerBound == isl_bool_true){
				isl_debug_printf(", %s", "is negative");
			}
			if(loopTile->loopDimInfo[i].isNoUpperBound == isl_bool_true){
				isl_debug_printf(", %s", "is positive");
			}
		} else {
			isl_debug_printf(" %s", "is bounded");
		}
		isl_debug_printf("\n%s", "");
	}

	isl_union_set_free(delta);

	isl_debug_printf("\n%s\n", "#delta analyze stop");
}

static __isl_give isl_union_set* loop_tile_get_delta_corrector(__isl_keep loop_tile_info* loopTile){
	char equationStr[4096];
	equationStr[0] = '\0';

	sprintf(&equationStr[strlen(equationStr)],"{ [%s", loopTile->loopDimInfo[0].dimName);
	for(int i=1; i<loopTile->loopDimSize; i++){
		sprintf(&equationStr[strlen(equationStr)],", %s", loopTile->loopDimInfo[i].dimName);
	}
	sprintf(&equationStr[strlen(equationStr)],"] : true");

	for(int i=0; i<loopTile->hyperplaneStartsFrom; i++){
		sprintf(&equationStr[strlen(equationStr)]," and %s=0", loopTile->loopDimInfo[i].dimName);
	}
	sprintf(&equationStr[strlen(equationStr)]," }");

	return isl_union_set_read_from_str(loopTile->loopScop->ctx, equationStr);
}

static isl_stat loop_tile_read_delta_set(__isl_take isl_set *set, void *user){
	analyze_dim_info *analyzeDimInfo = (analyze_dim_info*) user;
	loop_tile_info *loopTile = analyzeDimInfo->loopTile;
	int analyzeDim = analyzeDimInfo->analyzeDim;

	isl_set *delta_set = isl_set_remove_divs(set);

	if(analyzeDim < loopTile->loopDimSize){
		char dimNamesList[256];
		isl_bool lower_bound, upper_bound;
		const char* dimName = isl_set_get_dim_name(delta_set, isl_dim_set, analyzeDim);

		dimNamesList[0]='\0';
		{
			int i;
			for(i=0; i < loopTile->loopDimSize; i++ ){
				sprintf(&dimNamesList[strlen(dimNamesList)], ",%s", isl_set_get_dim_name(delta_set,isl_dim_set, i));
			}
		}
		dimNamesList[0] = ' ';

		{
			isl_set* tmpSet = isl_set_copy(delta_set);
			if(analyzeDim > 0)
			{
				tmpSet = isl_set_remove_dims(tmpSet, isl_dim_set, 0, analyzeDim);
			}
			if(loopTile->loopDimSize - (analyzeDim + 1) > 0)
			{
				tmpSet = isl_set_remove_dims(tmpSet, isl_dim_set, 1, loopTile->loopDimSize - (analyzeDim + 1));
			}

			lower_bound = isl_set_dim_has_lower_bound(tmpSet, isl_dim_set, 0);
			upper_bound = isl_set_dim_has_upper_bound(tmpSet, isl_dim_set, 0);

			isl_set_free(tmpSet);
		}

		{
			char* minValStr = 0;
			char* maxValStr = 0;

			if(lower_bound == isl_bool_false && upper_bound == isl_bool_false){
				loopTile->loopDimInfo[analyzeDim].isNoLowerBound = isl_bool_true;
				loopTile->loopDimInfo[analyzeDim].isNoUpperBound = isl_bool_true;
				minValStr = malloc(2*sizeof(char));
				maxValStr = malloc(2*sizeof(char));

				sprintf(minValStr,"0");
				sprintf(maxValStr,"0");
			}
			else if(lower_bound == isl_bool_false){
				char buff[1024];
				maxValStr = get_pw_aff_value_to_str(isl_set_dim_max(isl_set_copy(delta_set), analyzeDim));
				loopTile->loopDimInfo[analyzeDim].isNoLowerBound = isl_bool_true;

				sprintf(buff,"{ [%s] : %s < 0 and %s < %s }", dimNamesList, dimName, dimName, maxValStr);

				minValStr = get_pw_aff_value_to_str(isl_set_dim_max(isl_set_intersect(isl_set_copy(delta_set), isl_set_read_from_str(loopTile->loopScop->ctx, buff)), analyzeDim));
			}
			else if(upper_bound == isl_bool_false){
				char buff[1024];
				minValStr = get_pw_aff_value_to_str(isl_set_dim_min(isl_set_copy(delta_set), analyzeDim));
				loopTile->loopDimInfo[analyzeDim].isNoUpperBound = isl_bool_true;

				sprintf(buff,"{ [%s] : 0 < %s and %s < %s }", dimNamesList, dimName, minValStr, dimName);

				maxValStr = get_pw_aff_value_to_str(isl_set_dim_min(isl_set_intersect(isl_set_copy(delta_set), isl_set_read_from_str(loopTile->loopScop->ctx, buff)), analyzeDim));
			}
			else{
				minValStr = get_pw_aff_value_to_str(isl_set_dim_min(isl_set_copy(delta_set), analyzeDim));
				maxValStr = get_pw_aff_value_to_str(isl_set_dim_max(isl_set_copy(delta_set), analyzeDim));
			}

			{
				char buff[1024];

				sprintf(buff,"{ [%s] : %s = %s or 0 = %s or %s = %s }", dimNamesList , minValStr, dimName, dimName, maxValStr, dimName );
				delta_set = isl_set_intersect(delta_set, isl_set_read_from_str(loopTile->loopScop->ctx, buff));
			}

			free(minValStr);
			free(maxValStr);
		}

		{
			isl_union_set *unionSet = isl_union_set_from_set(delta_set);
			analyze_dim_info analyzeDimInfo;

			analyzeDimInfo.analyzeDim = analyzeDim + 1;
			analyzeDimInfo.loopTile = loopTile;

			isl_union_set_foreach_set_in_set(unionSet, loop_tile_read_delta_set, &analyzeDimInfo);

			isl_union_set_free(unionSet);
		}
	}else{
		isl_debug_printf_set("\n#distance vector set : %s\n", delta_set);

		if(loopTile->deltaSetToScan)
			loopTile->deltaSetToScan = isl_union_set_union(loopTile->deltaSetToScan, isl_union_set_from_set(delta_set));
		else
			loopTile->deltaSetToScan = isl_union_set_from_set(delta_set);
	}

	return isl_stat_ok;
}

static void loop_tile_create_tile_equations(__isl_keep loop_tile_info* loopTile){
	isl_debug_printf("\n%s\n", "#create tile equations start");

	sprintf(loopTile->spaceInfoEquations,"{[d0, b0, c0");
	for(int i=0;i<loopTile->loopDimSize;i++){
		sprintf(&(loopTile->spaceInfoEquations[strlen(loopTile->spaceInfoEquations)]), ", a%i", i + 1);
	}

	sprintf(&(loopTile->spaceInfoEquations[strlen(loopTile->spaceInfoEquations)]), "] : exists b1");
	sprintf(&(loopTile->spaceInfoEquations[strlen(loopTile->spaceInfoEquations)]), ", c1");
	for(int i=1;i<loopTile->loopDimSize;i++){
		sprintf(&(loopTile->spaceInfoEquations[strlen(loopTile->spaceInfoEquations)]), ", b%i", i + 1);
		sprintf(&(loopTile->spaceInfoEquations[strlen(loopTile->spaceInfoEquations)]), ", c%i", i + 1);
	}

	sprintf(&(loopTile->spaceInfoEquations[strlen(loopTile->spaceInfoEquations)]), " : ((b1 = a1 and a1 >= 0) or (b1 = -a1 and a1 < 0))");
	sprintf(&(loopTile->spaceInfoEquations[strlen(loopTile->spaceInfoEquations)]), " and ((c1 = -1 and a1 >= 0) or (c1 = 0 and a1 < 0))");
	for(int i=1;i<loopTile->loopDimSize;i++){
		sprintf(&(loopTile->spaceInfoEquations[strlen(loopTile->spaceInfoEquations)]), " and ((b%i = a%i and a%i >= 0) or (b%i = -a%i and a%i < 0))", i + 1, i + 1, i + 1, i + 1, i + 1, i + 1);
		sprintf(&(loopTile->spaceInfoEquations[strlen(loopTile->spaceInfoEquations)]), " and ((c%i = -1 and a%i >= 0) or (c%i = 0 and a%i < 0))", i + 1, i + 1, i + 1, i + 1);
	}

	sprintf(&(loopTile->spaceInfoEquations[strlen(loopTile->spaceInfoEquations)]), " and b0 > 0 and b0 = b1");
	for(int i=1;i<loopTile->loopDimSize;i++){
		sprintf(&(loopTile->spaceInfoEquations[strlen(loopTile->spaceInfoEquations)]), " + b%i", i + 1);
	}

	sprintf(&(loopTile->spaceInfoEquations[strlen(loopTile->spaceInfoEquations)]), " and c0 = c1");
	for(int i=1;i<loopTile->loopDimSize;i++){
		sprintf(&(loopTile->spaceInfoEquations[strlen(loopTile->spaceInfoEquations)]), " + c%i", i + 1);
	}

	for(int i=0; i < loopTile->loopDimSize; i++){
		if(loopTile->loopDimInfo[i].isNoLowerBound == isl_bool_true){
			sprintf(&(loopTile->spaceInfoEquations[strlen(loopTile->spaceInfoEquations)]), " and a%i <= 0 ", i+ 1 );
		}
		if(loopTile->loopDimInfo[i].isNoUpperBound == isl_bool_true){
			sprintf(&(loopTile->spaceInfoEquations[strlen(loopTile->spaceInfoEquations)]), " and a%i >= 0 ", i+ 1 );
		}

		if(loopTile->daptParams->dapt_no_order_dims == isl_bool_true ){
			if(loopTile->loopDimInfo[i].isOrder == isl_bool_true || i < loopTile->hyperplaneStartsFrom){
				sprintf(&(loopTile->spaceInfoEquations[strlen(loopTile->spaceInfoEquations)]), " and a%i = 0 ", i+ 1 );
			}
		}
		else{
			if(i < loopTile->hyperplaneStartsFrom){
				sprintf(&(loopTile->spaceInfoEquations[strlen(loopTile->spaceInfoEquations)]), " and a%i = 0 ", i+ 1 );
			}
		}
	}

	sprintf(&(loopTile->spaceInfoEquations[strlen(loopTile->spaceInfoEquations)]), " and ( 1 = 0 ");
	for(int i=0; i < loopTile->loopDimSize; i++){
		if(!(loopTile->loopDimInfo[i].isOrder == isl_bool_true || i < loopTile->hyperplaneStartsFrom)){
			sprintf(&(loopTile->spaceInfoEquations[strlen(loopTile->spaceInfoEquations)]), " or a%i != 0 ", i+ 1 );
		}
	}
	sprintf(&(loopTile->spaceInfoEquations[strlen(loopTile->spaceInfoEquations)]), " ) ");


	sprintf(loopTile->spaceInfoParallelFreeEquations,"%s", loopTile->spaceInfoEquations);

	isl_union_set_foreach_point(loopTile->deltaSetToScan, loop_tile_create_tile_equations_point, loopTile);

	sprintf(&(loopTile->spaceInfoEquations[strlen(loopTile->spaceInfoEquations)]), " }");
	sprintf(&(loopTile->spaceInfoParallelFreeEquations[strlen(loopTile->spaceInfoParallelFreeEquations)]), " }");

	isl_debug_printf("\n#tile equations: %s\n", loopTile->spaceInfoEquations);
	isl_debug_printf("\n#parallel free equation: %s\n", loopTile->spaceInfoParallelFreeEquations);

	isl_debug_printf("\n%s\n", "#create tile equations stop");
}

static isl_stat loop_tile_create_tile_equations_point(__isl_take isl_point *pnt, void *user){
	loop_tile_info* loopTile = (loop_tile_info*) user;
	isl_set* set = isl_set_from_point(pnt);

	int dim0Value = get_pw_aff_value_to_int( isl_set_dim_max(isl_set_copy(set), 0 ));

	sprintf(&(loopTile->spaceInfoEquations[strlen(loopTile->spaceInfoEquations)]), " and %i*a1", dim0Value);
	sprintf(&(loopTile->spaceInfoParallelFreeEquations[strlen(loopTile->spaceInfoParallelFreeEquations)]), " and %i*a1", dim0Value);

	for(int i= 1; i < loopTile->loopDimSize; i++){
		int dimIValue = get_pw_aff_value_to_int( isl_set_dim_max(isl_set_copy(set), i));
		sprintf(&(loopTile->spaceInfoEquations[strlen(loopTile->spaceInfoEquations)]), " + %i*a%i", dimIValue, i+1);
		sprintf(&(loopTile->spaceInfoParallelFreeEquations[strlen(loopTile->spaceInfoParallelFreeEquations)]), " + %i*a%i", dimIValue, i+1);
	}

	sprintf(&(loopTile->spaceInfoEquations[strlen(loopTile->spaceInfoEquations)]), " >= 0");
	sprintf(&(loopTile->spaceInfoParallelFreeEquations[strlen(loopTile->spaceInfoParallelFreeEquations)]), " = 0");

	isl_set_free(set);

	return isl_stat_ok;
}

static void loop_tile_create_hyperplanes(__isl_keep loop_tile_info* loopTile){
	isl_debug_printf("\n%s\n", "#create hyperplanes start");

	if(loopTile->daptParams->method == 1){

		for(int i=loopTile->hyperplaneStartsFrom;i<loopTile->loopDimSize;i++){
			if(loopTile->loopDimInfo[i].isOrder == isl_bool_false)
				if(loop_tile_create_parallel_free_hyperplane(loopTile, i) == isl_bool_true) break;
		}

		for(int i=loopTile->hyperplaneStartsFrom;i<loopTile->loopDimSize;i++){
			if(loopTile->loopDimInfo[i].isOrder == isl_bool_false)
				loop_tile_create_hyperplane_for_dim(loopTile, i);
		}
	}
	if(loopTile->daptParams->method == 2){
		for(int i=loopTile->hyperplaneStartsFrom;i<loopTile->loopDimSize;i++){
			if(loopTile->loopDimInfo[i].isOrder == isl_bool_false)
				loop_tile_create_hyperplane_for_dim(loopTile, i);
		}
	}
	if(loopTile->daptParams->method == 3){
		for(int i=loopTile->hyperplaneStartsFrom;i<loopTile->loopDimSize;i++){
			if(loopTile->loopDimInfo[i].isOrder == isl_bool_false)
				if(loop_tile_create_diamond_hyperplanes_for_dim(loopTile, i)==isl_bool_true) break;
		}
		for(int i=loopTile->hyperplaneStartsFrom;i<loopTile->loopDimSize;i++){
			if(loopTile->loopDimInfo[i].isOrder == isl_bool_false)
				loop_tile_create_hyperplane_for_dim(loopTile, i);
		}
	}
	if(loopTile->daptParams->method == 4){

		loop_tile_create_parallel_free_hyperplane_test(loopTile);

		for(int i=loopTile->hyperplaneStartsFrom;i<loopTile->loopDimSize;i++){
			if(loopTile->loopDimInfo[i].isOrder == isl_bool_false)
				loop_tile_create_hyperplane_for_dim(loopTile, i);
		}
	}

	isl_debug_printf("\n%s\n", "#create hyperplanes stop");
}

static isl_bool loop_tile_create_diamond_hyperplanes_for_dim(__isl_keep loop_tile_info* loopTile, int dim){
	char equationStr[4096];
	isl_set* ortogonalVector1 = 0;
	isl_set* ortogonalVector2 = 0;

	{
		sprintf(equationStr,"{[d0 = 0, b0, c0");
		for(int i=0;i<loopTile->loopDimSize;i++){
			sprintf(&equationStr[strlen(equationStr)], ", a%i", i + 1);
		}

		sprintf(&equationStr[strlen(equationStr)], "] : a%i > 0 and ( false", dim + 1);
		for(int i=0;i<loopTile->loopDimSize;i++){
			if(i!=dim) sprintf(&equationStr[strlen(equationStr)], " or a%i > 0", i + 1);
		}
		sprintf(&equationStr[strlen(equationStr)], " ) }");
		ortogonalVector1 = isl_set_lexmin(isl_set_intersect(isl_set_read_from_str(loopTile->loopScop->ctx, loopTile->spaceInfoEquations), isl_set_read_from_str(loopTile->loopScop->ctx, equationStr)));

		isl_debug_printf("\n#hyperplane_for_dim: lexmin( %s * ", equationStr);
		isl_debug_printf("%s )\n", loopTile->spaceInfoEquations);

		isl_debug_printf_set("\n#result: %s\n", ortogonalVector1);

		ortogonalVector1 = isl_set_remove_dims(ortogonalVector1, isl_dim_set, 0, 3);

		isl_debug_printf_set("\n#hyperplane: %s\n", ortogonalVector1);
	}
	{
		sprintf(equationStr,"{[d0 = 0, b0, c0");
		for(int i=0;i<loopTile->loopDimSize;i++){
			sprintf(&equationStr[strlen(equationStr)], ", a%i", i + 1);
		}

		sprintf(&equationStr[strlen(equationStr)], "] : a%i < 0 and ( false", dim + 1);
		for(int i=0;i<loopTile->loopDimSize;i++){
			if(i!=dim) sprintf(&equationStr[strlen(equationStr)], " or a%i > 0", i + 1);
		}
		sprintf(&equationStr[strlen(equationStr)], " ) }");
		ortogonalVector2 = isl_set_lexmin(isl_set_intersect(isl_set_read_from_str(loopTile->loopScop->ctx, loopTile->spaceInfoEquations), isl_set_read_from_str(loopTile->loopScop->ctx, equationStr)));

		isl_debug_printf("\n#hyperplane_for_dim: lexmin( %s * ", equationStr);
		isl_debug_printf("%s )\n", loopTile->spaceInfoEquations);

		isl_debug_printf_set("\n#result: %s\n", ortogonalVector2);

		ortogonalVector2 = isl_set_remove_dims(ortogonalVector2, isl_dim_set, 0, 3);

		isl_debug_printf_set("\n#hyperplane: %s\n", ortogonalVector2);
	}

	if(isl_set_is_empty(ortogonalVector1)==isl_bool_true || isl_set_is_empty(ortogonalVector2)==isl_bool_true){
		isl_set_free(ortogonalVector1);
		isl_set_free(ortogonalVector2);
	}else{
		isl_debug_printf("\n#%s\n", "diamond hyperplanes was found");

		isl_debug_printf_set("\n#hyperplane: %s\n", ortogonalVector1);
		if(loop_tile_check_hyperplane(loopTile, ortogonalVector1) == isl_bool_true){
			if(!loopTile->spaceInfoHyperplanesList){
				loopTile->spaceInfoHyperplanesList = isl_set_list_alloc(loopTile->loopScop->ctx, loopTile->loopDimSize);
			}

			loopTile->spaceInfoHyperplanesList = isl_set_list_add(loopTile->spaceInfoHyperplanesList, ortogonalVector1);
		}
		isl_debug_printf_set("\n#hyperplane: %s\n", ortogonalVector2);
		if(loop_tile_check_hyperplane(loopTile, ortogonalVector2) == isl_bool_true){
			if(!loopTile->spaceInfoHyperplanesList){
				loopTile->spaceInfoHyperplanesList = isl_set_list_alloc(loopTile->loopScop->ctx, loopTile->loopDimSize);
			}

			loopTile->spaceInfoHyperplanesList = isl_set_list_add(loopTile->spaceInfoHyperplanesList, ortogonalVector2);
		}

		return isl_bool_true;
	}

	return isl_bool_false;
}

static isl_bool loop_tile_create_parallel_free_hyperplane(__isl_keep loop_tile_info* loopTile, int dim){
	char equationStr[4096];

	sprintf(equationStr,"{[d0 = 0, b0, c0");
	for(int i=0;i<loopTile->loopDimSize;i++){
		sprintf(&equationStr[strlen(equationStr)], ", a%i", i + 1);
	}

	sprintf(&equationStr[strlen(equationStr)], "] : a%i != 0 }", dim + 1);

	{
		isl_set* ortogonalVector = isl_set_lexmin(isl_set_intersect(isl_set_read_from_str(loopTile->loopScop->ctx, loopTile->spaceInfoParallelFreeEquations), isl_set_read_from_str(loopTile->loopScop->ctx, equationStr)));

		isl_debug_printf("\n#hyperplane for dim parallel free: lexmin( %s * ", equationStr);
		isl_debug_printf("%s )\n", loopTile->spaceInfoParallelFreeEquations);

		isl_debug_printf_set("\n#result: %s\n", ortogonalVector);

		ortogonalVector = isl_set_remove_dims(ortogonalVector, isl_dim_set, 0, 3);

		isl_debug_printf_set("\n#hyperplane: %s\n", ortogonalVector);

		if(loop_tile_check_hyperplane(loopTile, ortogonalVector) == isl_bool_true){
			if(!loopTile->spaceInfoHyperplanesList){
				loopTile->spaceInfoHyperplanesList = isl_set_list_alloc(loopTile->loopScop->ctx, loopTile->loopDimSize);
			}

			loopTile->spaceInfoHyperplanesList = isl_set_list_add(loopTile->spaceInfoHyperplanesList, ortogonalVector);

			loopTile->parallelFreeExists = isl_bool_true;

			return isl_bool_true;
		}
		else
			isl_set_free(ortogonalVector);
	}

	return isl_bool_false;
}

static void loop_tile_create_parallel_free_hyperplane_test(__isl_keep loop_tile_info* loopTile){
	char equationStr[4096];

	sprintf(equationStr,"{[d0 = 0, b0, c0");
	for(int i=0;i<loopTile->loopDimSize;i++){
		sprintf(&equationStr[strlen(equationStr)], ", a%i", i + 1);
	}

	sprintf(&equationStr[strlen(equationStr)], "] : ( false");

	for(int i=0;i<loopTile->loopDimSize;i++){
		sprintf(&equationStr[strlen(equationStr)], " or a%i !=0", i + 1);
	}

	sprintf(&equationStr[strlen(equationStr)], " ) }");

	{
		isl_set* ortogonalVector = isl_set_lexmin(isl_set_intersect(isl_set_read_from_str(loopTile->loopScop->ctx, loopTile->spaceInfoParallelFreeEquations), isl_set_read_from_str(loopTile->loopScop->ctx, equationStr)));

		isl_debug_printf("\n#hyperplane for parallel free: lexmin( %s * ", equationStr);
		isl_debug_printf("%s )\n", loopTile->spaceInfoParallelFreeEquations);

		isl_debug_printf_set("\n#result: %s\n", ortogonalVector);

		ortogonalVector = isl_set_remove_dims(ortogonalVector, isl_dim_set, 0, 3);

		isl_debug_printf_set("\n#hyperplane: %s\n", ortogonalVector);

		if(loop_tile_check_hyperplane(loopTile, ortogonalVector) == isl_bool_true){
			if(!loopTile->spaceInfoHyperplanesList){
				loopTile->spaceInfoHyperplanesList = isl_set_list_alloc(loopTile->loopScop->ctx, loopTile->loopDimSize);
			}

			loopTile->spaceInfoHyperplanesList = isl_set_list_add(loopTile->spaceInfoHyperplanesList, ortogonalVector);

			loopTile->parallelFreeExists = isl_bool_true;
		}
		else
			isl_set_free(ortogonalVector);
	}
}

static void loop_tile_create_hyperplane_for_dim(__isl_keep loop_tile_info* loopTile, int dim){
	char equationStr[4096];

	sprintf(equationStr,"{[d0 = 0, b0, c0");
	for(int i=0;i<loopTile->loopDimSize;i++){
		sprintf(&equationStr[strlen(equationStr)], ", a%i", i + 1);
	}

	sprintf(&equationStr[strlen(equationStr)], "] : a%i != 0", dim + 1);

	if(loopTile->daptParams->dapt_unit_spacee_only == isl_bool_true){
		for(int i=0;i<dim;i++){
			sprintf(&equationStr[strlen(equationStr)], " and  a%i = 0", i + 1);
		}
		for(int i=dim+1;i<loopTile->loopDimSize;i++){
			sprintf(&equationStr[strlen(equationStr)], " and  a%i = 0", i + 1);
		}
	}

	sprintf(&equationStr[strlen(equationStr)], " }");

	{
		isl_set* ortogonalVector = isl_set_lexmin(isl_set_intersect(isl_set_read_from_str(loopTile->loopScop->ctx, loopTile->spaceInfoEquations), isl_set_read_from_str(loopTile->loopScop->ctx, equationStr)));

		isl_debug_printf("\n#hyperplane for dim: lexmin( %s * ", equationStr);
		isl_debug_printf("%s )\n", loopTile->spaceInfoEquations);

		isl_debug_printf_set("\n#result: %s\n", ortogonalVector);

		ortogonalVector = isl_set_remove_dims(ortogonalVector, isl_dim_set, 0, 3);

		isl_debug_printf_set("\n#hyperplane: %s\n", ortogonalVector);

		if(loop_tile_check_hyperplane(loopTile, ortogonalVector) == isl_bool_true){
			if(!loopTile->spaceInfoHyperplanesList){
				loopTile->spaceInfoHyperplanesList = isl_set_list_alloc(loopTile->loopScop->ctx, loopTile->loopDimSize);
			}

			loopTile->spaceInfoHyperplanesList = isl_set_list_add(loopTile->spaceInfoHyperplanesList, ortogonalVector);
		}
		else
			isl_set_free(ortogonalVector);
	}
}

static isl_bool loop_tile_check_hyperplane(__isl_keep loop_tile_info* loopTile, __isl_keep isl_set *hyperplane){
	isl_bool result = isl_bool_false;

	if(isl_set_is_empty(hyperplane) == isl_bool_true){
		isl_debug_printf("\n#hyperplane check: %s \n", "hyperplane is empty");
	}else{
		if(!loopTile->hyperplaneCheckInfo){
			loop_tile_add_hyperplane(loopTile, hyperplane);
			result = isl_bool_true;

			isl_debug_printf("\n#hyperplane check: %s \n", "first hyperplane");
		}
		else{
			char equationStr[4096];

			equationStr[0]='\0';

			sprintf(&equationStr[strlen(equationStr)], "{ : exists %s, c%i ", loopTile->hyperplaneCheckInfo->paramIdList, loopTile->hyperplaneCheckInfo->lastId + 1);
			sprintf(&equationStr[strlen(equationStr)], ": ( %s or c%i!=0 ) ", loopTile->hyperplaneCheckInfo->paramIdCondition, loopTile->hyperplaneCheckInfo->lastId + 1);
			for(int i=0;i<loopTile->loopDimSize;i++){
				sprintf(&equationStr[strlen(equationStr)], "and c%i*%i = %s ", loopTile->hyperplaneCheckInfo->lastId + 1, get_pw_aff_value_to_int( isl_set_dim_max(isl_set_copy(hyperplane), i)), loopTile->hyperplaneCheckInfo->statementList[i]);
			}
			sprintf(&equationStr[strlen(equationStr)], "}");

			isl_set* testSet = isl_set_read_from_str(loopTile->loopScop->ctx, equationStr);

			if(isl_set_is_empty(testSet) == isl_bool_true){
				loop_tile_add_hyperplane(loopTile, hyperplane);
				result = isl_bool_true;
			}

			isl_debug_printf("\n#hyperplane check: %s = {}\n", equationStr);

			isl_set_free(testSet);
		}
	}

	if( result == isl_bool_true){
		isl_debug_printf("\n#result: %s\n", "true");
	}
	else{
		isl_debug_printf("\n#result: %s\n", "false");
	}

	return result;
}

static void loop_tile_add_hyperplane(__isl_keep loop_tile_info* loopTile, __isl_keep isl_set *hyperplane){
	if(!loopTile->hyperplaneCheckInfo){
		loopTile->hyperplaneCheckInfo = malloc(sizeof(hyperplane_check_info));
		loopTile->hyperplaneCheckInfo->lastId = 0;
		loopTile->hyperplaneCheckInfo->paramIdList[0] = '\0';
		loopTile->hyperplaneCheckInfo->paramIdCondition[0] = '\0';
		loopTile->hyperplaneCheckInfo->statementList = (char**)malloc(sizeof(char*)*loopTile->loopDimSize);
		for(int i=0; i<loopTile->loopDimSize; i++){
			loopTile->hyperplaneCheckInfo->statementList[i] = (char*)malloc(sizeof(char)*256);
			loopTile->hyperplaneCheckInfo->statementList[i][0] = '\0';
		}
	}
	loopTile->hyperplaneCheckInfo->lastId += 1;

	if(loopTile->hyperplaneCheckInfo->lastId==1){
		sprintf(&loopTile->hyperplaneCheckInfo->paramIdList[strlen(loopTile->hyperplaneCheckInfo->paramIdList)],"c%i", loopTile->hyperplaneCheckInfo->lastId);
		sprintf(&loopTile->hyperplaneCheckInfo->paramIdCondition[strlen(loopTile->hyperplaneCheckInfo->paramIdCondition)],"c%i!=0", loopTile->hyperplaneCheckInfo->lastId);
		for(int i=0; i<loopTile->loopDimSize; i++){
			sprintf(&loopTile->hyperplaneCheckInfo->statementList[i][strlen(loopTile->hyperplaneCheckInfo->statementList[i])],"c%i*%i", loopTile->hyperplaneCheckInfo->lastId, get_pw_aff_value_to_int( isl_set_dim_max(isl_set_copy(hyperplane), i)));
		}
	}
	else{
		sprintf(&loopTile->hyperplaneCheckInfo->paramIdList[strlen(loopTile->hyperplaneCheckInfo->paramIdList)],", c%i", loopTile->hyperplaneCheckInfo->lastId);
		sprintf(&loopTile->hyperplaneCheckInfo->paramIdCondition[strlen(loopTile->hyperplaneCheckInfo->paramIdCondition)]," or c%i!=0", loopTile->hyperplaneCheckInfo->lastId);
		for(int i=0; i<loopTile->loopDimSize; i++){
			sprintf(&loopTile->hyperplaneCheckInfo->statementList[i][strlen(loopTile->hyperplaneCheckInfo->statementList[i])]," + c%i*%i", loopTile->hyperplaneCheckInfo->lastId, get_pw_aff_value_to_int( isl_set_dim_max(isl_set_copy(hyperplane), i)));
		}
	}
}

static void loop_tile_create_spaces(__isl_keep loop_tile_info* loopTile){
	id_name *spacesNames;
	isl_debug_printf("\n%s\n", "#create spaces start");

	isl_set_list_foreach(loopTile->spaceInfoHyperplanesList, loop_tile_space_from_hyperplane_set, loopTile);


	spacesNames = loop_scop_get_id_names(loopTile->loopScop, "h", loopTile->hyperplaneDimSize);

	for(int i=0; i<loopTile->hyperplaneDimSize; i++){
		sprintf(loopTile->spaceInfo[i].spaceName, "%s", spacesNames[i]);
	}

	free(spacesNames);

	for(int i=0; i<loopTile->hyperplaneDimSize; i++){
		loop_tile_space_to_space_mapper(loopTile,i);
	}

	isl_debug_printf_map("\n#spaces mapper: %s\n", loopTile->spaceInfoMapper);

	isl_debug_printf("\n%s\n", "#create spaces stop");
}


static isl_stat loop_tile_space_from_hyperplane_set(__isl_take isl_set *set, void *user){
	isl_set_foreach_point(set, loop_tile_space_from_hyperplanes, user);

	isl_set_free(set);

	return isl_stat_ok;
}
static isl_stat loop_tile_space_from_hyperplanes(__isl_take isl_point *pnt, void *user){
	loop_tile_info* loopTile = (loop_tile_info*) user;
	isl_set* set = isl_set_from_point(pnt);
	int currentSpace = loopTile->hyperplaneDimSize;

	loopTile->hyperplaneDimSize += 1;
	loopTile->spaceInfo = realloc(loopTile->spaceInfo, loopTile->hyperplaneDimSize * sizeof(space_Info));
	loopTile->spaceInfo[currentSpace].vector = malloc(loopTile->loopDimSize * sizeof(int));

	for(int i=0; i<loopTile->loopDimSize; i++){
		loopTile->spaceInfo[currentSpace].vector[i] = get_pw_aff_value_to_int( isl_set_dim_max(isl_set_copy(set), i));
	}

	isl_set_free(set);

	return isl_stat_ok;
}

static void loop_tile_space_to_space_mapper(__isl_keep loop_tile_info* loopTile, int spaceInfoPos){
	char equationStr[4096];
	char outDimsStr[256];
	space_Info* spaceInfo = &(loopTile->spaceInfo[spaceInfoPos]);
	int* spaceInfoVector = spaceInfo->vector;
	int spaceSize = loopTile->daptParams->size;

	if(spaceInfoPos < loopTile->daptParams->sizesDim){
		spaceSize = loopTile->daptParams->sizes[spaceInfoPos];
	}

	equationStr[0] = '\0';

	if(strlen(loopTile->domainParams)>0){
		sprintf(&equationStr[strlen(equationStr)],"[%s] -> { ", loopTile->domainParams);
	}
	sprintf(&equationStr[strlen(equationStr)],"[%s", loopTile->loopDimInfo[0].dimName);
	for(int i=1; i<loopTile->loopDimSize; i++){
		sprintf(&equationStr[strlen(equationStr)],", %s", loopTile->loopDimInfo[i].dimName);
	}

	outDimsStr[0] = '\0';
	for(int i=0; i<loopTile->hyperplaneStartsFrom; i++){
		sprintf(&outDimsStr[strlen(outDimsStr)],", i%s = %s", loopTile->loopDimInfo[i].dimName, loopTile->loopDimInfo[i].dimName);
	}
	for(int i=0; i<loopTile->hyperplaneDimSize; i++){
		sprintf(&outDimsStr[strlen(outDimsStr)],", %s", loopTile->spaceInfo[i].spaceName);
	}
	for(int i=loopTile->hyperplaneStartsFrom; i<loopTile->loopDimSize; i++){
		sprintf(&outDimsStr[strlen(outDimsStr)],", i%s = %s", loopTile->loopDimInfo[i].dimName, loopTile->loopDimInfo[i].dimName);
	}
	outDimsStr[0] = ' ';

	sprintf(&equationStr[strlen(equationStr)],"] -> [%s", outDimsStr);

	sprintf(&equationStr[strlen(equationStr)],"] : exists b%s : %i * %s", spaceInfo->spaceName, spaceInfoVector[0], loopTile->loopDimInfo[0].dimName);
	for(int i=1; i<loopTile->loopDimSize; i++){
		sprintf(&equationStr[strlen(equationStr)]," + %i * %s", spaceInfoVector[i], loopTile->loopDimInfo[i].dimName);
	}
	sprintf(&equationStr[strlen(equationStr)]," - b%s = 0 and ", spaceInfo->spaceName);

	sprintf(&equationStr[strlen(equationStr)],"%i * %s", spaceSize, spaceInfo->spaceName);
	sprintf(&equationStr[strlen(equationStr)]," <= b%s <= ", spaceInfo->spaceName);
	sprintf(&equationStr[strlen(equationStr)],"%i * (%s + 1) - 1 }", spaceSize, spaceInfo->spaceName);

	{
		isl_map* mapper;
		mapper = isl_map_read_from_str(loopTile->loopScop->ctx, equationStr);

		isl_debug_printf("\n#part: %s \n", equationStr );

		if(!loopTile->spaceInfoMapper){
			loopTile->spaceInfoMapper = isl_map_copy(mapper);
		}

		loopTile->spaceInfoMapper = isl_map_coalesce(isl_map_intersect(loopTile->spaceInfoMapper, mapper));
	}
}

static void loop_tile_calculate_tile_scop(__isl_keep loop_tile_info* loopTile){
	isl_debug_printf("\n%s\n", "#create tile scop start");

	isl_union_set_foreach_set_in_set(loopTile->loopScop->loopDomain,loop_tile_create_tile_scop_mapper, loopTile);

	isl_debug_printf_union_map("\n#tile scop mapper: %s\n", loopTile->tileMapper);

	{
		schedule_mapper_info mapperInfo;
		loop_scop *tileScop = 0;

		mapperInfo.scheduleMap = loopTile->loopScop->loopMap;
		mapperInfo.scheduleMapper = loopTile->tileMapper;

		tileScop = loop_scop_from_schedule_mapper_info(loopTile->loopScop, &mapperInfo);

		//loop_scop_norm_debug_printf(tileScop);

		isl_debug_printf_union_set("\n#tile scop delta: %s\n", tileScop->delta );

		isl_union_set_foreach_set_in_set(tileScop->delta, loop_tile_calculate_tile_delta, loopTile);

		loop_scop_loop_scop_free(tileScop);
	}

	{
		char equationStr[4096];

		equationStr[0] = '\0';
		sprintf(&equationStr[strlen(equationStr)],"{ [%s", loopTile->spaceInfo[0].spaceName);
		for(int i=1; i<loopTile->hyperplaneDimSize; i++){
			sprintf(&equationStr[strlen(equationStr)],", %s", loopTile->spaceInfo[i].spaceName);
		}
		sprintf(&equationStr[strlen(equationStr)],"] : not (%s = 0", loopTile->spaceInfo[0].spaceName);
		for(int i=1; i<loopTile->hyperplaneDimSize; i++){
			sprintf(&equationStr[strlen(equationStr)]," and %s = 0", loopTile->spaceInfo[i].spaceName);
		}
		sprintf(&equationStr[strlen(equationStr)],") }");

		loopTile->tileDelta = isl_union_set_intersect(loopTile->tileDelta, isl_union_set_read_from_str(loopTile->loopScop->ctx, equationStr));
	}

	isl_debug_printf_union_set("\n#tile delta: %s\n", loopTile->tileDelta );

	isl_debug_printf("\n%s\n", "#create tile scop stop");
}

static isl_stat loop_tile_create_tile_scop_mapper(__isl_take isl_set *set, void *user){
	loop_tile_info* loopTile = (loop_tile_info*)user;
	const char *tupleName = isl_set_get_tuple_name(set);
	isl_map *mapper = isl_map_copy(loopTile->spaceInfoMapper);

	mapper = isl_map_set_tuple_name(mapper, isl_dim_in, tupleName);
	mapper = isl_map_set_tuple_name(mapper, isl_dim_out, tupleName);

	if(!loopTile->tileMapper){
		loopTile->tileMapper = isl_union_map_from_map(isl_map_copy(mapper));
	}

	loopTile->tileMapper = isl_union_map_coalesce(isl_union_map_union(loopTile->tileMapper, isl_union_map_from_map(mapper)));

	isl_set_free(set);

	return isl_stat_ok;
}

static isl_stat loop_tile_calculate_tile_delta(__isl_take isl_set *set, void *user){
	loop_tile_info* loopTile = (loop_tile_info*)user;

	int setDimSize = isl_set_dim(set, isl_dim_set);
	char equationStr[4096];
	char outDimsStr[256];
	char outDimsCondStr[256];

	outDimsStr[0] = '\0';
	outDimsCondStr[0] = '\0';
	for(int i=0; i<setDimSize ; i++){
		const char *dimName = isl_set_get_dim_name(set, isl_dim_set, i);
		sprintf(&outDimsStr[strlen(outDimsStr)],", %s", dimName);
		if(i<loopTile->hyperplaneStartsFrom){
				sprintf(&outDimsCondStr[strlen(outDimsCondStr)],"and %s=0", dimName);
		}
	}
	outDimsStr[0] = ' ';

	sprintf(equationStr,"{ [%s] : true %s }", outDimsStr, outDimsCondStr);

	isl_set *tileDelta = isl_set_intersect(set, isl_set_read_from_str(loopTile->loopScop->ctx, equationStr));

	isl_debug_printf_set("\n#tile scop delta part: %s\n", tileDelta );

	if(0<loopTile->hyperplaneStartsFrom){
		tileDelta = isl_set_remove_dims(tileDelta, isl_dim_set, 0, loopTile->hyperplaneStartsFrom);
	}

	tileDelta = isl_set_remove_dims(tileDelta, isl_dim_set, loopTile->hyperplaneDimSize, loopTile->loopDimSize - loopTile->hyperplaneStartsFrom);

	if(!loopTile->tileDelta){
		loopTile->tileDelta = isl_union_set_from_set(isl_set_copy(tileDelta));
	}

	loopTile->tileDelta = isl_union_set_coalesce(isl_union_set_union(loopTile->tileDelta, isl_union_set_from_set(tileDelta)));

	return isl_stat_ok;
}

static void loop_tile_read_tile_delta_info(__isl_keep loop_tile_info* loopTile){
	isl_debug_printf("\n%s\n", "#tile delta analyze start");

	if(isl_union_set_is_empty(loopTile->loopScop->delta)==isl_bool_true){
		loopTile->tileDeltaSetToScan = isl_union_set_read_from_str(loopTile->loopScop->ctx, "{}");
	}else{
		analyze_dim_info analyzeDimInfo;

		analyzeDimInfo.analyzeDim = 0;
		analyzeDimInfo.loopTile = loopTile;

		isl_union_set_foreach_set_in_set(loopTile->tileDelta, loop_tile_read_tile_delta_set, &analyzeDimInfo);
	}

	isl_debug_printf("\n%s\n", "#tile delta analyze stop");
}

static isl_stat loop_tile_read_tile_delta_set(__isl_take isl_set *set, void *user){
	analyze_dim_info *analyzeDimInfo = (analyze_dim_info*) user;
	loop_tile_info* loopTile = analyzeDimInfo->loopTile;
	int analyzeDim = analyzeDimInfo->analyzeDim;

	set = isl_set_remove_divs(set);

	if(analyzeDim < loopTile->hyperplaneDimSize){
		char dimNamesList[256];
		isl_bool lower_bound, upper_bound;
		const char* dimName = isl_set_get_dim_name(set, isl_dim_set, analyzeDim);

		dimNamesList[0]='\0';
		{
			int i;
			for(i=0; i < loopTile->hyperplaneDimSize; i++ ){
				sprintf(&dimNamesList[strlen(dimNamesList)], ",%s", isl_set_get_dim_name(set,isl_dim_set, i));
			}
		}
		dimNamesList[0] = ' ';

		{
			isl_set* tmpSet = isl_set_copy(set);

			if(analyzeDim > 0)
			{
				tmpSet = isl_set_remove_dims(tmpSet, isl_dim_set, 0, analyzeDim);
			}
			if(loopTile->hyperplaneDimSize - (analyzeDim + 1) > 0)
			{
				tmpSet = isl_set_remove_dims(tmpSet, isl_dim_set, 1, loopTile->hyperplaneDimSize - (analyzeDim + 1));
			}

			lower_bound = isl_set_dim_has_lower_bound(tmpSet, isl_dim_set, 0);
			upper_bound = isl_set_dim_has_upper_bound(tmpSet, isl_dim_set, 0);

			isl_set_free(tmpSet);
		}

		{
			char* minValStr = 0;
			char* maxValStr = 0;

			if(lower_bound == isl_bool_false && upper_bound == isl_bool_false){
				minValStr = malloc(2*sizeof(char));
				maxValStr = malloc(2*sizeof(char));

				sprintf(minValStr,"0");
				sprintf(maxValStr,"0");
			}
			else if(lower_bound == isl_bool_false){
				char buff[1024];
				maxValStr = get_pw_aff_value_to_str(isl_set_dim_max(isl_set_copy(set), analyzeDim));

				sprintf(buff,"{ [%s] : %s < 0 and %s < %s }", dimNamesList, dimName, dimName, maxValStr);

				minValStr = get_pw_aff_value_to_str(isl_set_dim_max(isl_set_intersect(isl_set_copy(set), isl_set_read_from_str(loopTile->loopScop->ctx, buff)), analyzeDim));
			}
			else if(upper_bound == isl_bool_false){
				char buff[1024];
				minValStr = get_pw_aff_value_to_str(isl_set_dim_min(isl_set_copy(set), analyzeDim));

				sprintf(buff,"{ [%s] : 0 < %s and %s < %s }", dimNamesList, dimName, minValStr, dimName);

				maxValStr = get_pw_aff_value_to_str(isl_set_dim_min(isl_set_intersect(isl_set_copy(set), isl_set_read_from_str(loopTile->loopScop->ctx, buff)), analyzeDim));
			}
			else{
				minValStr = get_pw_aff_value_to_str(isl_set_dim_min(isl_set_copy(set), analyzeDim));
				maxValStr = get_pw_aff_value_to_str(isl_set_dim_max(isl_set_copy(set), analyzeDim));
			}

			{
				char buff[1024];

				sprintf(buff,"{ [%s] : %s = %s or 0 = %s or %s = %s }", dimNamesList , minValStr, dimName, dimName, maxValStr, dimName );
				set = isl_set_intersect(set, isl_set_read_from_str(loopTile->loopScop->ctx, buff));
			}

			free(minValStr);
			free(maxValStr);
		}
		{
			analyze_dim_info analyzeDimInfo;

			analyzeDimInfo.analyzeDim = analyzeDim + 1;
			analyzeDimInfo.loopTile = loopTile;

			isl_union_set *unionSet = isl_union_set_from_set(set);
			isl_union_set_foreach_set_in_set(unionSet, loop_tile_read_tile_delta_set, &analyzeDimInfo);
			isl_union_set_free(unionSet);
		}
	}
	else{
		isl_debug_printf_set("\n#tile relatiions distance vector set: %s\n", set);

		if(loopTile->tileDeltaSetToScan)
			loopTile->tileDeltaSetToScan = isl_union_set_union(loopTile->tileDeltaSetToScan, isl_union_set_from_set(set));
		else
			loopTile->tileDeltaSetToScan = isl_union_set_from_set(set);
	}

	return isl_stat_ok;
}

static void loop_tile_create_wf_equations(__isl_keep loop_tile_info* loopTile){
	isl_debug_printf("\n%s\n", "#create wafefront equations start");

	sprintf(loopTile->tileInfoEquations,"{[d0, b0, c0");
	for(int i=0; i<loopTile->hyperplaneDimSize; i++){
		sprintf(&(loopTile->tileInfoEquations[strlen(loopTile->tileInfoEquations)]), ", a%i", i + 1);
	}

	sprintf(&(loopTile->tileInfoEquations[strlen(loopTile->tileInfoEquations)]), "] : exists b1");
	for(int i=1; i<loopTile->hyperplaneDimSize; i++){
		sprintf(&(loopTile->tileInfoEquations[strlen(loopTile->tileInfoEquations)]), ", b%i", i + 1);
	}

	sprintf(&(loopTile->tileInfoEquations[strlen(loopTile->tileInfoEquations)]), " : ((b1 = a1 and a1 >= 0) or (b1 = -a1 and a1 < 0))");
	for(int i=1; i<loopTile->hyperplaneDimSize; i++){
		sprintf(&(loopTile->tileInfoEquations[strlen(loopTile->tileInfoEquations)]), " and ((b%i = a%i and a%i >= 0) or (b%i = -a%i and a%i < 0))", i + 1, i + 1, i + 1, i + 1, i + 1, i + 1);
	}

	sprintf(&(loopTile->tileInfoEquations[strlen(loopTile->tileInfoEquations)]), " and b0 > 0 and b0 = b1");
	for(int i=1; i<loopTile->hyperplaneDimSize; i++){
		sprintf(&(loopTile->tileInfoEquations[strlen(loopTile->tileInfoEquations)]), " + b%i", i + 1);
	}

	isl_union_set_foreach_point(loopTile->tileDeltaSetToScan, loop_tile_create_wf_equations_point, loopTile);

	sprintf(&(loopTile->tileInfoEquations[strlen(loopTile->tileInfoEquations)]), " }");

	isl_debug_printf("\n#wafefront equations: %s\n", loopTile->tileInfoEquations);

	isl_debug_printf("\n%s\n", "#create wafefront equations stop");
}

static isl_stat loop_tile_create_wf_equations_point(__isl_take isl_point *pnt, void *user){
	loop_tile_info* loopTile = (loop_tile_info*) user;
	isl_set* set = isl_set_from_point(pnt);

	int dimIValue = get_pw_aff_value_to_int( isl_set_dim_max(isl_set_copy(set), 0 ));

	sprintf(&(loopTile->tileInfoEquations[strlen(loopTile->tileInfoEquations)]), " and %i*a1", dimIValue);

	for(int i=1; i<loopTile->hyperplaneDimSize; i++){
		int dimIValue = get_pw_aff_value_to_int( isl_set_dim_max(isl_set_copy(set), i));

		sprintf(&(loopTile->tileInfoEquations[strlen(loopTile->tileInfoEquations)]), " + %i*a%i", dimIValue, i+1);
	}

	sprintf(&(loopTile->tileInfoEquations[strlen(loopTile->tileInfoEquations)]), " > 0");

	isl_set_free(set);

	return isl_stat_ok;
}


static void loop_tile_calculate_wf_hyperplane(__isl_keep loop_tile_info* loopTile){
	char equationStr[4096];

	isl_debug_printf("\n%s\n", "#calculate wafefront hyperplane start");

	sprintf(equationStr,"{[d0 = 0, b0, c0");
	for(int i=0; i<loopTile->hyperplaneDimSize; i++){
		sprintf(&equationStr[strlen(equationStr)], ", a%i", i + 1);
	}

	sprintf(&equationStr[strlen(equationStr)], "] : exists c1");
	for(int i=1; i<loopTile->hyperplaneDimSize; i++){
		sprintf(&equationStr[strlen(equationStr)], ", c%i", i + 1);
	}

	sprintf(&equationStr[strlen(equationStr)], " : ((c1 = -1 and a1 >= 0) or (c1 = 0 and a1 < 0))");
	for(int i=1; i<loopTile->hyperplaneDimSize; i++){
		sprintf(&equationStr[strlen(equationStr)], " and ((c%i = -1 and a%i >= 0) or (c%i = 0 and a%i < 0))", i + 1, i + 1, i + 1, i + 1);
	}

	sprintf(&equationStr[strlen(equationStr)], " and c0 = c1");
	for(int i=1; i<loopTile->hyperplaneDimSize; i++){
		sprintf(&equationStr[strlen(equationStr)], " + c%i", i + 1);
	}

	for(int i=0; i<loopTile->hyperplaneDimSize; i++){
		sprintf(&equationStr[strlen(equationStr)], " and a%i != 0", i + 1);
	}

	sprintf(&equationStr[strlen(equationStr)], "}");

	isl_debug_printf("\n#wafefront hyperplane: lexmin( %s * ", equationStr);
	isl_debug_printf("%s )\n", loopTile->tileInfoEquations);

	loopTile->wafefrontHyperplane = isl_set_lexmin(isl_set_intersect(isl_set_read_from_str(loopTile->loopScop->ctx, loopTile->tileInfoEquations), isl_set_read_from_str(loopTile->loopScop->ctx, equationStr)));
	isl_debug_printf_set("\n#result: %s\n", loopTile->wafefrontHyperplane);
	loopTile->wafefrontHyperplane = isl_set_remove_dims(loopTile->wafefrontHyperplane, isl_dim_set, 0, 3 );
	isl_debug_printf_set("\n#wafefront hyperplane: %s\n", loopTile->wafefrontHyperplane);

	isl_debug_printf("\n%s\n", "#calculate wafefront hyperplane stop");
}

static void loop_tile_create_parallel_free_schedule(__isl_keep loop_tile_info* loopTile){
	isl_debug_printf("\n%s\n", "#create parallel free schedule start");

	isl_union_map_foreach_map_in_map(loopTile->loopScop->loopMap, loop_tile_parallel_free_schedule_for_map, loopTile);

	isl_debug_printf_union_map("\n#parallel free schedule: %s\n",loopTile->wafefrontScheduleMap);

	{
		id_name *idNames = 0;
		loopTile->iterators = isl_id_list_alloc(loopTile->loopScop->ctx,2+(loopTile->loopDimSize*2));

		idNames = loop_scop_get_id_names(loopTile->loopScop, "o", 1);
		loopTile->iterators = isl_id_list_add(loopTile->iterators, isl_id_alloc(loopTile->loopScop->ctx,idNames[0],0));
		free(idNames);

		for(int i=0; i<loopTile->hyperplaneStartsFrom; i++){
			loopTile->iterators = isl_id_list_add(loopTile->iterators, isl_id_alloc(loopTile->loopScop->ctx,loopTile->loopDimInfo[i].dimName,0));
		}

		idNames = loop_scop_get_id_names(loopTile->loopScop, "w", 1);
		loopTile->iterators = isl_id_list_add(loopTile->iterators, isl_id_alloc(loopTile->loopScop->ctx,idNames[0],0));
		free(idNames);

		idNames = loop_scop_get_id_names(loopTile->loopScop, "h", loopTile->loopDimSize);
		{
			char idName[8];
			sprintf(idName,"p%s",idNames[0]);
			loopTile->iterators = isl_id_list_add(loopTile->iterators, isl_id_alloc(loopTile->loopScop->ctx,idName,0));
			for(int i=1; i< loopTile->loopDimSize; i++){
				loopTile->iterators = isl_id_list_add(loopTile->iterators, isl_id_alloc(loopTile->loopScop->ctx,idNames[i],0));
			}
		}
		free(idNames);

		idNames = loop_scop_get_id_names(loopTile->loopScop, "t", 1);
		loopTile->iterators = isl_id_list_add(loopTile->iterators, isl_id_alloc(loopTile->loopScop->ctx,idNames[0],0));
		free(idNames);

		for(int i=loopTile->hyperplaneStartsFrom; i<loopTile->loopDimSize; i++){
			loopTile->iterators = isl_id_list_add(loopTile->iterators, isl_id_alloc(loopTile->loopScop->ctx,loopTile->loopDimInfo[i].dimName,0));
		}
	}

	isl_debug_printf("\n%s\n", "#create parallel free schedule stop");
}

static isl_stat loop_tile_parallel_free_schedule_for_map(__isl_take isl_map *map, void *user){
	loop_tile_info* loopTile = (loop_tile_info*)user;
	id_name *wfNames = loop_scop_get_id_names(loopTile->loopScop, "w", 1);
	const char *tupleName = isl_map_get_tuple_name(map, isl_dim_in);
	char *mapStr = isl_map_to_str(map);
	char *mapToStr = str_substring(mapStr,"] -> [", "]");
	isl_union_map *wafefrontScheduleMap = 0;

	wafefrontScheduleMap = loop_tile_space_to_parallel_free_schedule(wafefrontScheduleMap, loopTile, tupleName, mapToStr, wfNames[0]);

	if(!loopTile->wafefrontScheduleMap){
		loopTile->wafefrontScheduleMap = isl_union_map_copy(wafefrontScheduleMap);
	}
	loopTile->wafefrontScheduleMap = isl_union_map_coalesce(isl_union_map_union(loopTile->wafefrontScheduleMap, wafefrontScheduleMap));

	free(mapToStr);
	free(mapStr);
	free(wfNames);
	isl_map_free(map);

	return isl_stat_ok;
}

static __isl_give isl_union_map * loop_tile_space_to_parallel_free_schedule(__isl_take isl_union_map *wafefrontScheduleMap, __isl_keep loop_tile_info* loopTile, const char *tupleName, char *mapToStr, id_name wfName){
	char equationStr[65536];
	char tmpNamesStr[512];
	char tmpEquationStr[65536];

	equationStr[0] = '\0';

	if(strlen(loopTile->domainParams)>0){
		sprintf(&equationStr[strlen(equationStr)],"[%s] -> { ", loopTile->domainParams);
	}
	sprintf(&equationStr[strlen(equationStr)],"%s[%s", tupleName, loopTile->loopDimInfo[0].dimName);
	for(int i=1; i<loopTile->loopDimSize; i++){
		sprintf(&equationStr[strlen(equationStr)],", %s", loopTile->loopDimInfo[i].dimName);
	}
	sprintf(&equationStr[strlen(equationStr)],"] -> [%d", loopTile->scheduleIdx);

	for(int i=0; i<loopTile->hyperplaneStartsFrom; i++){
		sprintf(&equationStr[strlen(equationStr)],", %s", loopTile->loopDimInfo[i].dimName);
	}

	sprintf(&equationStr[strlen(equationStr)],", %s", wfName);

	for(int i=0; i<loopTile->hyperplaneDimSize; i++){
		sprintf(&equationStr[strlen(equationStr)],", %s", loopTile->spaceInfo[i].spaceName);
	}
	for(int i=loopTile->hyperplaneDimSize; i<loopTile->loopDimSize; i++){
		sprintf(&equationStr[strlen(equationStr)],", %s", "0");
	}
	sprintf(&equationStr[strlen(equationStr)],", t%s", wfName);
	for(int i=loopTile->hyperplaneStartsFrom; i<loopTile->loopDimSize; i++){
		sprintf(&equationStr[strlen(equationStr)],", %s", loopTile->loopDimInfo[i].dimName);
	}

	tmpNamesStr[0]='\0';
	tmpEquationStr[0]='\0';
	sprintf(&tmpNamesStr[strlen(tmpNamesStr)]," b%s", wfName);
	for(int spaceInfoPos=0; spaceInfoPos<loopTile->hyperplaneDimSize; spaceInfoPos++){
		space_Info* spaceInfo = &(loopTile->spaceInfo[spaceInfoPos]);
		int* spaceInfoVector = spaceInfo->vector;
		int spaceSize = loopTile->daptParams->size;

		if(spaceInfoPos < loopTile->daptParams->sizesDim){
			spaceSize = loopTile->daptParams->sizes[spaceInfoPos];
		}

		sprintf(&tmpNamesStr[strlen(tmpNamesStr)],", b%s", spaceInfo->spaceName);

		sprintf(&tmpEquationStr[strlen(tmpEquationStr)],"and %i * %s", spaceInfoVector[0], loopTile->loopDimInfo[0].dimName);
		for(int i=1; i<loopTile->loopDimSize; i++){
			sprintf(&tmpEquationStr[strlen(tmpEquationStr)]," + %i * %s", spaceInfoVector[i], loopTile->loopDimInfo[i].dimName);
		}
		sprintf(&tmpEquationStr[strlen(tmpEquationStr)]," - b%s = 0 and ", spaceInfo->spaceName);

		sprintf(&tmpEquationStr[strlen(tmpEquationStr)],"%i * %s", spaceSize, spaceInfo->spaceName);
		sprintf(&tmpEquationStr[strlen(tmpEquationStr)]," <= b%s <= ", spaceInfo->spaceName);
		sprintf(&tmpEquationStr[strlen(tmpEquationStr)],"%i * (%s + 1) - 1 ", spaceSize, spaceInfo->spaceName);
	}

	sprintf(&equationStr[strlen(equationStr)],"] : exists %s : 1 = 1 %s", tmpNamesStr, tmpEquationStr );

	if(loopTile->wafefrontHyperplane!=0){
		sprintf(&equationStr[strlen(equationStr)]," and b%s = %i * b%s", wfName, get_pw_aff_value_to_int(isl_set_dim_min(isl_set_copy(loopTile->wafefrontHyperplane), 0)), loopTile->spaceInfo[0].spaceName);
		for(int spaceInfoPos=1; spaceInfoPos<loopTile->hyperplaneDimSize; spaceInfoPos++){
			sprintf(&equationStr[strlen(equationStr)]," + %i * b%s", get_pw_aff_value_to_int(isl_set_dim_min(isl_set_copy(loopTile->wafefrontHyperplane), spaceInfoPos)), loopTile->spaceInfo[spaceInfoPos].spaceName);
		}

		if(loopTile->daptParams->sizeTimeTile > 0  && loopTile->hyperplaneDimSize >= 2){
			sprintf(&equationStr[strlen(equationStr)]," and %i * t%s <= b%s <= %i * (t%s + 1) - 1", loopTile->daptParams->sizeTimeTile, wfName, wfName, loopTile->daptParams->sizeTimeTile, wfName );
		} else{
			sprintf(&equationStr[strlen(equationStr)]," and t%s = 0", wfName);
		}
	}
	else{
		sprintf(&equationStr[strlen(equationStr)]," and t%s = 0", wfName);
	}

	sprintf(&equationStr[strlen(equationStr)]," and %s = 0 }", wfName);

	isl_debug_printf("\n#part: %s \n", equationStr );

	{
		isl_union_map* map;
		map = isl_union_map_read_from_str(loopTile->loopScop->ctx, equationStr);

		if(!wafefrontScheduleMap){
			wafefrontScheduleMap = isl_union_map_copy(map);
		}

		wafefrontScheduleMap = isl_union_map_coalesce(isl_union_map_intersect(wafefrontScheduleMap, map));
	}

	return wafefrontScheduleMap;
}

static void loop_tile_create_wafefront_schedule(__isl_keep loop_tile_info* loopTile){
	isl_debug_printf("\n%s\n", "#create wafefront schedule start");

	isl_union_map_foreach_map_in_map(loopTile->loopScop->loopMap, loop_tile_wafefront_schedule_for_map, loopTile);

	isl_debug_printf_union_map("\n#wafefront schedule: %s\n",loopTile->wafefrontScheduleMap);

	{
		id_name *idNames = 0;
		loopTile->iterators = isl_id_list_alloc(loopTile->loopScop->ctx,2+(loopTile->loopDimSize*2));

		idNames = loop_scop_get_id_names(loopTile->loopScop, "o", 1);
		loopTile->iterators = isl_id_list_add(loopTile->iterators, isl_id_alloc(loopTile->loopScop->ctx,idNames[0],0));
		free(idNames);

		for(int i=0; i<loopTile->hyperplaneStartsFrom; i++){
			loopTile->iterators = isl_id_list_add(loopTile->iterators, isl_id_alloc(loopTile->loopScop->ctx,loopTile->loopDimInfo[i].dimName,0));
		}

		idNames = loop_scop_get_id_names(loopTile->loopScop, "w", 1);
		loopTile->iterators = isl_id_list_add(loopTile->iterators, isl_id_alloc(loopTile->loopScop->ctx,idNames[0],0));
		free(idNames);

		idNames = loop_scop_get_id_names(loopTile->loopScop, "h", loopTile->loopDimSize);
		for(int i=0; i< loopTile->loopDimSize; i++){
			loopTile->iterators = isl_id_list_add(loopTile->iterators, isl_id_alloc(loopTile->loopScop->ctx,idNames[i],0));
		}
		free(idNames);

		idNames = loop_scop_get_id_names(loopTile->loopScop, "t", 1);
		loopTile->iterators = isl_id_list_add(loopTile->iterators, isl_id_alloc(loopTile->loopScop->ctx,idNames[0],0));
		free(idNames);

		for(int i=loopTile->hyperplaneStartsFrom; i<loopTile->loopDimSize; i++){
			loopTile->iterators = isl_id_list_add(loopTile->iterators, isl_id_alloc(loopTile->loopScop->ctx,loopTile->loopDimInfo[i].dimName,0));
		}
	}

	isl_debug_printf("\n%s\n", "#create wafefront schedule stop");
}

static isl_stat loop_tile_wafefront_schedule_for_map(__isl_take isl_map *map, void *user){
	loop_tile_info* loopTile = (loop_tile_info*)user;
	id_name *wfNames = loop_scop_get_id_names(loopTile->loopScop, "w", 1);
	const char *tupleName = isl_map_get_tuple_name(map, isl_dim_in);
	char *mapStr = isl_map_to_str(map);
	char *mapToStr = str_substring(mapStr,"] -> [", "]");
	isl_union_map *wafefrontScheduleMap = 0;

	wafefrontScheduleMap = loop_tile_space_to_wafefront_schedule(wafefrontScheduleMap, loopTile, tupleName, mapToStr, wfNames[0]);

	if(!loopTile->wafefrontScheduleMap){
		loopTile->wafefrontScheduleMap = isl_union_map_copy(wafefrontScheduleMap);
	}
	loopTile->wafefrontScheduleMap = isl_union_map_coalesce(isl_union_map_union(loopTile->wafefrontScheduleMap, wafefrontScheduleMap));

	free(mapToStr);
	free(mapStr);
	free(wfNames);
	isl_map_free(map);

	return isl_stat_ok;
}


static __isl_give isl_union_map * loop_tile_space_to_wafefront_schedule(__isl_take isl_union_map *wafefrontScheduleMap, __isl_keep loop_tile_info* loopTile, const char *tupleName, char *mapToStr, id_name wfName){
	char equationStr[65536];
	char tmpNamesStr[512];
	char tmpEquationStr[65536];

	equationStr[0] = '\0';

	if(strlen(loopTile->domainParams)>0){
		sprintf(&equationStr[strlen(equationStr)],"[%s] -> { ", loopTile->domainParams);
	}
	sprintf(&equationStr[strlen(equationStr)],"%s[%s", tupleName, loopTile->loopDimInfo[0].dimName);
	for(int i=1; i<loopTile->loopDimSize; i++){
		sprintf(&equationStr[strlen(equationStr)],", %s", loopTile->loopDimInfo[i].dimName);
	}
	sprintf(&equationStr[strlen(equationStr)],"] -> [%d", loopTile->scheduleIdx);

	for(int i=0; i<loopTile->hyperplaneStartsFrom; i++){
		sprintf(&equationStr[strlen(equationStr)],", %s", loopTile->loopDimInfo[i].dimName);
	}

	sprintf(&equationStr[strlen(equationStr)],", %s", wfName);

	if(loopTile->hyperplaneDimSize >= 2){
		for(int i=0; i<loopTile->hyperplaneDimSize; i++){
			sprintf(&equationStr[strlen(equationStr)],", %s", loopTile->spaceInfo[i].spaceName);
		}
		for(int i=loopTile->hyperplaneDimSize; i<loopTile->loopDimSize; i++){
			sprintf(&equationStr[strlen(equationStr)],", %s", "0");
		}
	}
	else{
		for(int i=0; i<loopTile->loopDimSize; i++){
			sprintf(&equationStr[strlen(equationStr)],", %s", "0");
		}
	}

	sprintf(&equationStr[strlen(equationStr)],", t%s", wfName);
	for(int i=loopTile->hyperplaneStartsFrom; i<loopTile->loopDimSize; i++){
		sprintf(&equationStr[strlen(equationStr)],", %s", loopTile->loopDimInfo[i].dimName);
	}

	if(loopTile->hyperplaneDimSize >= 2){
		tmpNamesStr[0]='\0';
		tmpEquationStr[0]='\0';
		sprintf(&tmpNamesStr[strlen(tmpNamesStr)]," b%s", wfName);
		for(int spaceInfoPos=0; spaceInfoPos<loopTile->hyperplaneDimSize; spaceInfoPos++){
			space_Info* spaceInfo = &(loopTile->spaceInfo[spaceInfoPos]);
			int* spaceInfoVector = spaceInfo->vector;
			int spaceSize = loopTile->daptParams->size;

			if(spaceInfoPos < loopTile->daptParams->sizesDim){
				spaceSize = loopTile->daptParams->sizes[spaceInfoPos];
			}

			sprintf(&tmpNamesStr[strlen(tmpNamesStr)],", b%s", spaceInfo->spaceName);

			sprintf(&tmpEquationStr[strlen(tmpEquationStr)],"and %i * %s", spaceInfoVector[0], loopTile->loopDimInfo[0].dimName);
			for(int i=1; i<loopTile->loopDimSize; i++){
				sprintf(&tmpEquationStr[strlen(tmpEquationStr)]," + %i * %s", spaceInfoVector[i], loopTile->loopDimInfo[i].dimName);
			}
			sprintf(&tmpEquationStr[strlen(tmpEquationStr)]," - b%s = 0 and ", spaceInfo->spaceName);

			sprintf(&tmpEquationStr[strlen(tmpEquationStr)],"%i * %s", spaceSize, spaceInfo->spaceName);
			sprintf(&tmpEquationStr[strlen(tmpEquationStr)]," <= b%s <= ", spaceInfo->spaceName);
			sprintf(&tmpEquationStr[strlen(tmpEquationStr)],"%i * (%s + 1) - 1 ", spaceSize, spaceInfo->spaceName);
		}

		sprintf(&equationStr[strlen(equationStr)],"] : exists %s : 1 = 1 %s", tmpNamesStr, tmpEquationStr );
	}
	else{
		sprintf(&equationStr[strlen(equationStr)],"] : 1 = 1 " );
	}

	if(loopTile->wafefrontHyperplane!=0 && loopTile->hyperplaneDimSize >= 2){
		sprintf(&equationStr[strlen(equationStr)]," and b%s = %i * b%s", wfName, get_pw_aff_value_to_int(isl_set_dim_min(isl_set_copy(loopTile->wafefrontHyperplane), 0)), loopTile->spaceInfo[0].spaceName);
		for(int spaceInfoPos=1; spaceInfoPos<loopTile->hyperplaneDimSize; spaceInfoPos++){
			sprintf(&equationStr[strlen(equationStr)]," + %i * b%s", get_pw_aff_value_to_int(isl_set_dim_min(isl_set_copy(loopTile->wafefrontHyperplane), spaceInfoPos)), loopTile->spaceInfo[spaceInfoPos].spaceName);
		}
		sprintf(&equationStr[strlen(equationStr)]," and %s = %i * %s", wfName, get_pw_aff_value_to_int(isl_set_dim_min(isl_set_copy(loopTile->wafefrontHyperplane), 0)), loopTile->spaceInfo[0].spaceName);
		for(int spaceInfoPos=1; spaceInfoPos<loopTile->hyperplaneDimSize; spaceInfoPos++){
			sprintf(&equationStr[strlen(equationStr)]," + %i * %s", get_pw_aff_value_to_int(isl_set_dim_min(isl_set_copy(loopTile->wafefrontHyperplane), spaceInfoPos)), loopTile->spaceInfo[spaceInfoPos].spaceName);
		}

		if(loopTile->daptParams->sizeTimeTile > 0 ){
			sprintf(&equationStr[strlen(equationStr)]," and %i * t%s <= b%s <= %i * (t%s + 1) - 1", loopTile->daptParams->sizeTimeTile, wfName, wfName, loopTile->daptParams->sizeTimeTile, wfName );
		} else{
			sprintf(&equationStr[strlen(equationStr)]," and t%s = 0", wfName);
		}
	}
	else{
		sprintf(&equationStr[strlen(equationStr)]," and %s = 0 and t%s = 0", wfName, wfName);
	}

	sprintf(&equationStr[strlen(equationStr)]," }");

	isl_debug_printf("\n#part: %s \n", equationStr );

	{
		isl_union_map* map;
		map = isl_union_map_read_from_str(loopTile->loopScop->ctx, equationStr);

		if(!wafefrontScheduleMap){
			wafefrontScheduleMap = isl_union_map_copy(map);
		}

		wafefrontScheduleMap = isl_union_map_coalesce(isl_union_map_intersect(wafefrontScheduleMap, map));
	}

	return wafefrontScheduleMap;
}

static void loop_tile_create_wafefront_code(__isl_keep loop_tile_info* loopTile){
	isl_debug_printf("\n%s\n", "#create code start");

	loopTile->wafefrontTileSchedule = isl_union_map_apply_range(isl_union_map_copy(loopTile->loopScop->loopMapper), isl_union_map_copy(loopTile->wafefrontScheduleMap));

	loopTile->wafefrontTileSchedule = isl_union_map_intersect_domain(loopTile->wafefrontTileSchedule, isl_union_set_copy(loopTile->loopScop->orygScop->loopDomain));

	isl_debug_printf_union_map("\n#wafefront code: %s\n", loopTile->wafefrontTileSchedule);

	isl_debug_printf("\n%s\n", "#create code stop");
}


static void loop_tile_create_approximated_relations(__isl_keep loop_tile_info* loopTile){
	//TODO: remove this functions
	return;

	isl_debug_printf("\n%s\n", "#create approximated relations start");

	isl_union_set_foreach_set_in_set(loopTile->deltaSetToScan, loop_tile_create_approximated_relations_for_set, loopTile);

	for(int i=0; i<loopTile->loopDimSize; i++){
		if(loopTile->loopDimInfo[i].isOrder == isl_bool_true){
			loop_tile_create_approximated_relations_for_x_set(1, i, loopTile);
			loop_tile_create_approximated_relations_for_x_set(-1, i, loopTile);
		} else
		if(loopTile->loopDimInfo[i].isNoLowerBound == isl_bool_true || loopTile->loopDimInfo[i].isNoUpperBound == isl_bool_true ){
			if(loopTile->loopDimInfo[i].isNoLowerBound == isl_bool_true){
				loop_tile_create_approximated_relations_for_x_set(-1, i, loopTile);
			}
			if(loopTile->loopDimInfo[i].isNoUpperBound == isl_bool_true){
				 loop_tile_create_approximated_relations_for_x_set(1, i, loopTile);
			}
		}
	}

	isl_debug_printf_union_map("\n#approximated relations: %s\n", loopTile->approximatedRelationsTpl);

	isl_debug_printf("\n%s\n", "#create approximated relations stop");
}

static isl_stat loop_tile_create_approximated_relations_for_set(__isl_take isl_set *set, void *user){
	loop_tile_info* loopTile = (loop_tile_info*)user;

	char equationStr[4096];

	equationStr[0] = '\0';

	if(strlen(loopTile->domainParams)>0){
		sprintf(&equationStr[strlen(equationStr)],"[%s] -> { ", loopTile->domainParams);
	}
	sprintf(&equationStr[strlen(equationStr)],"[%s", loopTile->loopDimInfo[0].dimName);
	for(int i=1; i<loopTile->loopDimSize; i++){
		sprintf(&equationStr[strlen(equationStr)],", %s", loopTile->loopDimInfo[i].dimName);
	}
	sprintf(&equationStr[strlen(equationStr)],"] -> [%s'", loopTile->loopDimInfo[0].dimName);
	for(int i=1; i<loopTile->loopDimSize; i++){
		sprintf(&equationStr[strlen(equationStr)],", %s'", loopTile->loopDimInfo[i].dimName);
	}
	sprintf(&equationStr[strlen(equationStr)],"] : 1 = 1");
	for(int i=0; i<loopTile->loopDimSize; i++){
		sprintf(&equationStr[strlen(equationStr)]," and %s' = %s + %d", loopTile->loopDimInfo[i].dimName, loopTile->loopDimInfo[i].dimName, get_pw_aff_value_to_int(isl_set_dim_min(isl_set_copy(set), i)));
	}
	sprintf(&equationStr[strlen(equationStr)]," }");

	isl_debug_printf("\n#part: %s \n", equationStr );

	{
		isl_union_map* map;
		map = isl_union_map_read_from_str(loopTile->loopScop->ctx, equationStr);

		if(!loopTile->approximatedRelationsTpl){
			loopTile->approximatedRelationsTpl = isl_union_map_copy(map);
		}

		loopTile->approximatedRelationsTpl = isl_union_map_coalesce(isl_union_map_union(loopTile->approximatedRelationsTpl, map));
	}

	isl_set_free(set);

	return isl_stat_ok;
}

static void loop_tile_create_approximated_relations_for_x_set(int value, int pos, __isl_keep loop_tile_info* loopTile){
	char equationStr[4096];

		equationStr[0] = '\0';

		if(strlen(loopTile->domainParams)>0){
			sprintf(&equationStr[strlen(equationStr)],"[%s] -> { ", loopTile->domainParams);
		}
		sprintf(&equationStr[strlen(equationStr)],"[%s", loopTile->loopDimInfo[0].dimName);
		for(int i=1; i<loopTile->loopDimSize; i++){
			sprintf(&equationStr[strlen(equationStr)],", %s", loopTile->loopDimInfo[i].dimName);
		}
		sprintf(&equationStr[strlen(equationStr)],"] -> [%s'", loopTile->loopDimInfo[0].dimName);
		for(int i=1; i<loopTile->loopDimSize; i++){
			sprintf(&equationStr[strlen(equationStr)],", %s'", loopTile->loopDimInfo[i].dimName);
		}
		sprintf(&equationStr[strlen(equationStr)],"] : 1 = 1");
		for(int i=0; i<loopTile->loopDimSize; i++){
			if(i != pos)
				sprintf(&equationStr[strlen(equationStr)]," and %s' = %s + %d", loopTile->loopDimInfo[i].dimName, loopTile->loopDimInfo[i].dimName, 0);
			else
				sprintf(&equationStr[strlen(equationStr)]," and %s' = %s + %d", loopTile->loopDimInfo[i].dimName, loopTile->loopDimInfo[i].dimName, value);
		}
		sprintf(&equationStr[strlen(equationStr)]," }");

		isl_debug_printf("\n#part: %s \n", equationStr );

		{
			isl_union_map* map;
			map = isl_union_map_read_from_str(loopTile->loopScop->ctx, equationStr);

			if(!loopTile->approximatedRelationsTpl){
				loopTile->approximatedRelationsTpl = isl_union_map_copy(map);
			}

			loopTile->approximatedRelationsTpl = isl_union_map_coalesce(isl_union_map_union(loopTile->approximatedRelationsTpl, map));
		}
}
