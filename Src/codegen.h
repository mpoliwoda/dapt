/*
 * codegen.h
 *
 *  Created on: May 28, 2020
 *
 */
#include <pet.h>
#include <isl/union_map.h>

#ifndef CODEGEN_H_
#define CODEGEN_H_

#if defined(__cplusplus)
extern "C" {
#endif

char* codegen_to_str(__isl_keep isl_union_map *scheduleMap);
char* codegen_macros_to_str(__isl_keep isl_union_map *scheduleMap, __isl_keep pet_scop *pet);
char* codegen_wavefront_to_str(__isl_keep isl_union_map *scheduleMap, __isl_keep pet_scop *pet, isl_id_list* iterators, isl_bool isParallel);

#if defined(__cplusplus)
}
#endif

#endif /* CODEGEN_H_ */
