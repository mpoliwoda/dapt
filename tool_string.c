/*
 * str_func.c
 *
 *  Created on: 8 sie 2018
 *
 */

#include "tool_string.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char* str_replace(char* string, const char* substr, const char* replacement) {
	char* tok = NULL;
	char* newstr = NULL;
	char* oldstr = NULL;
	int   oldstr_len = 0;
	int   substr_len = 0;
	int   replacement_len = 0;
	int   startpos = 0;

	newstr = strdup(string);
	substr_len = strlen(substr);
	replacement_len = strlen(replacement);

	if (substr == NULL || replacement == NULL) {
		return newstr;
	}

	while ((tok = strstr(newstr + startpos, substr))) {
		oldstr = newstr;
		oldstr_len = strlen(oldstr);
		newstr = (char*)malloc(sizeof(char) * (oldstr_len - substr_len + replacement_len + 1));

		if (newstr == NULL) {
			free(oldstr);
			return NULL;
		}

		memcpy(newstr, oldstr, tok - oldstr);
		memcpy(newstr + (tok - oldstr), replacement, replacement_len);
		memcpy(newstr + (tok - oldstr) + replacement_len, tok + substr_len, oldstr_len - substr_len - (tok - oldstr));
		memset(newstr + oldstr_len - substr_len + replacement_len, 0, 1);

		startpos = (tok - oldstr) + replacement_len;
		free(oldstr);
	}

	free(string);

	return newstr;
}

char* str_substring(const char* string, const char* start, const char* end){
	char* returnStr = NULL;
	char* tokenStart = strstr(string,start);
	char* tokenEnd = NULL;

	if(tokenStart != NULL){
		tokenStart= tokenStart + strlen(start);
		tokenEnd = strstr(tokenStart,end);
	}

	if(tokenEnd != NULL && tokenStart != NULL){
		returnStr = malloc(sizeof(char)*((tokenEnd-tokenStart)+1));
		strncpy(returnStr,tokenStart,(tokenEnd-tokenStart));
		returnStr[tokenEnd-tokenStart] = '\0';
	}
	else{
		returnStr = malloc(sizeof(char));
		returnStr[0] = '\0';
	}

	return returnStr;
}

int str_count(char* string, const char* substr){
	int count = 0;

	char* tokenStart = strstr(string,substr);

	while(tokenStart != NULL){
		count++;
		tokenStart= tokenStart + strlen(substr);
		tokenStart = strstr(tokenStart,substr);
	}

	return count;
}

char* str_concatenation(char* toExp, const char* toAdd){
	char* dest = malloc(sizeof(char)* (strlen(toExp) + strlen(toAdd) + 1));

	sprintf(dest,"%s",toExp);
	sprintf(&dest[strlen(dest)],"%s",toAdd);

	free(toExp);

	return dest;
}

char* get_param_value(char* paramName, int argc, char *argv[]){
	char* paramValue = 0;
	int i;

	for(i=1; i < argc; i++){
		if(strstr(argv[i], paramName)){
			paramValue = &argv[i][strlen(paramName)];
			break;
		}
	}

	return paramValue;
}

int is_param_set(char* paramName, int argc, char *argv[]){
	int paramValue = 0;
	int i;

	for(i=1; i < argc; i++){
		if(strstr(argv[i], paramName)){
			paramValue = -1;
			break;
		}
	}

	return paramValue;
}
