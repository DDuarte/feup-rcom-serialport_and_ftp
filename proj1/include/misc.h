#ifndef MISC_H_
#define MISC_H_

#include <stdint.h>

/*! \file misc.h
    \brief Miscellaneous functions and macros

     Definition of functions and macros that are used across this project
*/

#define DEBUG(msg)                //fprintf(stderr, "DEBUG: %s\n", msg)
#define DEBUGF(msg, args...)      //fprintf(stderr, "DEBUG: " msg "\n", args)
#define ERROR(msg)                fprintf(stderr, "ERROR: %s\n", msg)
#define ERRORF(msg, args...)      fprintf(stderr, "ERROR: " msg "\n", args)
#define DEBUG_LINE(msg, args...)  //fprintf(stderr, "DEBUG: " msg " (%s, %d)\n", args, __FUNCTION__, __LINE__)

//#define DEBUG_CALLS // only enable when sh*t goes really wrong

#ifdef DEBUG_CALLS
    #define LOG printf("DEBUG: %s, %s, %d\n", __FILE__, __FUNCTION__, __LINE__);
#else
    #define LOG ((void)0);
#endif

typedef union int32
{
   char b[4];
   int32_t w;
} int32;

#endif // MISC_H_
