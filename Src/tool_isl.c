/*
 * tool_isl.c
 *
 *  Created on: May 14, 2020
 *
 */

#include <isl/aff.h>

#include "tool_isl.h"
#include "tool_string.h"
#include <string.h>


struct userCall{
	void* fn;
	void* user;
};

static isl_stat foreach_basic_set(__isl_take isl_basic_set *bset, void *user){
	isl_set *set;
	struct userCall* userCall = (struct userCall*)user;
	isl_stat (*fn)(__isl_take isl_set *set, void *user) = (isl_stat (*)(__isl_take isl_set *set, void *user))userCall->fn;

	set = isl_set_renew(isl_set_from_basic_set(bset));

	return fn(set, userCall->user);
}

static isl_stat foreach_set(__isl_take isl_set *set, void *user){
	struct userCall* userCall = (struct userCall*)user;

	isl_stat stat = isl_set_foreach_basic_set(set, foreach_basic_set, userCall);

	isl_set_free(set);
	return stat;
}

isl_stat isl_union_set_foreach_set_in_set(__isl_keep isl_union_set *uset,
	isl_stat (*fn)(__isl_take isl_set *set, void *user), void *user){

	struct userCall userCall;

	userCall.fn = (void*)fn;
	userCall.user = user;

	return isl_union_set_foreach_set(uset, foreach_set, &userCall);
}


static isl_stat foreach_basic_map(__isl_take isl_basic_map *bmap, void *user){
	isl_map *map;
	struct userCall* userCall = (struct userCall*)user;
	isl_stat (*fn)(__isl_take isl_map *map, void *user) = (isl_stat (*)(__isl_take isl_map *map, void *user))userCall->fn;

	map = isl_map_renew(isl_map_from_basic_map(bmap));

	return fn(map, userCall->user);
}

static isl_stat foreach_map(__isl_take isl_map *map, void *user){
	struct userCall* userCall = (struct userCall*)user;

	isl_stat stat = isl_map_foreach_basic_map(map, foreach_basic_map, userCall);

	isl_map_free(map);
	return stat;
}

isl_stat isl_union_map_foreach_map_in_map(__isl_keep isl_union_map *umap,
	isl_stat (*fn)(__isl_take isl_map *map, void *user), void *user){

	struct userCall userCall;

	userCall.fn = (void*)fn;
	userCall.user = user;

	return isl_union_map_foreach_map(umap, foreach_map, &userCall);
}

int isl_printf_union_set(const char* template, __isl_keep isl_union_set* uset){
	int result = 0;

	result = isl_printf_str(template, isl_union_set_to_str(uset));

	return result;
}

int isl_printf_str(const char* template, __isl_take char* isl_str){
	int result = 0;

	result = printf(template, isl_str);

	free(isl_str);

	return result;
}

int isl_printf_union_map(const char* template, __isl_keep isl_union_map* umap){
	int result = 0;

	result = isl_printf_str(template, isl_union_map_to_str(umap));

	return result;
}

int isl_printf_schedule(const char* template, __isl_keep isl_schedule* sched){
	int result = 0;

	result = isl_printf_str(template, isl_schedule_to_str(sched));

	return result;
}

isl_set* isl_set_renew(__isl_take isl_set* set){
	char *setStr = isl_set_to_str(set);
	isl_set *newSet = isl_set_read_from_str(isl_set_get_ctx(set), setStr);

	free(setStr);
	isl_set_free(set);

	return newSet;
}

isl_map* isl_map_renew(__isl_take isl_map* map){
	char *mapStr = isl_map_to_str(map);
	isl_map *newMap = isl_map_read_from_str(isl_map_get_ctx(map), mapStr);

	free(mapStr);
	isl_map_free(map);

	return newMap;
}

char* get_pw_aff_value_to_str(__isl_take isl_pw_aff* pw_aff){
	char* value_str;
	char* pw_aff_str = isl_pw_aff_to_str(pw_aff);

	value_str = str_substring(pw_aff_str, "{ [(", ")]");

	isl_pw_aff_free(pw_aff);
	free(pw_aff_str);

	return value_str;
}

int get_pw_aff_value_to_int(__isl_take isl_pw_aff* pw_aff){
	int value_int;
	char* value_str = get_pw_aff_value_to_str(pw_aff);

	value_int = atoi(value_str);

	free(value_str);
	return value_int;
}

isl_stat clear_tuple_names_in_map(__isl_take isl_map *map, void *user){
	isl_union_map** userUnionMap = (isl_union_map**)user;

	map = isl_map_set_tuple_name(map,isl_dim_in,"");
	map = isl_map_set_tuple_name(map,isl_dim_out,"");

	{
		isl_union_map* unionMap = isl_union_map_from_map(map);
		if(!userUnionMap[0]){
			userUnionMap[0] = isl_union_map_copy(unionMap);
		}
		userUnionMap[0] = isl_union_map_coalesce(isl_union_map_union(userUnionMap[0], unionMap));
	}

	return isl_stat_ok;
}

isl_stat clear_tuple_name_in_set(__isl_take isl_set *set, void *user){
	isl_union_set** userUnionSet = (isl_union_set**)user;

	set = isl_set_set_tuple_name(set, "");

	{
		isl_union_set* unionSet = isl_union_set_from_set(set);
		if(!userUnionSet[0]){
			userUnionSet[0] = isl_union_set_copy(unionSet);
		}
		userUnionSet[0] = isl_union_set_coalesce(isl_union_set_union(userUnionSet[0], unionSet));
	}

	return isl_stat_ok;
}

__isl_give isl_union_set* isl_union_set_copy_by_str(__isl_keep isl_union_set* uset){
	isl_union_set *result = 0;
	char *usetStr = isl_union_set_to_str(uset);

	result = isl_union_set_read_from_str(isl_union_set_get_ctx(uset), usetStr);
	free(usetStr);

	return result;
}

__isl_give isl_set* convert_map_out_dims_to_set(__isl_keep isl_map *map){
	isl_set *set = 0;

	map = isl_map_copy(map);
	map = isl_map_set_tuple_name(map, isl_dim_in, "");
	map = isl_map_set_tuple_name(map, isl_dim_out, "");

	{
		char *mapStr = isl_map_to_str(map);
		char *mapParams = str_substring(mapStr, "[", "] -> {");
		char *inDims = str_substring(mapStr, "{ [", "] -> [");
		char *outDims = str_substring(mapStr, "] -> [", "] }");
		int mapStrLen = strlen(mapStr)+10;

		free(mapStr);
		mapStr = malloc(mapStrLen*sizeof(char));

		if(strlen(mapParams) == 0){
			sprintf(mapStr, "[%s] -> { [%s] }", inDims, outDims );
		}else{
			if(strlen(inDims) == 0){
				sprintf(mapStr, "[%s] -> { [%s] }", mapParams, outDims );
			}else{
				sprintf(mapStr, "[%s, %s] -> { [%s] }", mapParams, inDims, outDims );
			}
		}

		set = isl_set_read_from_str(isl_map_get_ctx(map), mapStr);
	}

	isl_map_free(map);

	return set;
}

__isl_give isl_map* isl_map_reverse_as_new(__isl_take isl_map *map){
	isl_ctx * ctx = isl_map_get_ctx(map);
	isl_map *reverseMap =  isl_map_reverse(map);
	char * reverseMapStr = isl_map_to_str(reverseMap);

	isl_map_free(reverseMap);
	reverseMap = isl_map_read_from_str(ctx, reverseMapStr);
	free(reverseMapStr);

	return reverseMap;
}
