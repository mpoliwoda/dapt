/*
 * norm_scop.c
 *
 *  Created on: May 14, 2020
 *
 */

#include <string.h>
#include <isl/constraint.h>
#include "tool_string.h"
#include "tool_isl.h"
#include "norm_map.h"
#include "tool_isl.h"
#include "tool_debug.h"

#include "norm_scop.h"

typedef struct {
	int counter;
	char **bounds;
}bound_list;

typedef struct {
	int dimIdx;
	bound_list upper;
	bound_list lower;
}dim_bounds;

typedef struct out_dim_info{
	isl_bool isOrderDim;
	isl_bool isStatementOrderDim;
	int orderValue;
	char *value;
	char *orderId;
	const char *orderOwnerId;
}out_dim_info;

typedef struct schedule_map_info{
	loop_scop *loopScop;
	isl_map *scheduleMap;
	isl_map *scheduleMapper;
	isl_union_set *scheduleDomainBounds;
	int outDimCount;
	out_dim_info *outDimInfoList;
} schedule_map_info;

typedef struct global_out_dim_info{
	isl_bool isOrderDim;
	isl_bool isDimToRemap;
} global_out_dim_info;

typedef struct schedule_map_info_list{
	loop_scop *loopScop;

	int outDimCount;
	global_out_dim_info *globalOutDimInfo;

	int scheduleMapInfoCount;
	schedule_map_info *scheduleMapInfoList;
} schedule_map_info_list;

//#############################################################################################################################################

static __isl_give schedule_map_info_list * schedule_map_info_list_alloc(__isl_keep loop_scop *loopScop);
static void schedule_map_info_list_init(schedule_map_info_list * mapInfoList, __isl_keep loop_scop *loopScop);
static __isl_null schedule_map_info_list * schedule_map_info_list_free(__isl_take schedule_map_info_list *mapInfoList);
static isl_stat schedule_map_info_list_fill(__isl_take isl_map *map, void *user);
static void schedule_map_info_list_sort(__isl_keep schedule_map_info_list *mapInfoList);
static int  schedule_map_info_list_split(__isl_keep schedule_map_info_list **mapInfoList);
static void schedule_map_info_list_build_mapper(__isl_keep schedule_map_info_list *mapInfoList);
static void schedule_map_info_list_set_global_Out_Dim_Info(__isl_keep schedule_map_info_list *mapInfoList);

static void schedule_map_info_init(schedule_map_info *mapInfo, __isl_keep loop_scop *loopScop, __isl_take isl_map *map);
static void schedule_map_info_free(schedule_map_info *mapInfo);
static int schedule_map_info_compare(schedule_map_info *left, schedule_map_info *right);

static void schedule_map_calculate_mapper(dim_bounds *dimBounds, schedule_map_info_list *mapInfoList, schedule_map_info *mapInfo);
static void schedule_map_calculate_upper_bounds(schedule_map_info *mapInfo);

static isl_stat schedule_map_fill_upper_bounds_list(__isl_take isl_set *set, void *user);
isl_stat fill_bounds_from_constraint (__isl_take isl_constraint *constraint, void *user);


static isl_stat schedule_map_domain_bounds(__isl_take isl_set *set, void *user);


//#############################################################################################################################################

__isl_take loop_scop_list* normalize_calc_scop(__isl_keep loop_scop *orygScop){
	loop_scop_list *loopScopList = malloc(sizeof(loop_scop_list));

	loop_scop_from_pet_debug_printf(orygScop);

	int mapInfoCount = 1;
	schedule_map_info_list *mapInfoList = schedule_map_info_list_alloc(orygScop);
	isl_union_map *scheduleMap;
	schedule_mapper_info scheduleMapperInfo;

	if(orygScop->daptParams->isl_schedule_on == isl_bool_true)
		if(orygScop->daptParams->dapt_optimize_off == isl_bool_true)
			scheduleMap = isl_union_map_copy(orygScop->loopIslMap);
		else
			scheduleMap = shedule_map_normalize(orygScop->loopIslMap, orygScop->loopDomain);
	else
		if(orygScop->daptParams->dapt_optimize_off == isl_bool_true)
			scheduleMap = isl_union_map_copy(orygScop->loopMap);
		else
			scheduleMap = shedule_map_normalize(orygScop->loopMap, orygScop->loopDomain);

	isl_debug_printf("\n#%s\n", "######################################################################");
	isl_debug_printf_union_map("\n#norm schedule map:\n%s\n", scheduleMap);
	isl_debug_printf("\n#%s\n", "######################################################################");

	isl_union_map_foreach_map(scheduleMap, schedule_map_info_list_fill, mapInfoList);
	isl_union_map_free(scheduleMap);

	schedule_map_info_list_sort(mapInfoList);

	if(orygScop->daptParams->dapt_scop_split == isl_bool_true){
		mapInfoCount = schedule_map_info_list_split(&mapInfoList);
	}

	for(int i=0; i<mapInfoCount; i++){
		schedule_map_info_list_set_global_Out_Dim_Info(&(mapInfoList[i]));
		schedule_map_info_list_build_mapper(&(mapInfoList[i]));
	}

	loopScopList->count = mapInfoCount;
	loopScopList->loopScops = (loop_scop**)malloc((loopScopList->count)*sizeof(loop_scop*));

	for(int i=0; i<mapInfoCount; i++){
		scheduleMapperInfo.scheduleIdx = i;
		scheduleMapperInfo.scheduleMap = 0;
		scheduleMapperInfo.scheduleMapper = 0;

		for(int j=0; j<mapInfoList[i].scheduleMapInfoCount; j++){
			if(scheduleMapperInfo.scheduleMapper == 0){
				scheduleMapperInfo.scheduleMapper = isl_union_map_from_map(mapInfoList[i].scheduleMapInfoList[j].scheduleMapper);
			}else {
				scheduleMapperInfo.scheduleMapper = isl_union_map_union(scheduleMapperInfo.scheduleMapper, isl_union_map_from_map(mapInfoList[i].scheduleMapInfoList[j].scheduleMapper));
			}

			if(scheduleMapperInfo.scheduleMap == 0){
				scheduleMapperInfo.scheduleMap = isl_union_map_from_map(mapInfoList[i].scheduleMapInfoList[j].scheduleMap);
			}else {
				scheduleMapperInfo.scheduleMap = isl_union_map_union(scheduleMapperInfo.scheduleMap, isl_union_map_from_map(mapInfoList[i].scheduleMapInfoList[j].scheduleMap));
			}

			mapInfoList[i].scheduleMapInfoList[j].scheduleMapper = 0;
			mapInfoList[i].scheduleMapInfoList[j].scheduleMap = 0;
		}

		//

		loopScopList->loopScops[i] = loop_scop_from_schedule_mapper_info(orygScop, &scheduleMapperInfo);
		loop_scop_norm_debug_printf(loopScopList->loopScops[i]);

		isl_union_map_free(scheduleMapperInfo.scheduleMapper);
		isl_union_map_free(scheduleMapperInfo.scheduleMap);
	}

	for(int i=0; i<mapInfoCount; i++){
		schedule_map_info_list_free(&(mapInfoList[i]));
	}

	free(mapInfoList);

	return loopScopList;
}

__isl_null loop_scop_list* loop_scop_list_free(loop_scop_list* loopScopList){
	if(!loopScopList) return 0;

		for(int i=0; i<loopScopList->count ;i++){
			loopScopList->loopScops[i] = loop_scop_loop_scop_free(loopScopList->loopScops[i]);
		}

		free(loopScopList);

	return 0;
}

static __isl_give schedule_map_info_list * schedule_map_info_list_alloc(__isl_keep loop_scop *loopScop){
	schedule_map_info_list *mapInfoList = (schedule_map_info_list*)malloc(sizeof(schedule_map_info_list));

	schedule_map_info_list_init(mapInfoList, loopScop);

	return mapInfoList;
}

//#############################################################################################################################################

static void schedule_map_info_list_init(schedule_map_info_list * mapInfoList, __isl_keep loop_scop *loopScop){
	mapInfoList->loopScop = loopScop;
	mapInfoList->scheduleMapInfoCount = 0;
	mapInfoList->scheduleMapInfoList = 0;
	mapInfoList->outDimCount = 0;
	mapInfoList->globalOutDimInfo = 0;
}

__isl_null schedule_map_info_list * schedule_map_info_list_free(__isl_take schedule_map_info_list *mapInfoList){
	for(int i=0; i<mapInfoList->scheduleMapInfoCount; i++){
		schedule_map_info_free(&(mapInfoList->scheduleMapInfoList[i]));
	}

	free(mapInfoList->globalOutDimInfo);
	free(mapInfoList->scheduleMapInfoList);

	return 0;
}

static isl_stat schedule_map_info_list_fill(__isl_take isl_map *map, void *user){
	schedule_map_info_list *mapInfoList = (schedule_map_info_list*)user;

	mapInfoList->scheduleMapInfoCount+=1;
	mapInfoList->scheduleMapInfoList = realloc(mapInfoList->scheduleMapInfoList, mapInfoList->scheduleMapInfoCount * sizeof(schedule_map_info));

	schedule_map_info_init(&(mapInfoList->scheduleMapInfoList[mapInfoList->scheduleMapInfoCount-1]), mapInfoList->loopScop, isl_map_copy(map));

	isl_map_free(map);

	return isl_stat_ok;
}

static void schedule_map_info_list_sort(schedule_map_info_list *mapInfoList){
	int outDimCount = mapInfoList->scheduleMapInfoList[0].outDimCount;
	isl_bool *isGlobalOrderDim = malloc(outDimCount*sizeof(isl_bool));

	for(int i=0; i<outDimCount; i++){
		isGlobalOrderDim[i] = isl_bool_true;

		for(int j=0; j<mapInfoList->scheduleMapInfoCount; j++){
			if( mapInfoList->scheduleMapInfoList[j].outDimInfoList[i].isOrderDim == isl_bool_false){
				isGlobalOrderDim[i] = isl_bool_false;
				break;
			}
		}
	}

	for(int i=0; i<outDimCount; i++){
		for(int j=0; j<mapInfoList->scheduleMapInfoCount && isGlobalOrderDim[i] == isl_bool_false; j++){
			if(mapInfoList->scheduleMapInfoList[j].outDimInfoList[i].isOrderDim == isl_bool_true){
				if(i == 0){
					mapInfoList->scheduleMapInfoList[j].outDimInfoList[i].isOrderDim = isl_bool_false;
				}
				else{
					if(isGlobalOrderDim[i-1] == isl_bool_false){
						mapInfoList->scheduleMapInfoList[j].outDimInfoList[i].isOrderDim = isl_bool_false;
					}
					else{
						for(int k=0; k<mapInfoList->scheduleMapInfoCount; k++){
							if(mapInfoList->scheduleMapInfoList[k].outDimInfoList[i].isOrderDim == isl_bool_false){
								isl_bool isDiffGroup = isl_bool_false;
								for(int l=i-1; l >= 0; l--){
									if(strcmp(mapInfoList->scheduleMapInfoList[j].outDimInfoList[l].value, mapInfoList->scheduleMapInfoList[k].outDimInfoList[l].value) != 0){
										isDiffGroup = isl_bool_true;
									}
								}
								if(isDiffGroup == isl_bool_false){
									mapInfoList->scheduleMapInfoList[j].outDimInfoList[i].isOrderDim = isl_bool_false;
									break;
								}
							}
						}
					}
				}
			}
		}
	}


	for(int i=1; i<mapInfoList->scheduleMapInfoCount; i++){
		for(int j=i; j>0; j--){
			for(int k=0; k<outDimCount; k++){
				if(isGlobalOrderDim[k] == isl_bool_true){
					if(mapInfoList->scheduleMapInfoList[j-1].outDimInfoList[k].orderValue != mapInfoList->scheduleMapInfoList[j].outDimInfoList[k].orderValue){
						if(mapInfoList->scheduleMapInfoList[j-1].outDimInfoList[k].orderValue > mapInfoList->scheduleMapInfoList[j].outDimInfoList[k].orderValue){
							schedule_map_info tmp = mapInfoList->scheduleMapInfoList[j-1];
							mapInfoList->scheduleMapInfoList[j-1] = mapInfoList->scheduleMapInfoList[j];
							mapInfoList->scheduleMapInfoList[j] = tmp;
						}
						break;
					}
				}
			}
		}
	}
}

static int  schedule_map_info_list_split(__isl_keep schedule_map_info_list **mapInfoList){
	int splitedScopes = 0;
	schedule_map_info_list *oryginalMapInfoList = mapInfoList[0];

	schedule_map_info_list *splitedMapInfoList = 0;
	schedule_map_info_list *currentMapInfoList = 0;

	schedule_map_info* lastAddedScheduleMapInfo = 0;

	for(int i=0; i<oryginalMapInfoList->scheduleMapInfoCount; i++ ){

		if(lastAddedScheduleMapInfo!=0){
			if(schedule_map_info_compare(lastAddedScheduleMapInfo, &(oryginalMapInfoList->scheduleMapInfoList[i])) != 0){
				currentMapInfoList = 0;
			}
		}

		if(currentMapInfoList == 0){
			splitedScopes+=1;
			splitedMapInfoList = realloc(splitedMapInfoList, splitedScopes*sizeof(schedule_map_info_list));
			currentMapInfoList = &(splitedMapInfoList[splitedScopes-1]);
			schedule_map_info_list_init(currentMapInfoList, oryginalMapInfoList->loopScop);
		}

		currentMapInfoList->scheduleMapInfoCount+=1;
		currentMapInfoList->scheduleMapInfoList = realloc(currentMapInfoList->scheduleMapInfoList, currentMapInfoList->scheduleMapInfoCount * sizeof(schedule_map_info));
		currentMapInfoList->scheduleMapInfoList[currentMapInfoList->scheduleMapInfoCount-1] = oryginalMapInfoList->scheduleMapInfoList[i];

		lastAddedScheduleMapInfo = &(oryginalMapInfoList->scheduleMapInfoList[i]);
	}
	free(oryginalMapInfoList->scheduleMapInfoList);
	free(oryginalMapInfoList);

	mapInfoList[0] = splitedMapInfoList;

	return splitedScopes;
}

static int schedule_map_info_compare(schedule_map_info *left, schedule_map_info *right){
	int retVal = 0;
	for(int i=0; i<left->outDimCount; i++){
		if(left->outDimInfoList[i].isOrderDim == isl_bool_true && right->outDimInfoList[i].isOrderDim == isl_bool_true){
			if(left->outDimInfoList[i].orderValue > right->outDimInfoList[i].orderValue){
				retVal = 1;
				break;
			}
			if(left->outDimInfoList[i].orderValue < right->outDimInfoList[i].orderValue){
				retVal = -1;
				break;
			}
		}else{
			if(left->outDimInfoList[i].isOrderDim == isl_bool_false && right->outDimInfoList[i].isOrderDim == isl_bool_true){
				retVal = -1;
			}
			if(left->outDimInfoList[i].isOrderDim == isl_bool_true && right->outDimInfoList[i].isOrderDim == isl_bool_false){
				retVal = 1;
			}
			break;
		}
	}
	return retVal;
}

static void schedule_map_info_list_build_mapper(__isl_keep schedule_map_info_list *mapInfoList){
	dim_bounds *dimBounds = malloc(mapInfoList->outDimCount * sizeof(dim_bounds));

	for(int i=0; i<mapInfoList->scheduleMapInfoCount; i++){
		isl_printer *printer = isl_printer_to_str(mapInfoList->loopScop->ctx);
		printer = isl_printer_print_int(printer, 0);

		for(int k=0; k<mapInfoList->outDimCount; k++){
			if(mapInfoList->globalOutDimInfo[k].isOrderDim == isl_bool_true){
				printer = isl_printer_print_int(printer, mapInfoList->scheduleMapInfoList[i].outDimInfoList[k].orderValue);
			}

			if(mapInfoList->globalOutDimInfo[k].isDimToRemap == isl_bool_true){
				char *codeStr = isl_printer_get_str(printer);
				mapInfoList->scheduleMapInfoList[i].outDimInfoList[k].orderId = codeStr;
			}

			if(mapInfoList->globalOutDimInfo[k].isDimToRemap == isl_bool_true){
				for(int j=i-1; 0 <= j; j--){
					if(strcmp(mapInfoList->scheduleMapInfoList[i].outDimInfoList[k].orderId, mapInfoList->scheduleMapInfoList[j].outDimInfoList[k].orderId) != 0){
						if(mapInfoList->scheduleMapInfoList[j].outDimInfoList[k].orderOwnerId == 0){
							mapInfoList->scheduleMapInfoList[j].outDimInfoList[k].orderOwnerId = mapInfoList->scheduleMapInfoList[i].outDimInfoList[k].orderId;
						}
					}
				}
			}
		}

		isl_printer_free(printer);
	}

	for(int i=0; i<mapInfoList->scheduleMapInfoCount; i++){
		for(int k=0; k<mapInfoList->outDimCount; k++){
			dimBounds[k].dimIdx = k;

			dimBounds[k].lower.bounds=0;
			dimBounds[k].lower.counter = 0;
			dimBounds[k].upper.bounds=0;
			dimBounds[k].upper.counter = 0;
		}

		if(mapInfoList->loopScop->daptParams->isl_schedule_on == isl_bool_false && mapInfoList->loopScop->daptParams->dapt_optimize_off == isl_bool_false){
			for(int k=0; k<mapInfoList->outDimCount; k++){
				if(mapInfoList->globalOutDimInfo[k].isDimToRemap == isl_bool_true && mapInfoList->scheduleMapInfoList[i].outDimInfoList[k].isOrderDim == isl_bool_true){
					isl_union_set *scheduleDomainBounds = 0;

					for(int j=i-1; 0 <= j; j--){
						if(mapInfoList->scheduleMapInfoList[j].outDimInfoList[k].orderOwnerId!=0){
							if(strcmp(mapInfoList->scheduleMapInfoList[i].outDimInfoList[k].orderId, mapInfoList->scheduleMapInfoList[j].outDimInfoList[k].orderOwnerId) == 0){
								if(scheduleDomainBounds)
									scheduleDomainBounds = isl_union_set_union(scheduleDomainBounds, isl_union_set_copy(mapInfoList->scheduleMapInfoList[j].scheduleDomainBounds));
								else
									scheduleDomainBounds = isl_union_set_copy(mapInfoList->scheduleMapInfoList[j].scheduleDomainBounds);
							}
						}
					}

					if(scheduleDomainBounds){
						scheduleDomainBounds = isl_union_set_coalesce(scheduleDomainBounds);
						isl_union_set_foreach_set_in_set(scheduleDomainBounds, schedule_map_fill_upper_bounds_list ,&(dimBounds[k]));

						//isl_debug_printf_int("\n#statement:\n%d\n",i);
						//isl_debug_printf_int("\n#dim:\n%d\n",k);
						//isl_debug_printf("\n#orderId:\n%s\n",mapInfoList->scheduleMapInfoList[i].outDimInfoList[k].orderId);
						//isl_debug_printf_union_set("\n#scheduleDomainBounds:\n%s\n", scheduleDomainBounds);
					}
					isl_union_set_free(scheduleDomainBounds);
				}
			}
		}


		schedule_map_calculate_mapper(dimBounds, mapInfoList, &(mapInfoList->scheduleMapInfoList[i]));
		schedule_map_calculate_upper_bounds(&(mapInfoList->scheduleMapInfoList[i]));

		for(int k=0; k<mapInfoList->outDimCount; k++){
			for(int j=0; j<dimBounds[k].lower.counter; j++){
				free(dimBounds[k].lower.bounds[j]);
			}
			free(dimBounds[k].lower.bounds);

			for(int j=0; j<dimBounds[k].upper.counter; j++){
				free(dimBounds[k].upper.bounds[j]);
			}
			free(dimBounds[k].upper.bounds);
		}
	}

	free(dimBounds);
}

static void schedule_map_info_list_set_global_Out_Dim_Info(__isl_keep schedule_map_info_list *mapInfoList){
	mapInfoList->outDimCount = mapInfoList->scheduleMapInfoList[0].outDimCount;
	mapInfoList->globalOutDimInfo = malloc(mapInfoList->outDimCount * sizeof(global_out_dim_info));

	for(int i=0; i<mapInfoList->outDimCount; i++){
		mapInfoList->globalOutDimInfo[i].isDimToRemap = isl_bool_false;
		mapInfoList->globalOutDimInfo[i].isOrderDim = isl_bool_true;

		for(int j=0; j<mapInfoList->scheduleMapInfoCount; j++){
			if(mapInfoList->scheduleMapInfoList[j].outDimInfoList[i].isOrderDim == isl_bool_false){
				mapInfoList->globalOutDimInfo[i].isOrderDim = isl_bool_false;
				break;
			}
		}

		if(mapInfoList->globalOutDimInfo[i].isOrderDim == isl_bool_false){
			mapInfoList->globalOutDimInfo[i].isDimToRemap = isl_bool_true;
		}
	}

	for(int i=mapInfoList->outDimCount-1; i>0; i--){
		if(mapInfoList->globalOutDimInfo[i].isDimToRemap == isl_bool_true){
			break;
		}
		else{
			for(int j=0; j<mapInfoList->scheduleMapInfoCount; j++){
				mapInfoList->scheduleMapInfoList[j].outDimInfoList[i].isStatementOrderDim = isl_bool_true;
			}
		}
	}
}

//#############################################################################################################################################

static void schedule_map_info_init(schedule_map_info *mapInfo, __isl_keep loop_scop *loopScop, __isl_take isl_map *map){
	mapInfo->loopScop = loopScop;
	mapInfo->scheduleMap = map;
	mapInfo->scheduleMapper =0;
	mapInfo->scheduleDomainBounds = 0;

	mapInfo->outDimCount = isl_map_dim(map,isl_dim_out);
	mapInfo->outDimInfoList = malloc(mapInfo->outDimCount*sizeof(out_dim_info));

	{
		isl_set *set = convert_map_out_dims_to_set(map);
		isl_map *reverseMap = isl_map_reverse_as_new(isl_map_copy(map));

		for(int i=0; i<mapInfo->outDimCount; i++){
			const char *dimName = isl_map_get_dim_name(reverseMap, isl_dim_in, i);

			mapInfo->outDimInfoList[i].value = get_pw_aff_value_to_str(isl_set_dim_max(isl_set_copy(set), i));

			if(dimName != 0){
				mapInfo->outDimInfoList[i].isOrderDim = isl_bool_false;
			} else {
				mapInfo->outDimInfoList[i].isOrderDim = isl_bool_true;
				mapInfo->outDimInfoList[i].orderValue = atoi(mapInfo->outDimInfoList[i].value);
			}

			mapInfo->outDimInfoList[i].isStatementOrderDim = isl_bool_false;
			mapInfo->outDimInfoList[i].orderId = 0;
			mapInfo->outDimInfoList[i].orderOwnerId = 0;
		}
		isl_map_free(reverseMap);
		isl_set_free(set);
	}
}

static void schedule_map_info_free(schedule_map_info *mapInfo){
	isl_map_free(mapInfo->scheduleMap);
	isl_map_free(mapInfo->scheduleMapper);
	isl_union_set_free(mapInfo->scheduleDomainBounds);

	for(int i=0; i<mapInfo->outDimCount; i++){
		free(mapInfo->outDimInfoList[i].orderId);
		free(mapInfo->outDimInfoList[i].value);
	}
	free(mapInfo->outDimInfoList);
}

static void schedule_map_calculate_mapper(dim_bounds *dimBounds, schedule_map_info_list *mapInfoList, schedule_map_info *mapInfo){
	id_name* outDimNames = loop_scop_get_id_names(mapInfoList->loopScop, "i", mapInfoList->outDimCount);

	mapInfo->scheduleMapper = isl_map_copy(mapInfo->scheduleMap);

	{
		const char *tupleName = isl_map_get_tuple_name(mapInfo->scheduleMapper, isl_dim_in);
		mapInfo->scheduleMapper = isl_map_set_tuple_name(mapInfo->scheduleMapper, isl_dim_out, tupleName );
	}

	for(int i=0; i<mapInfoList->outDimCount; i++){
		mapInfo->scheduleMapper = isl_map_set_dim_name(mapInfo->scheduleMapper, isl_dim_out,i, outDimNames[i]);
	}

	{
		isl_set *set = convert_map_out_dims_to_set(mapInfo->scheduleMapper);
		isl_printer *printer = isl_printer_to_str(mapInfoList->loopScop->ctx);
		char *mapperStr = 0, *constraintStr = 0;

		printer = isl_printer_print_str(printer, " : 1 = 1 ");

		for(int k=0; k< mapInfoList->outDimCount; k++){
			if(mapInfoList->globalOutDimInfo[k].isDimToRemap == isl_bool_true && mapInfoList->loopScop->daptParams->isl_schedule_on == isl_bool_false && mapInfoList->loopScop->daptParams->dapt_optimize_off == isl_bool_false){
				if(dimBounds[k].upper.counter > 0){
					char *dimValue = get_pw_aff_value_to_str(isl_set_dim_max(isl_set_copy(set), k));

					printer = isl_printer_print_str(printer, " and ( 1 = 0 ");

					for(int i = 0; i < dimBounds[k].upper.counter; i ++ ){

						printer = isl_printer_print_str(printer, " or ( ");

						printer = isl_printer_print_str(printer, outDimNames[k]);
						printer = isl_printer_print_str(printer, " = ");
						printer = isl_printer_print_str(printer, dimBounds[k].upper.bounds[i]);
						printer = isl_printer_print_str(printer, " + 1");

						for(int j = 0; j < dimBounds[k].upper.counter; j ++ ){
							printer = isl_printer_print_str(printer, " and ");
							printer = isl_printer_print_str(printer, dimBounds[k].upper.bounds[i]);
							printer = isl_printer_print_str(printer, " >= ");
							printer = isl_printer_print_str(printer, dimBounds[k].upper.bounds[j]);
						}

						printer = isl_printer_print_str(printer, " ) ");
					}

					printer = isl_printer_print_str(printer, " ) ");

					mapInfo->scheduleMapper = isl_map_drop_constraints_involving_dims(mapInfo->scheduleMapper, isl_dim_out, k, 1);

					free(dimValue);
				}
				else if(dimBounds[k].lower.counter > 0){
					char *dimValue = get_pw_aff_value_to_str(isl_set_dim_max(isl_set_copy(set), k));

					printer = isl_printer_print_str(printer, " and ( 1 = 0 ");

					for(int i = 0; i < dimBounds[k].lower.counter; i ++ ){

						printer = isl_printer_print_str(printer, " or ( ");

						printer = isl_printer_print_str(printer, outDimNames[k]);
						printer = isl_printer_print_str(printer, " = ");
						printer = isl_printer_print_str(printer, dimBounds[k].lower.bounds[i]);

						for(int j = 0; j < dimBounds[k].lower.counter; j ++ ){
							printer = isl_printer_print_str(printer, " and ");
							printer = isl_printer_print_str(printer, dimBounds[k].lower.bounds[i]);
							printer = isl_printer_print_str(printer, " >= ");
							printer = isl_printer_print_str(printer, dimBounds[k].lower.bounds[j]);
						}

						printer = isl_printer_print_str(printer, " ) ");
					}

					printer = isl_printer_print_str(printer, " ) ");

					mapInfo->scheduleMapper = isl_map_drop_constraints_involving_dims(mapInfo->scheduleMapper, isl_dim_out, k, 1);

					free(dimValue);

				} else if(mapInfo->outDimInfoList[k].isOrderDim == isl_bool_true){
					printer = isl_printer_print_str(printer, " and ( 1 = 0 ");
					printer = isl_printer_print_str(printer, " or ( ");
					printer = isl_printer_print_str(printer, outDimNames[k]);
					printer = isl_printer_print_str(printer, " = ");
					printer = isl_printer_print_str(printer, "0");
					printer = isl_printer_print_str(printer, " ) ) ");

					mapInfo->scheduleMapper = isl_map_drop_constraints_involving_dims(mapInfo->scheduleMapper, isl_dim_out, k, 1);
				}
			}
		}

		mapperStr = isl_map_to_str(mapInfo->scheduleMapper);
		mapperStr[strlen(mapperStr)-1] = '\0';
		isl_map_free(mapInfo->scheduleMapper);

		constraintStr = isl_printer_get_str(printer);
		printer = isl_printer_flush(printer);

		printer = isl_printer_print_str(printer, mapperStr);
		printer = isl_printer_print_str(printer, constraintStr);
		printer = isl_printer_print_str(printer, " } ");
		free(mapperStr);
		free(constraintStr);

		mapperStr = isl_printer_get_str(printer);

		//isl_debug_printf("\n%s\n", mapperStr);

		mapInfo->scheduleMapper = isl_map_read_from_str(mapInfo->loopScop->ctx, mapperStr);
		free(mapperStr);


		isl_printer_free(printer);
		isl_set_free(set);
		free(outDimNames);
	}
}

static void schedule_map_calculate_upper_bounds(schedule_map_info *mapInfo){
	isl_union_set *domain = isl_union_set_coalesce(isl_union_map_domain(isl_union_map_reverse(isl_union_map_intersect_domain(isl_union_map_from_map(isl_map_copy(mapInfo->scheduleMapper)), isl_union_set_copy(mapInfo->loopScop->loopDomain)))));

	isl_union_set_foreach_set_in_set(domain, schedule_map_domain_bounds, mapInfo);

	mapInfo->scheduleDomainBounds = isl_union_set_coalesce(isl_union_set_remove_divs(mapInfo->scheduleDomainBounds));

	isl_union_set_free(domain);
}

static isl_stat schedule_map_domain_bounds(__isl_take isl_set *set, void *user){
	schedule_map_info *mapInfo = (schedule_map_info*)user;

	set = isl_set_set_tuple_name(set, "");

	for(int i=mapInfo->outDimCount-1; i>0; i--){
		if(mapInfo->outDimInfoList[i].isStatementOrderDim == isl_bool_true){
			set = isl_set_remove_dims(set, isl_dim_set,i,1);
		}
		else{
			break;
		}
	}

	if(mapInfo->scheduleDomainBounds)
		mapInfo->scheduleDomainBounds = isl_union_set_union(mapInfo->scheduleDomainBounds, isl_union_set_from_set(set));
	else
		mapInfo->scheduleDomainBounds = isl_union_set_from_set(set);

	return isl_stat_ok;
}

static isl_stat schedule_map_fill_upper_bounds_list(__isl_take isl_set *set, void *user){
	dim_bounds *dimBounds = (dim_bounds*)user;
	isl_basic_set *bset = 0;

	set = isl_set_remove_divs(set);

	{
		char *setStr = isl_set_to_str(set);
		bset = isl_basic_set_read_from_str(isl_set_get_ctx(set),setStr);
		free(setStr);
	}

	{
		int dimCount = isl_basic_set_dim(bset, isl_dim_set);
		dimCount = dimCount - dimBounds->dimIdx;
		dimCount = dimCount - 1;

		if(dimCount>0){
			bset = isl_basic_set_remove_dims(bset,isl_dim_set,dimBounds->dimIdx + 1, dimCount);
		}
	}

	{
		isl_constraint_list *constraintList = isl_basic_set_get_constraint_list(bset);
		isl_constraint_list_foreach(constraintList, fill_bounds_from_constraint, dimBounds);
		isl_constraint_list_free(constraintList);
	}

	isl_set_free(set);
	isl_basic_set_free(bset);

	return isl_stat_ok;
}

isl_stat fill_bounds_from_constraint (__isl_take isl_constraint *constraint, void *user){
	dim_bounds *dimBounds = (dim_bounds*)user;

	if(isl_constraint_is_upper_bound(constraint,isl_dim_set,dimBounds->dimIdx) == isl_bool_true){
		isl_aff *affbound = isl_constraint_get_bound(constraint,isl_dim_set,dimBounds->dimIdx);

		dimBounds->upper.counter += 1;
		dimBounds->upper.bounds = realloc(dimBounds->upper.bounds, sizeof(char*)*dimBounds->upper.counter);
		{
			char* aff_str = isl_aff_to_str(affbound);

			dimBounds->upper.bounds[dimBounds->upper.counter-1] = str_substring(aff_str, "-> [(", ")]");
			free(aff_str);
		}
		isl_aff_free(affbound);
	}

	if(isl_constraint_is_lower_bound(constraint,isl_dim_set,dimBounds->dimIdx) == isl_bool_true){
		isl_aff *affbound = isl_constraint_get_bound(constraint,isl_dim_set,dimBounds->dimIdx);

		dimBounds->lower.counter += 1;
		dimBounds->lower.bounds = realloc(dimBounds->lower.bounds, sizeof(char*)*dimBounds->lower.counter);

		{
			char* aff_str = isl_aff_to_str(affbound);

			dimBounds->lower.bounds[dimBounds->lower.counter-1] = str_substring(aff_str, "-> [(", ")]");
			free(aff_str);
		}

		isl_aff_free(affbound);
	}

	isl_constraint_free(constraint);

	return isl_stat_ok;
}

//#############################################################################################################################################
