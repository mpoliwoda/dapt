/*
 * norm_scop.h
 *
 *  Created on: May 14, 2020
 *
 */

#include "loop_scop.h"

#ifndef NORM_SCOP_H_
#define NORM_SCOP_H_

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct loop_scop_list{
	int count;
	loop_scop **loopScops;
} loop_scop_list;

__isl_take loop_scop_list* normalize_calc_scop(__isl_keep loop_scop *orygScop);
__isl_null loop_scop_list* loop_scop_list_free(loop_scop_list* loopScopList);

#if defined(__cplusplus)
}
#endif

#endif /* NORM_SCOP_H_ */
