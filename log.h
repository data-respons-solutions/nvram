#ifndef LOG_H_
#define LOG_H_

#include <stdio.h>

#define pr_dbg(fmt, ...) \
		print_debug("dbg: " fmt, ##__VA_ARGS__);

#define pr_err(fmt, ...) \
		fprintf(stderr, "error: " fmt, ##__VA_ARGS__);

void enable_debug(void);
void print_debug(const char* fmt, ...);

#endif // LOG_H_
