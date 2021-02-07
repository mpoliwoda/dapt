/*
 * codegen.c
 *
 *  Created on: May 28, 2020
 *
 */

#include "codegen.h"
#include <string.h>
#include <isl/space.h>
#include <isl/id.h>

typedef struct codegen_ast_stmt_annotation
{
    struct pet_stmt* stmt;

    isl_id_to_ast_expr* id2expr;
} codegen_ast_stmt_annotation;

typedef struct omp_Pragma_Info{
	isl_bool addPragmaParall;
	isl_bool isInParallelRegion;

	int levelInParallelLevel;
} omp_Pragma_Info;

static __isl_give isl_ast_node* codegen_wavefront_visitor_at_each_domain(__isl_take isl_ast_node* node, __isl_keep isl_ast_build* build, void* user);
static struct pet_stmt* codegen_wavefront_get_pet_stmt(pet_scop* pet, const char* label);
static __isl_give isl_multi_pw_aff* codegen_wavefront_visitor_build_ast_exprs_index_callback(__isl_take isl_multi_pw_aff* index, __isl_keep isl_id* id, void* user);
static __isl_give isl_printer* codegen_wavefront_print_user(__isl_take isl_printer* printer, __isl_take isl_ast_print_options* options, __isl_keep isl_ast_node* node, void* user);
static __isl_give isl_printer* codegen_wavefront_print_for(__isl_take isl_printer* printer, __isl_take isl_ast_print_options* ast_options, __isl_keep isl_ast_node* node, void* user);
static codegen_ast_stmt_annotation* codegen_ast_stmt_annotation_alloc();
static void codegen_ast_stmt_annotation_free(void* user);

char* codegen_to_str(__isl_keep isl_union_map *scheduleMap){
	char *codeStr = 0;
	isl_ast_node *ast_tree = 0;
	isl_ctx *ctx = isl_union_map_get_ctx(scheduleMap);

	isl_options_set_ast_always_print_block(ctx, 1);

	{
		isl_ast_build *ast_build = isl_ast_build_from_context(isl_set_universe(isl_space_params(isl_union_map_get_space(scheduleMap))));

		ast_tree = isl_ast_build_ast_from_schedule(ast_build, isl_union_map_copy(scheduleMap));

		isl_ast_build_free(ast_build);
	}

	{
		isl_printer *printer = isl_printer_to_str(ctx);
		int format= isl_printer_get_output_format(printer);

		printer = isl_printer_set_output_format(printer, ISL_FORMAT_C);
		printer = isl_printer_print_ast_node(printer, ast_tree);
		printer = isl_printer_set_output_format(printer, format);

		codeStr = isl_printer_get_str(printer);

		isl_printer_free(printer);
	}

	return codeStr;
}

char* codegen_macros_to_str(__isl_keep isl_union_map *scheduleMap, __isl_keep pet_scop *pet){
	char *codeStr = 0;
	isl_ctx *ctx = isl_union_map_get_ctx(scheduleMap);
	isl_printer *printer = isl_printer_to_str(ctx);
	isl_ast_node *ast_tree = 0;

	{
		isl_ast_build *ast_build = 0;

		ast_build = isl_ast_build_from_context(isl_set_copy(pet->context));
		ast_build = isl_ast_build_set_at_each_domain(ast_build, &codegen_wavefront_visitor_at_each_domain, pet);

		ast_tree = isl_ast_build_ast_from_schedule(ast_build, isl_union_map_copy(scheduleMap));

		isl_ast_build_free(ast_build);
	}

	printer = isl_ast_node_print_macros(ast_tree, printer);
	printer = isl_printer_start_line(printer);
	printer = isl_printer_end_line(printer);


	codeStr = isl_printer_get_str(printer);

	isl_printer_free(printer);
	isl_ast_node_free(ast_tree);

	return codeStr;
}

char* codegen_wavefront_to_str(__isl_keep isl_union_map *scheduleMap, __isl_keep pet_scop *pet, isl_id_list* iterators, isl_bool isParallel){
	char *codeStr = 0;
	isl_ast_node *ast_tree = 0;
	isl_ctx *ctx = isl_union_map_get_ctx(scheduleMap);
	omp_Pragma_Info ompPragmaInfo;

	ompPragmaInfo.addPragmaParall = isParallel;
	ompPragmaInfo.isInParallelRegion = isl_bool_false;
	ompPragmaInfo.levelInParallelLevel = 0;

	isl_options_set_ast_always_print_block(ctx, 1);

	{
		isl_ast_build *ast_build = 0;

		ast_build = isl_ast_build_from_context(isl_set_copy(pet->context));
		if(iterators!=0){
			ast_build = isl_ast_build_set_iterators(ast_build, isl_id_list_copy(iterators));
		}
		ast_build = isl_ast_build_set_at_each_domain(ast_build, &codegen_wavefront_visitor_at_each_domain, pet);

		ast_tree = isl_ast_build_ast_from_schedule(ast_build, isl_union_map_copy(scheduleMap));

		isl_ast_build_free(ast_build);
	}

	{
		isl_ast_print_options* ast_options = isl_ast_print_options_alloc(ctx);
		isl_printer *printer = isl_printer_to_str(ctx);
		int format= isl_printer_get_output_format(printer);

		ast_options = isl_ast_print_options_set_print_for(ast_options, &codegen_wavefront_print_for, &ompPragmaInfo);
		ast_options = isl_ast_print_options_set_print_user(ast_options, &codegen_wavefront_print_user, 0);

		printer = isl_printer_set_output_format(printer, ISL_FORMAT_C);

		printer = isl_ast_node_print(ast_tree, printer, ast_options);

		printer = isl_printer_set_output_format(printer, format);

		codeStr = isl_printer_get_str(printer);

		isl_printer_free(printer);
	}
	isl_ast_node_free(ast_tree);

	return codeStr;
}

static __isl_give isl_ast_node* codegen_wavefront_visitor_at_each_domain(__isl_take isl_ast_node* node, __isl_keep isl_ast_build* build, void* user)
{
	pet_scop* pet = (pet_scop*)user;

	codegen_ast_stmt_annotation* annotation = codegen_ast_stmt_annotation_alloc();

    isl_ctx* ctx = isl_ast_node_get_ctx(node);

    isl_ast_expr* expr = isl_ast_node_user_get_expr(node);

    isl_ast_expr* arg = isl_ast_expr_get_op_arg(expr, 0);

    isl_id* id = isl_ast_expr_get_id(arg);

    annotation->stmt = codegen_wavefront_get_pet_stmt(pet, isl_id_get_name(id));

    isl_map* map = isl_map_from_union_map(isl_ast_build_get_schedule(build));

    map = isl_map_reverse(map);

    isl_pw_multi_aff* iterator_map = isl_pw_multi_aff_from_map(map);

    annotation->id2expr = pet_stmt_build_ast_exprs(annotation->stmt, build, &codegen_wavefront_visitor_build_ast_exprs_index_callback, iterator_map, NULL, NULL);

    isl_pw_multi_aff_free(iterator_map);

    isl_id* annotation_id = isl_id_alloc(ctx, NULL, annotation);

    annotation_id = isl_id_set_free_user(annotation_id, &codegen_ast_stmt_annotation_free);

    isl_id_free(id);
    isl_ast_expr_free(expr);
    isl_ast_expr_free(arg);

    node = isl_ast_node_set_annotation(node, annotation_id);

    return node;
}

static struct pet_stmt* codegen_wavefront_get_pet_stmt(pet_scop* pet, const char* label)
{
    struct pet_stmt* value = NULL;

    for (int i = 0; i < pet->n_stmt; ++i)
    {
        struct pet_stmt* stmt = pet->stmts[i];

        const char* stmt_label = isl_set_get_tuple_name(stmt->domain);

        if (NULL != stmt_label && 0 == strcmp(label, stmt_label))
        {
            value = stmt;
            break;
        }
    }

    return value;
}

static __isl_give isl_multi_pw_aff* codegen_wavefront_visitor_build_ast_exprs_index_callback(__isl_take isl_multi_pw_aff* index, __isl_keep isl_id* id, void* user)
{
    isl_pw_multi_aff* iterator_map = (isl_pw_multi_aff*)user;

    iterator_map = isl_pw_multi_aff_copy(iterator_map);

    return isl_multi_pw_aff_pullback_pw_multi_aff(index, iterator_map);
}

static __isl_give isl_printer* codegen_wavefront_print_user(__isl_take isl_printer* printer, __isl_take isl_ast_print_options* options, __isl_keep isl_ast_node* node, void* user)
{
	isl_id* annotation_id = isl_ast_node_get_annotation(node);
	codegen_ast_stmt_annotation* annotation = (codegen_ast_stmt_annotation*)isl_id_get_user(annotation_id);

	printer = pet_stmt_print_body(annotation->stmt, printer, annotation->id2expr);

	isl_ast_print_options_free(options);
	isl_id_free(annotation_id);

	return printer;
}

static __isl_give isl_printer* codegen_wavefront_print_for(__isl_take isl_printer* printer, __isl_take isl_ast_print_options* ast_options, __isl_keep isl_ast_node* node, void* user){
	isl_ast_node* body = isl_ast_node_for_get_body(node);
	omp_Pragma_Info* ompPragmaInfo = ((omp_Pragma_Info*)user);
	isl_bool isParallelLevel = isl_bool_false;
	isl_bool isHyperplaneLevel = isl_bool_false;

	{
		isl_ast_expr* expr = isl_ast_node_for_get_iterator(node);
		isl_ast_expr* init = isl_ast_node_for_get_init(node);
		isl_ast_expr* cond = isl_ast_node_for_get_cond(node);
		isl_ast_expr* inc = isl_ast_node_for_get_inc(node);
		isl_id *id = isl_ast_expr_get_id(expr);
		const char *name = isl_id_get_name(id);
		const char *type = isl_options_get_ast_iterator_type(isl_printer_get_ctx(printer));

		if(ompPragmaInfo->isInParallelRegion == isl_bool_true && strstr(name, "h")!=0){
			ompPragmaInfo->levelInParallelLevel += 1;
			isHyperplaneLevel = isl_bool_true;
		}

		if(strstr(name, "w0")!=0 ){
			ompPragmaInfo->isInParallelRegion = isl_bool_true;
			isParallelLevel = isl_bool_true;
		}

		if(strstr(name, "ph")!=0 ){
			ompPragmaInfo->isInParallelRegion = isl_bool_true;
			ompPragmaInfo->levelInParallelLevel += 1;
			isHyperplaneLevel = isl_bool_true;
			isParallelLevel = isl_bool_true;
		}

		if(ompPragmaInfo->levelInParallelLevel == 1 && ompPragmaInfo->addPragmaParall == isl_bool_true && isHyperplaneLevel == isl_bool_true){
			printer = isl_printer_start_line(printer);
			printer = isl_printer_print_str(printer, "#pragma omp parallel for");
			printer = isl_printer_end_line(printer);
		}
		printer = isl_printer_start_line(printer);
		printer = isl_printer_print_str(printer, "for (");
		printer = isl_printer_print_str(printer, type);
		printer = isl_printer_print_str(printer, " ");
		printer = isl_printer_print_str(printer, name);
		printer = isl_printer_print_str(printer, " = ");
		printer = isl_printer_print_ast_expr(printer, init);
		printer = isl_printer_print_str(printer, "; ");
		printer = isl_printer_print_ast_expr(printer, cond);
		printer = isl_printer_print_str(printer, "; ");
		printer = isl_printer_print_str(printer, name);
		printer = isl_printer_print_str(printer, " += ");
		printer = isl_printer_print_ast_expr(printer, inc);
		printer = isl_printer_print_str(printer, ")");
		printer = isl_printer_print_str(printer, " {");
		printer = isl_printer_end_line(printer);

	    isl_id_free(id);
	    isl_ast_expr_free(inc);
	    isl_ast_expr_free(cond);
	    isl_ast_expr_free(init);
	    isl_ast_expr_free(expr);
	}

	printer = isl_printer_indent(printer, 2);

	if (isl_ast_node_get_type(body) == isl_ast_node_block){
		isl_ast_node_list* block_statements = isl_ast_node_block_get_children(body);

		for (int i = 0; i < isl_ast_node_list_n_ast_node(block_statements); ++i)
		{
			isl_ast_node* block_statement = isl_ast_node_list_get_ast_node(block_statements, i);

			printer = isl_ast_node_print(block_statement, printer, isl_ast_print_options_copy(ast_options));

			isl_ast_node_free(block_statement);
		}

		isl_ast_node_list_free(block_statements);
	}
	else{
		printer = isl_ast_node_print(body, printer, isl_ast_print_options_copy(ast_options));
	}

	printer = isl_printer_indent(printer, -2);

	if(isParallelLevel == isl_bool_true){
		ompPragmaInfo->isInParallelRegion = isl_bool_false;
		isParallelLevel = isl_bool_false;
		if(isHyperplaneLevel == isl_bool_true){
			ompPragmaInfo->levelInParallelLevel -= 1;
		}
	}

	if(ompPragmaInfo->isInParallelRegion == isl_bool_true && isHyperplaneLevel == isl_bool_true){
		ompPragmaInfo->levelInParallelLevel -= 1;
	}

    printer = isl_printer_start_line(printer);
    printer = isl_printer_print_str(printer, "}");
    printer = isl_printer_end_line(printer);

	isl_ast_node_free(body);
	isl_ast_print_options_free(ast_options);
	return printer;
}

static codegen_ast_stmt_annotation* codegen_ast_stmt_annotation_alloc()
{
	codegen_ast_stmt_annotation* annotation = (codegen_ast_stmt_annotation*)malloc(sizeof(codegen_ast_stmt_annotation));

    annotation->stmt = NULL;
    annotation->id2expr = NULL;

    return annotation;
}

static void codegen_ast_stmt_annotation_free(void* user)
{
    if (NULL == user)
        return;

    codegen_ast_stmt_annotation* annotation = (codegen_ast_stmt_annotation*)user;

    isl_id_to_ast_expr_free(annotation->id2expr);

    free(annotation);
}
