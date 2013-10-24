#ifndef MISC_H_
#define MISC_H_

#include <stdint.h>
#include <stdbool.h>

/*! \file misc.h
    \brief Miscellaneous functions and macros

     Definition of functions and macros that are used across this project
*/

#define DEBUG(msg)                //fprintf(stdout, "DEBUG: %s\n", msg)
#define DEBUGF(msg, args...)      //fprintf(stderr, "DEBUG: " msg "\n", args)
#define ERROR(msg)                fprintf(stderr, "ERROR: %s\n", msg)
#define ERRORF(msg, args...)      fprintf(stderr, "ERROR: " msg "\n", args)
#define WARNING(msg)              fprintf(stderr, "WARNING: %s\n", msg)
#define WARNINGF(msg, args...)    fprintf(stderr, "WARNING: " msg "\n", args)
#define PRINT(msg)                fprintf(stdout, "PRINT: %s\n", msg)
#define PRINTF(msg, args...)      fprintf(stdout, "PRINT: " msg "\n", args)
#define DEBUG_LINE(msg, args...)  //fprintf(stderr, "DEBUG: " msg " (%s, %d)\n", args, __FUNCTION__, __LINE__)

void set_verbose_output(bool v);
int print_message(const char* fmt, ...);

//#define DEBUG_CALLS // only enable when sh*t goes really wrong

#ifdef DEBUG_CALLS
    #define LOG printf("DEBUG: %s, %s, %d\n", __FILE__, __FUNCTION__, __LINE__);
#else
    #define LOG
#endif

typedef union int32
{
   char b[4];
   int32_t w;
} int32;

#endif // MISC_H_
