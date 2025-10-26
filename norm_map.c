/*
 * norm_map.c
 *
 *  Created on: Sep 22, 2020
 *
 */

#include <stdlib.h>
#include "norm_map.h"
#include "tool_debug.h"
#include "tool_isl.h"
#include <isl/union_map.h>
#include <isl/map.h>
#include <isl/set.h>
#include <string.h>

typedef struct out_dim_info{
	char *value;
	isl_bool isLoopOrderDim;
	isl_bool isStatementOrderDim;
	isl_bool isAddedDim;
}out_dim_info;

typedef struct statement_info{
	isl_ctx *ctx;
	char *mapstr;
	int outDimCount;
	out_dim_info *outDims;
} statement_info;

typedef struct statements_list_info{
	int count;
	int globalOutDimCount;
	statement_info *statements;
	isl_union_set *loopDomain;
} statements_list_info;


static isl_stat statements_list_fill(__isl_take isl_map *map, void *user);
static void statements_normalize_out_dims(statements_list_info *statementsList);
static void statement_add_order_dim(statement_info *statement, int dim);
static isl_union_map* statement_to_union_map(statement_info * statement);
static void statements_free(statements_list_info *statementsList);

__isl_give isl_union_map* shedule_map_normalize(__isl_keep isl_union_map* umap, __isl_keep isl_union_set* uset){
	isl_union_map *returnMap = 0;
	statements_list_info statementsList;

	statementsList.count = 0;
	statementsList.globalOutDimCount = 0;
	statementsList.statements = 0;
	statementsList.loopDomain = uset;

	isl_union_map_foreach_map(umap, statements_list_fill, &statementsList);
	statements_normalize_out_dims(&statementsList);
	for(int i=0; i<statementsList.count; i++){
		isl_union_map *statementMap = statement_to_union_map(&(statementsList.statements[i]));

		if(returnMap)
			returnMap = isl_union_map_union(returnMap, statementMap);
		else
			returnMap = statementMap;
	}

	statements_free(&statementsList);

	return returnMap;
}

static isl_stat statements_list_fill(__isl_take isl_map *map, void *user){
	statements_list_info *statementsList = (statements_list_info*)user;
	statement_info *statement;

	statementsList->count+=1;
	statementsList->statements = realloc(statementsList->statements, statementsList->count*sizeof(statement_info));

	statement = &(statementsList->statements[statementsList->count-1]);
	statement->mapstr = isl_map_to_str(map);
	statement->outDimCount = isl_map_dim(map, isl_dim_out);
	statement->outDims = malloc(statement->outDimCount*sizeof(out_dim_info));
	statement->ctx = isl_map_get_ctx(map);

	statementsList->globalOutDimCount = statement->outDimCount;

	{
		isl_set *set = convert_map_out_dims_to_set(map);
		isl_union_set * ureverseDomain = isl_union_map_domain(isl_union_map_reverse(isl_union_map_intersect_domain(isl_union_map_from_map(isl_map_copy(map)), isl_union_set_copy(statementsList->loopDomain))));

		char *usetStr = isl_union_set_to_str(ureverseDomain);
		isl_union_set_free(ureverseDomain);

		isl_set * reverseDomain = isl_set_read_from_str(statement->ctx, usetStr);
		free(usetStr);
		isl_map *reverseMap = isl_map_reverse_as_new(isl_map_copy(map));

		for(int i=0; i<statement->outDimCount; i++){
			char *minVal = get_pw_aff_value_to_str(isl_set_dim_min(isl_set_copy(reverseDomain), i));
			char *maxVal = get_pw_aff_value_to_str(isl_set_dim_max(isl_set_copy(reverseDomain), i));

			statement->outDims[i].isLoopOrderDim = isl_bool_false;
			statement->outDims[i].isStatementOrderDim = isl_bool_false;
			statement->outDims[i].isAddedDim = isl_bool_false;

			if(strcmp(minVal, maxVal) == 0 || isl_set_get_dim_name(reverseDomain, isl_dim_set, i) == 0  ){
				if(isl_map_get_dim_name(reverseMap, isl_dim_in, i) != 0){
					statement->outDims[i].value = malloc((strlen(minVal)+1)*sizeof(char));

					sprintf(statement->outDims[i].value,"%s", minVal);
				}
				else{
					statement->outDims[i].value = get_pw_aff_value_to_str(isl_set_dim_max(isl_set_copy(set), i));
				}

				if(strcmp(minVal, maxVal) == 0){
					char buf[256];
					sprintf(buf,"%d",atoi(statement->outDims[i].value));

					if(strcmp(buf,statement->outDims[i].value) == 0){
						statement->outDims[i].isLoopOrderDim = isl_bool_true;
					}
				}
			}else{
				statement->outDims[i].value = get_pw_aff_value_to_str(isl_set_dim_max(isl_set_copy(set), i));
			}

			free(minVal);
			free(maxVal);
		}

		isl_map_free(reverseMap);
		isl_set_free(reverseDomain);
		isl_set_free(set);
	}

	for(int i=statement->outDimCount-1; i>0 ; i--){
		if(statement->outDims[i].isLoopOrderDim == isl_bool_false){
			break;
		}
		statement->outDims[i].isStatementOrderDim = isl_bool_true;
	}

	isl_map_free(map);

	return isl_stat_ok;
}

static void statements_normalize_out_dims(statements_list_info *statementsList){
	for(int i=0; i<statementsList->globalOutDimCount; i++){
		for(int j=0; j<statementsList->count; j++){
			if(statementsList->statements[j].outDims[i].isLoopOrderDim == isl_bool_true){
				for(int k=0; k<statementsList->count; k++){
					if(statementsList->statements[k].outDims[i].isLoopOrderDim == isl_bool_false){
						isl_bool isDiffGroup = isl_bool_false;
						for(int l=i-1; l >= 0; l--){
							if(strcmp(statementsList->statements[j].outDims[l].value, statementsList->statements[k].outDims[l].value) != 0){
								isDiffGroup = isl_bool_true;
								break;
							}
						}
						if(isDiffGroup == isl_bool_false){
							statementsList->statements[j].outDims[i].isLoopOrderDim = isl_bool_false;
							break;
						}
					}
				}
			}
		}
	}

	for(int i=0; i<statementsList->globalOutDimCount; i++){
		isl_bool isOrderDim = isl_bool_false;
		isl_bool isNoOrderDim = isl_bool_false;

		for(int j=0; j<statementsList->count; j++){
			if(statementsList->statements[j].outDims[i].isStatementOrderDim == isl_bool_false){
				if(statementsList->statements[j].outDims[i].isLoopOrderDim == isl_bool_true){
					isOrderDim = isl_bool_true;
				}
				else{
					isNoOrderDim = isl_bool_true;
				}
			}
		}

		if(isOrderDim == isl_bool_true && isNoOrderDim == isl_bool_true){
			for(int j=0; j<statementsList->count; j++){
				statement_add_order_dim(&(statementsList->statements[j]), i);
			}
			statementsList->globalOutDimCount +=1;
		}
	}
}

static void statement_add_order_dim(statement_info *statement, int dim){
	out_dim_info *orderDim;

	statement->outDims = realloc(statement->outDims, (statement->outDimCount+1)*sizeof(out_dim_info));

	if(statement->outDims[dim].isLoopOrderDim == isl_bool_true){
		orderDim = &(statement->outDims[statement->outDimCount]);
	}
	else{
		for(int i=statement->outDimCount-1; i>= dim; i--){
			statement->outDims[i+1]=statement->outDims[i];
		}
		orderDim = &(statement->outDims[dim]);
	}
	statement->outDimCount += 1;

	orderDim->value = 0;
	orderDim->isLoopOrderDim = isl_bool_true;
	orderDim->isStatementOrderDim = isl_bool_true;
	orderDim->isAddedDim = isl_bool_true;
}

static isl_union_map* statement_to_union_map(statement_info * statement){

	char buff[2048];
	strstr(statement->mapstr,"-> [")[0] = '\0';

	sprintf(buff, "%s-> [", statement->mapstr);

	for(int i=0; i<statement->outDimCount; i++){
		if(statement->outDims[i].isAddedDim == isl_bool_true){
			sprintf(&(buff[strlen(buff)]), "%s", "0");
		}
		else{
			sprintf(&(buff[strlen(buff)]), "%s", statement->outDims[i].value);
		}
		sprintf(&(buff[strlen(buff)]), "%s", ",");
	}

	sprintf(&(buff[strlen(buff)-1]), "%s", "] }");


	return isl_union_map_read_from_str(statement->ctx, buff);
}

static void statements_free(statements_list_info *statementsList){
	for(int i=0; i<statementsList->count; i++){
		for(int j=0; j<statementsList->globalOutDimCount; j++){
			free(statementsList->statements[i].outDims[j].value);
		}
		free(statementsList->statements[i].mapstr);
		free(statementsList->statements[i].outDims);
	}
	free(statementsList->statements);
}
