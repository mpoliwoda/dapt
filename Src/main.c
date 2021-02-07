/*
 ============================================================================
 Name        : DAPT
 Author      :
 Version     :
 Copyright   :
 Description : Dependence Approximation for Parallelism and Tiling
 ============================================================================
 */

#include <stdlib.h>

#include <isl/ctx.h>

#include <pet.h>

#include "tool_string.h"
#include "tool_isl.h"
#include "loop_scop.h"
#include "norm_scop.h"
#include "loop_tile.h"
#include "tool_debug.h"
#include "dapt_params.h"
#include "codegen.h"

int main(int argc, char *argv[]) {
	isl_ctx *ctx = 0;
	pet_scop *petScop = 0;
	dapt_params daptParams;

	read_dapt_params(&daptParams, argc, argv);

	if(daptParams.help == isl_bool_true){
		char tabs[8];
		sprintf(tabs,"\t\t\t");

		printf("parameters\n");
		printf("\t%s%s%s\n", "filename=<path>      ", tabs, "file with loop to parse");
		printf("\t%s%s%s\n", "method=<int>         ", tabs, "method for tiles build");
		printf("\t%s%s%s\n", "                     ", tabs, "1 - parallel synchronization free (lexmax) then wafefront");
		printf("\t%s%s%s\n", "                     ", tabs, "2 - wafefront");
		printf("\t%s%s%s\n", "                     ", tabs, "3 - parallel startup");
		printf("\t%s%s%s\n", "                     ", tabs, "4 - parallel synchronization free (lexmin) then wafefront");
		printf("\t%s%s%s\n", "[size=<int>]         ", tabs, "default size (default:16)");
		printf("\t%s%s%s\n", "[sizes=<int>]        ", tabs, "number of spaces with specified size (default:0)");
		printf("\t%s%s%s\n", "[size<int>=<int>]    ", tabs, "specifies size list for spaces, when sizes is set (default:empty list)");

		printf("options\n");
		printf("\t%s%s%s\n", "--isl-schedule-on         ", tabs, "use isl schedule map for loop normalization");
		printf("\t%s%s%s\n", "--isl-schedule-show-code  ", tabs, "show code for isl schedule");

		printf("\t%s%s%s\n", "--dapt-scop-split-off     ", tabs, "dont split loops in scop");
		printf("\t%s%s%s\n", "--dapt-no-tiles           ", tabs, "only loop parse");
		printf("\t%s%s%s\n", "--dapt-optimize-off       ", tabs, "turns off loop optimization");
		printf("\t%s%s%s\n", "--dapt-no-order-dims      ", tabs, "turns off order dims from hyperplanes");

		printf("\t%s%s%s\n", "--debug-print-on          ", tabs, "print debug info to stderr");

		printf("\t%s%s%s\n", "--help                    ", tabs, "print this help, then exit");

		return EXIT_SUCCESS;
	}

	if(daptParams.debugPrintOn == isl_bool_true){
		isl_debug_on();
	}

	ctx = isl_ctx_alloc_with_pet_options();
	pet_options_set_signed_overflow(ctx, PET_OVERFLOW_IGNORE);

	petScop = pet_scop_extract_from_C_source(ctx, daptParams.filename , NULL);
	if(petScop != 0){
		loop_scop *loopScop = loop_scop_extract_from_pet_scop(petScop, &daptParams);

		if(daptParams.isl_schedule_show_code == isl_bool_true){
			isl_union_map *schedule = isl_union_map_intersect_domain(isl_union_map_copy(loopScop->loopIslMap), isl_union_set_copy(loopScop->loopDomain));
			isl_debug_printf("\n#%s\n", "######################################################################");
			isl_debug_printf("\n#%s\n", "isl schedule code:");
			isl_debug_printf_str("%s", codegen_wavefront_to_str(schedule, petScop, 0, isl_bool_false));
			isl_debug_printf("\n#%s\n", "######################################################################");

			isl_union_map_free(schedule);
		}

		loop_scop_list *loopScopList = normalize_calc_scop(loopScop);

		if(daptParams.dapt_no_tiles == isl_bool_false){
			loop_tile_list *loopTileList =  loop_tile_list_from_loop_scop_list(loopScopList);

			isl_printf_str("\n//dapt code:\n%s", codegen_macros_to_str(loopTileList->wafefrontTileSchedule, petScop));

			for(int i=0; i<loopTileList->count; i++){
				isl_printf_str("%s", codegen_wavefront_to_str(loopTileList->loopsTile[i]->wafefrontTileSchedule, petScop, loopTileList->loopsTile[i]->iterators, isl_bool_true));
			}

			loopTileList = loop_tile_list_free(loopTileList);
		}

		loopScopList = loop_scop_list_free(loopScopList);
		loopScop = loop_scop_loop_scop_free(loopScop);
	}

	pet_scop_free(petScop);
	isl_ctx_free(ctx);

	return EXIT_SUCCESS;
}
