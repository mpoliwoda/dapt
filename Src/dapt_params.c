/*
 * dapt_params.c
 *
 *  Created on: May 25, 2020
 *
 */

#include "dapt_params.h"
#include "tool_string.h"

void read_dapt_params(dapt_params *daptParams, int argc, char *argv[]){
	char* paramValue;

	daptParams->size = 16;
	daptParams->sizeTimeTile = 0;
	daptParams->sizesDim = 0;
	daptParams->sizes = 0;
	daptParams->method = 1;
	daptParams->filename = 0;
	daptParams->debugPrintOn = isl_bool_false;
	daptParams->help = isl_bool_false;
	daptParams->isl_schedule_on = isl_bool_false;
	daptParams->dapt_scop_split = isl_bool_true;
	daptParams->dapt_no_tiles = isl_bool_false;
	daptParams->dapt_optimize_off = isl_bool_false;
	daptParams->dapt_no_order_dims = isl_bool_false;
	daptParams->dapt_unit_spacee_only = isl_bool_false;
	sprintf(daptParams->iteratortype, "int");

	if(is_param_set("--debug-print-on", argc, argv)){
		daptParams->debugPrintOn = isl_bool_true;
	}
	if(is_param_set("--help", argc, argv)){
		daptParams->help = isl_bool_true;
	}
	daptParams->filename = get_param_value("filename=", argc, argv);
	if(!daptParams->filename) daptParams->help = isl_bool_true;

	paramValue = get_param_value("iteratortype=", argc, argv);
	if(paramValue){
		sprintf(daptParams->iteratortype, "%s", paramValue);
	}

	paramValue = get_param_value("size=", argc, argv);
	if(paramValue){
		daptParams->size = atoi(paramValue);
	}

	paramValue = get_param_value("sizetimetile=", argc, argv);
	if(paramValue){
		daptParams->sizeTimeTile = atoi(paramValue);
	}



	paramValue = get_param_value("sizes=", argc, argv);
	if(paramValue){
		char buf[32];
		daptParams->sizesDim = atoi(paramValue);
		daptParams->sizes = (int*)malloc(daptParams->sizesDim*sizeof(int));
		for(int i=0; i < daptParams->sizesDim; i++ ){
			daptParams->sizes[i] = daptParams->size;
			sprintf(buf, "size%i=",i+1);
			paramValue = get_param_value(buf, argc, argv);
			if(paramValue){
				daptParams->sizes[i] = atoi(paramValue);
			}
		}
	}

	paramValue = get_param_value("method=", argc, argv);
	if(paramValue){
		daptParams->method = atoi(paramValue);
	}

	if(!(daptParams->method==1 || daptParams->method==2 || daptParams->method==3 || daptParams->method==4)) daptParams->help = isl_bool_true;

	if(is_param_set("--isl-schedule-on", argc, argv)){
		daptParams->isl_schedule_on = isl_bool_true;
	}
	if(is_param_set("--isl-schedule-show-code", argc, argv)){
			daptParams->isl_schedule_show_code = isl_bool_true;
	}
	if(is_param_set("--dapt-scop-split-off", argc, argv)){
		daptParams->dapt_scop_split = isl_bool_false;
	}
	if(is_param_set("--dapt-no-tiles", argc, argv)){
		daptParams->dapt_no_tiles = isl_bool_true;
	}
	if(is_param_set("--dapt-optimize-off", argc, argv)){
		daptParams->dapt_optimize_off = isl_bool_true;
	}
	if(is_param_set("--dapt-no-order-dims", argc, argv)){
		daptParams->dapt_no_order_dims = isl_bool_true;
	}

	if(is_param_set("--dapt-unit-spacee-only", argc, argv)){
		daptParams->dapt_unit_spacee_only = isl_bool_true;
	}
}
