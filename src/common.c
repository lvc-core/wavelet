#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <ctype.h>
#include <stdlib.h>

#include "common.h"


int error_message(const char* message){
    fprintf(stderr, RED "error: %s\n" RESET, message);
    return -1;
}


int file_error(
	const char* action,
	const char* path)
{
    fprintf(stderr,
	    ORANGE "error while %s '%s': %s\n" RESET,
	    action,
	    path,
	    strerror(errno));

    return -1;
}


bool checked_multiply(
	size_t a,
	size_t b,
	size_t* result)
{
    if (a != 0 && b > SIZE_MAX / a) {
	return false;
    }

    *result = a * b;
    return true;
}


bool parse_size(
	const char* text,
	size_t* output)
{
    while (isspace((unsigned char)*text)) {
	++text;
    }

    if (!isdigit((unsigned char)*text)) {
	return false;
    }

    char* end = NULL;

    errno = 0;
    unsigned long long value = strtoull(text, &end, 10);

    if (errno != 0 || value > SIZE_MAX) {
	return false;
    }

    while (isspace((unsigned char)*end)) {
	++end;
    }

    if (*end != '\0') {
	return false;
    }

    *output = (size_t)value;
    return true;
}
