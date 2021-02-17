/*
 * dapt_params.h
 *
 *  Created on: May 25, 2020
 *
 */

#include <isl/ctx.h>

#ifndef DAPT_PARAMS_H_
#define DAPT_PARAMS_H_

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct dapt_params{
	int size;
	int sizeTimeTile;
	int sizesDim;
	int *sizes;
	int method;
	char *filename;
	char iteratortype[32];
	isl_bool debugPrintOn;
	isl_bool help;
	isl_bool isl_schedule_on;
	isl_bool isl_schedule_show_code;
	isl_bool dapt_scop_split;
	isl_bool dapt_no_tiles;
	isl_bool dapt_optimize_off;
	isl_bool dapt_no_order_dims;
	isl_bool dapt_unit_spacee_only;
	isl_bool dapt_respects_deps;
} dapt_params;

void read_dapt_params(dapt_params *daptParams, int argc, char *argv[]);

#if defined(__cplusplus)
}
#endif

#endif /* DAPT_PARAMS_H_ */
