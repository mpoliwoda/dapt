/*
 * norm_map.h
 *
 *  Created on: Sep 22, 2020
 *      Author: maciej
 */

#include <isl/union_map.h>
#include <isl/union_set.h>

#ifndef NORM_MAP_H_
#define NORM_MAP_H_

#if defined(__cplusplus)
extern "C" {
#endif

__isl_give isl_union_map* shedule_map_normalize(__isl_keep isl_union_map* umap, __isl_keep isl_union_set* uset);

#if defined(__cplusplus)
}
#endif

#endif /* NORM_MAP_H_ */
