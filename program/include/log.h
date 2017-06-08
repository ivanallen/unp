#ifndef __LOG_H__
#define __LOG_H__


#define PRINT(COLOR, format,...) do { \
	fprintf(stderr, "\x1b[K"); \
	fprintf(stderr, COLOR); \
	fprintf(stderr, format, ##__VA_ARGS__);\
	fprintf(stderr, "\x1b[0m");\
} while(0)

#define ERR_EXIT(msg) do { \
	fprintf(stderr, "\x1b[K\x1b[31m[%s:%d] ", __FILE__, __LINE__); \
	perror(msg); \
	fprintf(stderr, "\x1b[0m");\
	exit(1); \
} while(0)

#define ERR_QUIT(format,...) do { \
	fprintf(stderr, "\x1b[K\x1b[31m[%s:%d] ", __FILE__, __LINE__); \
	fprintf(stderr, format, ##__VA_ARGS__); \
	fprintf(stderr, "\x1b[0m");\
	exit(1); } \
while(0)

#define LOG_RED "\x1b[31m"
#define LOG_GREEN "\x1b[32m"
#define LOG_YELLOW "\x1b[33m"

// red info
#define ERR_PRINT(format,...) PRINT(LOG_RED, format, ##__VA_ARGS__) 

// green info
#define LOG(format,...) PRINT(LOG_GREEN, format, ##__VA_ARGS__)

// yellow info
#define WARNING(format,...) PRINT(LOG_YELLOW, format, ##__VA_ARGS__)


// cursor
#define CURSOR_UP(n) fprintf(stderr, "\x1b[%dA", n)
#define CURSOR_DOWN(n) fprintf(stderr, "\x1b[%dB", n)

#define CURSOR_ON() fprintf(stderr, "\x1b[?25h")
#define CURSOR_OFF() fprintf(stderr, "\x1b[?25l")

#define CURSOR_SAVE() fprintf(stderr, "\x1b[s")
#define CURSOR_RESTORE() fprintf(stderr, "\x1b[u")

#define CURSOR_POS(y, x) fprintf(stderr, "\x1b[%d;%dH", y, x)

#define RESET() fprintf(stderr, "\x1b[0m")

#define CLEAR() fprintf(stderr, "\x1b[2J")

#endif // __LOG_H__
