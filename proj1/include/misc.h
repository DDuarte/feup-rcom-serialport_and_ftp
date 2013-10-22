#ifndef MISC_H_
#define MISC_H_

/*! \file misc.h
    \brief Miscellaneous functions and macros

     Definition of functions and macros that are used across this project
*/

#define DEBUG(msg)          //printf("DEBUG: %s\n", (msg))
#define ERROR(msg)          //fprintf(stderr, "ERROR: %s\n", (msg))
#define DEBUG_LINE()        //printf("DEBUG: %s, %d\n", __FUNCTION__, __LINE__)
#define DEBUG_LINE_MSG(msg) //printf("DEBUG: %s (%s, %d)\n", (msg), __FUNCTION__, __LINE__)

//#define DEBUG_CALLS // only enable when sh*t goes really wrong

#ifdef DEBUG_CALLS
    #define LOG printf("DEBUG: %s, %s, %d\n", __FILE__, __FUNCTION__, __LINE__);
#else
    #define LOG ((void)0);
#endif

typedef union int16
{
   char b[2];
   short w;
} int16;

#endif // MISC_H_
