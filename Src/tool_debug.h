/*
 * tool_debug.h
 *
 *  Created on: May 15, 2020
 *
 */

#include <isl/union_map.h>
#include <isl/union_set.h>
#include <isl/map.h>
#include <isl/set.h>
#include <isl/schedule.h>

#ifndef TOOL_DEBUG_H_
#define TOOL_DEBUG_H_

#if defined(__cplusplus)
extern "C" {
#endif

void isl_debug_on();
int isl_debug_is_on();

int isl_debug_printf(const char* template, const char* str);
int isl_debug_printf_int(const char* template, int val);
int isl_debug_printf_str(const char* template, __isl_take char* isl_str);

int isl_debug_printf_union_set(const char* template, __isl_keep isl_union_set* uset);
int isl_debug_printf_union_map(const char* template, __isl_keep isl_union_map* umap);

int isl_debug_printf_set(const char* template, __isl_keep isl_set* set);
int isl_debug_printf_map(const char* template, __isl_keep isl_map* map);

int isl_debug_printf_schedule(const char* template, __isl_keep isl_schedule* sched);

#if defined(__cplusplus)
}
#endif

#endif /* TOOL_DEBUG_H_ */
