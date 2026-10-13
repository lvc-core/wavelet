#ifndef COMMON_H
#define COMMON_H

#include <stddef.h>
#include <stdbool.h>


#define RED 	"\033[31m"
#define GREEN	"\033[32m"
#define YELLOW	"\033[33m"
#define	BLUE	"\033[34m"
#define ORANGE	"\033[1m\033[33m"
#define RESET	"\033[0m"

int error_message(const char* message);

int file_error(
	const char* action,
	const char* path
);

bool checked_multiply(
	size_t a,
	size_t b,
	size_t* result
);

bool parse_size(
	const char* text,
	size_t* output
);


#endif // COMMON_H
