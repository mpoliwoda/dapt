/*
 * tool_isl.h
 *
 *  Created on: May 14, 2020
 *
 */

#include <isl/map.h>
#include <isl/union_map.h>
#include <isl/set.h>
#include <isl/union_set.h>
#include <isl/schedule.h>

#ifndef TOOL_ISL_H_
#define TOOL_ISL_H_

#if defined(__cplusplus)
extern "C" {
#endif

isl_stat isl_union_map_foreach_map_in_map(__isl_keep isl_union_map *umap,
	isl_stat (*fn)(__isl_take isl_map *map, void *user), void *user);
isl_stat isl_union_set_foreach_set_in_set(__isl_keep isl_union_set *uset,
	isl_stat (*fn)(__isl_take isl_set *set, void *user), void *user);

int isl_printf_str(const char* template, __isl_take char* isl_str);
int isl_printf_union_set(const char* template, __isl_keep isl_union_set* uset);
int isl_printf_union_map(const char* template, __isl_keep isl_union_map* umap);
int isl_printf_schedule(const char* template, __isl_keep isl_schedule* sched);

isl_set* isl_set_renew(__isl_take isl_set* set);
isl_map* isl_map_renew(__isl_take isl_map* map);

char* get_pw_aff_value_to_str(isl_pw_aff* pw_aff);
int get_pw_aff_value_to_int(__isl_take isl_pw_aff* pw_aff);

isl_stat clear_tuple_names_in_map(__isl_take isl_map *map, void *user);
isl_stat clear_tuple_name_in_set(__isl_take isl_set *set, void *user);

__isl_give isl_union_set* isl_union_set_copy_by_str(__isl_keep isl_union_set* uset);

__isl_give isl_set* convert_map_out_dims_to_set(__isl_keep isl_map *map);
__isl_give isl_map* isl_map_reverse_as_new(__isl_take isl_map *map);

#if defined(__cplusplus)
}
#endif

#endif /* TOOL_ISL_H_ */
