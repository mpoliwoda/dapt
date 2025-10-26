/*
 * tool_string.h
 *
 *  Created on: May 14, 2020
 *      Author: maciej
 */

#ifndef TOOL_STRING_H_
#define TOOL_STRING_H_

#if defined(__cplusplus)
extern "C" {
#endif

char* str_replace(char* string, const char* substr, const char* replacement);
char* str_substring(const char* string, const char* start, const char* end);
int str_count(char* string, const char* substr);
char* str_concatenation(char* toExp, const char* toAdd);
char* get_param_value(char* paramName, int argc, char *argv[]);
int is_param_set(char* paramName, int argc, char *argv[]);

#if defined(__cplusplus)
}
#endif


#endif /* TOOL_STRING_H_ */
