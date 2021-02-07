/*
 * loop_tile.h
 *
 *  Created on: May 15, 2020
 *
 */

#include "loop_scop.h"
#include "norm_scop.h"
#include "dapt_params.h"

#ifndef LOOP_TILE_H_
#define LOOP_TILE_H_

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct space_Info{
	id_name spaceName;
	int* vector;
} space_Info;

typedef struct loop_dim_info{
	const char *dimName;
	isl_bool isOrder;
	isl_bool isNoLowerBound;
	isl_bool isNoUpperBound;
}loop_dim_info;

typedef struct hyperplane_check_info{
	int lastId;
	char paramIdList[256];
	char paramIdCondition[256];
	char** statementList;
} hyperplane_check_info;

typedef struct loop_tile_info{
	int scheduleIdx;
	int loopDimSize;
	int hyperplaneDimSize;
	int hyperplaneStartsFrom;

	char *domainParams;
	char spaceInfoEquations[65536];
	char spaceInfoParallelFreeEquations[65536];
	char tileInfoEquations[65536];

	loop_scop* loopScop;
	dapt_params *daptParams;
	loop_dim_info *loopDimInfo;
	hyperplane_check_info* hyperplaneCheckInfo;
	space_Info *spaceInfo;

	isl_union_set *deltaSetToScan;
	isl_set_list *spaceInfoHyperplanesList;
	isl_map *spaceInfoMapper;
	isl_union_map *tileMapper;
	isl_union_set* tileDelta;
	isl_union_set *tileDeltaSetToScan;
	isl_bool parallelFreeExists;
	isl_set* wafefrontHyperplane;
	isl_union_map *wafefrontScheduleMap;
	isl_union_map *wafefrontTileSchedule;

	isl_id_list* iterators;

	isl_union_map *approximatedRelationsTpl;
	isl_union_map *approximatedRelations;

} loop_tile_info;

typedef struct loop_tile_list{
	int count;
	loop_tile_info **loopsTile;
	isl_union_map *wafefrontTileSchedule;
} loop_tile_list;

__isl_give loop_tile_list* loop_tile_list_from_loop_scop_list(loop_scop_list *loopScopList);
__isl_null loop_tile_list* loop_tile_list_free(loop_tile_list *loopTileList);

__isl_give loop_tile_info* loop_tile_from_loop_scop(__isl_keep loop_scop* loopScop);
__isl_null loop_tile_info* loop_tile_free(__isl_take loop_tile_info* loopTile);

#if defined(__cplusplus)
}
#endif

#endif /* LOOP_TILE_H_ */
