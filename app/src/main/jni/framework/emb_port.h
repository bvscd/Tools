#ifndef _EMB_PORT_DEFINED
#define _EMB_PORT_DEFINED

#ifdef __cplusplus
extern "C" {
#endif

#define MEM_LOW                             /* Compile for low memory env    */
#if 1
#define USE_DEBUG                           /* Compile with debug features   */
#endif

#if defined(WIN32_APP) || defined(LINUX_APP) 
#define USE_MALLOC
#endif

/******************************************************************************
 *   Target environment
 */
                                                  
/*
 * Macro switch for compiling as a part of native Linux application
 * May be defined automatically
 *
 * #define __linux__
 *
 */

/*
 * Macro switch for compiling as a part of native Cygwin/Linux application
 * May be defined automatically
 *
 * #define __CYGWIN__
 *
 */

/*
 * Macro switch for compiling as a part of native Win32 application
 * May be defined automatically
 *
 * #define _WIN32
 *
 */

/*
 * Macro switch for compiling as a part of Google Android application
 * May be defined automatically
 *
 * #define ANDROID
 *
 */
 
/*
 * Macro switch for compiling as a part of REX RTOS kernel
 *
 * #define OSREX
 *
 */

/*
 * Macro switch for compiling as a part of WISE v1.2 module
 *
 * #define WISE12
 *
 */

/*
 * Macro switch for compiling as a part of singlethreaded Java VM
 * Abstraction layer is JLib (used for handsets) 
 *
 * #define JAVA_ST_JLIB
 *
 */

/*
 * Macro switch for compiling as a part of multithreaded Java VM
 * Abstraction layer is XMOA (used for DTV devices)
 *
 * #define JAVA_MT_XMOA
 *
 */

/*
 * Check non-automatic targets
 *
 */

#if defined(OSREX)
#if defined(_TARGET_SET)
#error Conflict in compilation options
#endif
#define _TARGET_SET
#endif

#if defined(WISE12)
#if defined(_TARGET_SET)
#error Conflict in compilation options
#endif
#define _TARGET_SET
#endif

#if defined(JAVA_ST_JLIB)
#if defined(_TARGET_SET)
#error Conflict in compilation options
#endif
#define _TARGET_SET
#endif

#if defined(JAVA_MT_XMOA)
#if defined(_TARGET_SET)
#error Conflict in compilation options
#endif
#define _TARGET_SET
#endif

#if defined(ANDROID)
#if defined(_TARGET_SET)
#error Conflict in compilation options
#endif
#define _TARGET_SET
#endif

/*
 * Check automatic targets if target was not set yet
 *
 */

#if !defined(_TARGET_SET)

#if !defined(__linux__) && !defined(_WIN32) && !defined(__CYGWIN__)
#error Target environment is unknown
#endif
#if defined(__linux__) 
#if defined(__CYGWIN__) || defined(_WIN32) 
#error Conflict in compilation options
#endif
#else
#if defined(__CYGWIN__) && defined(_WIN32) 
#error Conflict in compilation options
#endif
#endif

#if defined(__linux__) || defined(__CYGWIN__)
#define LINUX_APP
#endif
#if defined(_WIN32)
#define WIN32_APP
#endif

#else
#undef _TARGET_SET
#endif

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable: 4001) /* C++ comments warning */
#endif
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#if defined(_MSC_VER) && ((_MSC_VER <= 1200) || (_MSC_VER >= 1600))
#include <stddef.h>
#endif
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

/*
 * Macro switches for MS Visual C++/i386 CPU
 *
 * #define _MSC_VER
 * #define _M_IX86
 *
 */

/*
 * Macro switches for MS Visual C++/x64 CPU
 *
 * #define _MSC_VER
 * #define _M_X64
 *
 */

/*
 * Macro switches for ARMCC/ARM CPU
 *
 * #define __ARMCC_VERSION
 * #define __TARGET_CPU_ARM926EJ_S - for ARM926EJ/S
 *
 */

/*
 * Macro switches for GCC/Vincent CPU
 *
 * #define __GNUC__
 * #define __VINCENT__
 *
 */

/*
 * Macro switches for GCC/MIPS-32 CPU
 *
 * #define __GNUC__
 * #define __MIPS32__
 *
 */

/*
 * Macro switches for GCC/ARM architecture v5
 *
 * #define __GNUC__
 * #define __ARM_ARCH_5__
 *
 */

/*
 * Macro switches for GCC/i386 CPU
 *
 * #define __GNUC__
 * #define __i386__
 *
 */

/*
 * Macro switches for GCC/x64 CPU
 *
 * #define __GNUC__
 * #define __x86_64__
 *
 */

#if defined(_MSC_VER) && defined(_M_IX86)
#define ENDIAN_LITTLE
#define PLATFORM_32BIT
#define ADDRESS_32BIT
#elif defined(_MSC_VER) && defined(_M_X64)
#define ENDIAN_LITTLE
#define PLATFORM_32BIT
#define ADDRESS_64BIT
#elif defined(__ARMCC_VERSION) && defined(__TARGET_CPU_ARM926EJ_S)
#define ENDIAN_LITTLE
#define PLATFORM_32BIT
#define ADDRESS_32BIT
#elif defined(__GNUC__) && defined(__VINCENT__)
#define ENDIAN_BIG
#define PLATFORM_32BIT
#define ADDRESS_32BIT
#elif defined(__GNUC__) && defined(__MIPS32__)
#define ENDIAN_LITTLE
#define PLATFORM_32BIT
#define ADDRESS_32BIT
#elif defined(__GNUC__) && defined(__arm__)
#define ENDIAN_LITTLE
#define PLATFORM_32BIT
#define ADDRESS_32BIT
#elif defined(__GNUC__) && defined(__aarch64__)
#define ENDIAN_LITTLE
#define PLATFORM_64BIT
#define ADDRESS_64BIT
#elif defined(__GNUC__) && defined(__i386__)
#define ENDIAN_LITTLE
#define PLATFORM_32BIT
#define ADDRESS_32BIT
#elif defined(__GNUC__) && defined(__x86_64__)
#define ENDIAN_LITTLE
#define PLATFORM_64BIT
#define ADDRESS_64BIT
#else
#error Unknown platform
#endif

/*
 * Other possible CPU options 
 *
 */
/* #define PLATFORM_64BIT */
/* #define ADDRESS_64BIT  */

#if defined(WIN32_APP) && !defined(WINVER)
#include "win_hack.h"
#endif

#if defined(OSREX)
#include <rex.h>
#include <comdef.h>
#endif

#if defined(LINUX_APP) || defined(ANDROID)
#include <pthread.h>
#endif


/******************************************************************************
 *   General types
 */

#ifdef WISE12
#ifdef true
#undef true
#endif
#ifdef false
#undef false
#endif
#ifdef _PAL_TYPEDEF_H_
#define uint32 ccuint32
#define int32  ccint32
#endif
#ifdef INLINE
#undef INLINE
#endif
#endif

#if defined(__GNUC__)
#include <stddef.h>
#include <stdint.h>
#endif
#if defined(__ARMCC_VERSION)
#include <stdint.h>
#endif

typedef unsigned char  byte;                /* byte value                    */
typedef unsigned int   unumber;             /* integer number                */
typedef unsigned int   umask;               /* bit mask value                */
#if defined(_MSC_VER) && (_MSC_VER <= 1200)
typedef int            ioffset;             /* offset value                  */
#else
typedef ptrdiff_t      ioffset;             /* offset value                  */
#endif
typedef size_t         usize;               /* size value                    */

#ifndef __CYGWIN__
typedef unsigned short ushort;              /* unsigned short integer value  */
#endif

#ifndef __cplusplus
typedef enum _bool {                        /* boolean value                 */
   true  = 1,
   false = 0
} bool;
#endif

#if defined(_MSC_VER)

typedef __int8            int8;             /* 8-bit integer value           */ 
typedef __int16           int16;            /* 16-bit integer value          */ 
typedef __int32           int32;            /* 32-bit integer value          */  
typedef __int64           int64;            /* 64-bit integer value          */   
typedef unsigned __int8   uint8;            /* 8-bit unsigned integer value  */ 
typedef unsigned __int16  uint16;           /* 16-bit unsigned integer value */ 
typedef unsigned __int32  uint32;           /* 32-bit unsigned integer value */ 
typedef unsigned __int64  uint64;           /* 64-bit unsigned integer value */ 

#define PORTABLE_W64(x)                                                       \
           x##ui64           

#elif defined(__ARMCC_VERSION) && defined(OSREX)

/*
 * All above types are defined in REX RTOS system headers
 *
 */

#define PORTABLE_W64(x)                                                       \
           x##LL

#elif defined(__GNUC__)

typedef int8_t           int8;              /* 8-bit integer value           */
typedef int16_t          int16;             /* 16-bit integer value          */
typedef int32_t          int32;             /* 32-bit integer value          */
typedef int64_t          int64;             /* 64-bit integer value          */
typedef uint8_t          uint8;             /* 8-bit unsigned integer value  */
typedef uint16_t         uint16;            /* 16-bit unsigned integer value */
typedef uint32_t         uint32;            /* 32-bit unsigned integer value */
typedef uint64_t         uint64;            /* 64-bit unsigned integer value */

#ifdef PLATFORM_64BIT
typedef __int128_t       int128;            /* 128-bit integer value         */
typedef __uint128_t      uint128;           /* 128-bit unsigned integer value*/
#endif

#define PORTABLE_W64(x)                                                       \
           x##LL

#elif defined(__ARMCC_VERSION)

typedef int8_t           int8;              /* 8-bit integer value           */
typedef int16_t          int16;             /* 16-bit integer value          */
typedef int32_t          int32;             /* 32-bit integer value          */
typedef int64_t          int64;             /* 64-bit integer value          */
typedef uint8_t          uint8;             /* 8-bit unsigned integer value  */
typedef uint16_t         uint16;            /* 16-bit unsigned integer value */
typedef uint32_t         uint32;            /* 32-bit unsigned integer value */
typedef uint64_t         uint64;            /* 64-bit unsigned integer value */

#define PORTABLE_W64(x)                                                       \
           x##LL

#else
#error Unknown compiler
#endif

typedef uint16 unicode;                     /* Unicode character             */ 
typedef void*  handle;                      /* Generic handle                */

#if defined(_MSC_VER) || defined(__ARMCC_VERSION)
#define PORTABLE_INLINE extern __inline
#elif defined(__GNUC__)
#define PORTABLE_INLINE static __inline
#else
#error Unknown compiler
#endif

#ifndef UNUSED 
#if defined(__GNUC__) 
#define UNUSED(x) x = x 
#elif defined(__ARMCC_VERSION) 
#define UNUSED(x)
#else 
#define UNUSED(x) x 
#endif
#endif 


/******************************************************************************
 *   General system/RTL calls 
 */

#define PORTABLE_MEMCMP(pd, ps, c)                                            \
           ( memcmp((pd), (ps), (c)) )
#define PORTABLE_MEMCPY(pd, ps, c)                                            \
           ( memcpy((pd), (ps), (c)) )
#define PORTABLE_MEMMOVE(p, b, c)                                             \
           ( memmove((p), (b), (c)) )
#define PORTABLE_MEMSET(p, b, c)                                              \
           ( memset((p), (b), (c)) )
#define PORTABLE_RAND()                                                       \
           ( rand() )

#if defined(_MSC_VER) 
#define PORTABLE_SPRINTF   sprintf
#elif defined(WIN32_APP) || defined(LINUX_APP)
#define PORTABLE_SPRINTF(_s, _c, ...)                                         \
           ( sprintf_s((_s), (_c), __VA_ARGS__) )
#elif defined(WISE12) || defined(ANDROID)
#define PORTABLE_SPRINTF(_s, _c, ...)                                         \
           ( sprintf((_s), __VA_ARGS__) ) 
#else
#error Not implemented
#endif

#define PORTABLE_SRAND(s)                                                     \
           ( srand((s)) )
#define PORTABLE_STRCHR(p, ch)                                                \
           ( strchr((p), (ch)) )
#define PORTABLE_STRNCHR(p, ch, n)                                            \
           ( portable_strnchr((p), (ch), (n)) )
#define PORTABLE_STRICMP(p1, p2)                                              \
           ( portable_strnicmp((p1), (p2),  -1) )
#define PORTABLE_STRLEN(p)                                                    \
           ( strlen((p)) )
#define PORTABLE_STRCMP(p1, p2)                                                \
           ( strcmp((p1), (p2)) )
#define PORTABLE_STRNICMP(p1, p2, n)                                          \
           ( portable_strnicmp((p1), (p2), (n)) )


/******************************************************************************
 *   Time-related calls 
 */

#ifndef EMB_NO_TIME_CALLS

#if defined(LINUX_APP) || defined(WIN32_APP) || defined(ANDROID)
#define PORTABLE_TICKS_PER_SEC                                                \
           ( CLOCKS_PER_SEC )
#else
#define TICKS_PER_SEC                                                         \
           ( portable_ticks_per_sec() )
#endif

#if defined(LINUX_APP) || defined(WIN32_APP) || defined(ANDROID)
#define PORTABLE_CLOCK()                                                      \
           ( clock() )
#else
#define PORTABLE_CLOCK()                                                      \
           ( portable_clock() )
#endif

#if defined(LINUX_APP) || defined(WIN32_APP) || defined(ANDROID)
#define PORTABLE_MKTIME(t)                                                    \
           ( mktime((t)) )
#else
#error Not implemented
#endif

#if defined(LINUX_APP) || defined(WIN32_APP) || defined(ANDROID)
#define PORTABLE_TIME(t)                                                      \
           ( time((t)) )
#else
#define PORTABLE_TIME(t)                                                      \
           ( portable_time() )
#endif

#define PORTABLE_TIME_EX(tm)                                                  \
           ( portable_time_ex((tm)) )

#endif /* EMB_NO_TIME_CALLS */


/******************************************************************************
 *   I/O support
 */

#if defined(LINUX_APP) || defined(WIN32_APP) || defined(JAVA_MT_XMOA) ||      \
    defined(ANDROID)
#define PORTABLE_STDOUT stdout
#define PORTABLE_STDERR stderr
#define PORTABLE_STDNUL NULL
#else
#define PORTABLE_STDOUT (FILE*)1
#define PORTABLE_STDERR (FILE*)2
#define PORTABLE_STDNUL NULL
#endif

#ifndef EMB_NO_FILE_CALLS

#if defined(WIN32_APP)                                                        
#define PORTABLE_FOPEN(fname, fmode)                                          \
           ( portable_fopen((fname), (fmode)) )
#elif defined(LINUX_APP) || defined(JAVA_MT_XMOA) || defined(ANDROID)         
#define PORTABLE_FOPEN(fname, fmode)                                          \
           ( fopen((fname), (fmode)) )
#else
#error Not implemented
#endif

#if defined(LINUX_APP) || defined(WIN32_APP) || defined(JAVA_MT_XMOA) ||      \
    defined(ANDROID)
#define PORTABLE_FGETS(buf, cnt, f)                                           \
           ( fgets((buf), (cnt), (f)) )
#else
#error Not implemented
#endif

#define PORTABLE_FSIZE(f)                                                     \
           ( portable_fsize((f)) )

#if defined(LINUX_APP) || defined(WIN32_APP) || defined(JAVA_MT_XMOA) ||      \
    defined(ANDROID)
#define PORTABLE_FSEEK(f, ofs, org)                                           \
           ( fseek((f), (ofs), (org)) )
#else
#error Not implemented
#endif

#if defined(LINUX_APP) || defined(WIN32_APP) || defined(JAVA_MT_XMOA) ||      \
    defined(ANDROID)
#define PORTABLE_FTELL(f)                                                     \
           ( ftell((f)) )
#else
#error Not implemented
#endif

#if defined(LINUX_APP) || defined(WIN32_APP) || defined(JAVA_MT_XMOA) ||      \
    defined(ANDROID)
#define PORTABLE_FREAD(buf, item, items, f)                                   \
           ( fread((buf), (item), (items), (f)) )
#else
#error Not implemented
#endif

#if defined(LINUX_APP) || defined(WIN32_APP) || defined(JAVA_MT_XMOA) ||      \
    defined(ANDROID)
#define PORTABLE_FWRITE(buf, item, items, f)                                  \
           ( fwrite((buf), (item), (items), (f)) )
#else
#error Not implemented
#endif

#if defined(LINUX_APP) || defined(WIN32_APP) || defined(JAVA_MT_XMOA) ||      \
    defined(ANDROID)
#define PORTABLE_FCLOSE(f)                                                    \
           ( fclose((f)) )
#else
#error Not implemented
#endif

#endif /* EMB_NO_FILE_CALLS */

#if defined(_MSC_VER)
#define PORTABLE_FPRINTF   fprintf
#elif defined(LINUX_APP) || defined(WIN32_APP) || defined(JAVA_MT_XMOA) ||    \
    defined(ANDROID)
#define PORTABLE_FPRINTF(f, ...)                                              \
           ( fprintf((f), __VA_ARGS__) )
#else
#define PORTABLE_FPRINTF(f, ...)                                              \
           ( portable_fprintf((f), __VA_ARGS__) )
#endif


#if defined(LINUX_APP) || defined(WIN32_APP) || defined(JAVA_MT_XMOA) ||      \
    defined(ANDROID)
#define PORTABLE_FFLUSH(f)                                                    \
           ( fflush((f)) )
#else
#error Not implemented
#endif


/******************************************************************************
 *   Memory allocation support
 */

#ifdef USE_MALLOC
#define PORTABLE_MALLOC(c)                                                    \
           ( malloc((c)) )
#define PORTABLE_FREE(p)                                                      \
           ( free((p)) )
#endif

/******************************************************************************
 *   Synchronization
 */

/*
 * Generic mutex object
 *
 */
typedef struct portable_mutex_s {
#if defined(WIN32_APP)
   HANDLE               sync;                    /* sync object              */
#elif defined(OSREX)
   rex_crit_sect_type   sync;
#elif defined(LINUX_APP) || defined(JAVA_MT_XMOA) || defined(ANDROID)
   pthread_mutex_t      sync;
#elif defined(JAVA_ST_JLIB) || defined(WISE12)
   byte                 sync;
#endif
} portable_mutex_t;

#if defined(WIN32_APP)
#define PORTABLE_DELAY(x)                                                     \
           Sleep((x))
#else
#define PORTABLE_DELAY(x)                                                              
#endif

#define PORTABLE_INTERLOCKED_EXCHANGE32(shared, exchange)                     \
           ( portable_interlocked_exchange32((shared), (exchange)) )
#define PORTABLE_MUTEX_CREATE(pmux)                                           \
           portable_mutex_create((pmux))
#define PORTABLE_MUTEX_DESTROY(pmux)                                          \
           portable_mutex_destroy((pmux))
#define PORTABLE_MUTEX_LOCK(pmux)                                             \
           portable_mutex_lock((pmux))
#define PORTABLE_MUTEX_UNLOCK(pmux)                                           \
           portable_mutex_unlock((pmux))


/******************************************************************************
 *  System/RTL functions, portable implementations
 */

#define IN                                  /* Mark INPUT function params    */
#define OUT                                 /* Mark OUTPUT function params   */

#ifndef EMB_NO_TIME_CALLS
/*@@portable_clock
 *
 * clock() equivalent
 *
 * Parameters:     none
 *
 * Return:         clock value in clock() notation
 *
 */
clock_t
   portable_clock(
      void);
#endif /* EMB_NO_TIME_CALLS */

/*@@portable_fprintf
 *
 * fprintf() equivalent
 *
 * Parameters:     f              file 
 *                 format         format string, ASCIIZ
 *
 * Return:         fprintf return value
 *
 */
int 
   portable_fprintf(
      FILE*         IN   f,
      const char*   IN   format,
      ...);

/*@@portable_interlocked_exchange32
 *
 * Performs 32-bit interlocked_exchange primitive
 *
 * Parameters:     shared         points to shared variable
 *                 exchange       value to exchange
 *
 * Return:         old value of shared variable
 *
 */
uint32  
   portable_interlocked_exchange32(
      uint32*   IN OUT   shared,
      uint32    IN       exchange);

/*@@portable_mutex_create
 *
 * Initializes a mutex object
 *
 * Parameters:     pmux           pointer to a mutex object
 *
 * Return:         none
 *
 */
void
   portable_mutex_create(
      portable_mutex_t*   IN OUT   pmux);

/*@@portable_mutex_destroy
 *
 * Releases a mutex object
 *
 * Parameters:     pmux           pointer to a mutex object
 *
 * Return:         none
 *
 */
void
   portable_mutex_destroy(
      portable_mutex_t*   IN OUT   pmux);

/*@@portable_mutex_lock
 *
 * Locks a mutex object
 *
 * Parameters:     pmux           pointer to a mutex object
 *
 * Return:         none
 *
 */
void
   portable_mutex_lock(
      portable_mutex_t*   IN OUT   pmux);

/*@@portable_mutex_unlock
 *
 * Unlocks a mutex object
 *
 * Parameters:     pmux           pointer to a mutex object
 *
 * Return:         none
 *
 */
void
   portable_mutex_unlock(
      portable_mutex_t*   IN OUT   pmux);

/*@@output_fn
 *
 * Output redirection hook function
 *
 */
typedef void
   (*output_fn)(
       FILE*       dev,
       const char* str);

/*@@portable_redirect_output
 *
 * Sets output redirection hook for device
 *
 * Parameters:     dev            device pointer
 *                 out            output callback
 *
 * Return:         true           if successful
 *                 false          if failed
 *
 */
bool
   portable_redirect_output(
      FILE*       IN   dev,
      output_fn   IN   out);

/*@@portable_strnchr
 *
 * strnchr() equivalent
 *
 * Parameters:     p              string
 *                 c              character
 *                 n              string length, <0 if zero-terminated  
 *
 * Return:         character position pointer or NULL if not found
 *
 */
const char*
   portable_strnchr(
      const char*   IN   p,
      char          IN   c,
      ioffset       IN   n);

/*@@portable_strnicmp
 *
 * strnicmp() equivalent
 *
 * Parameters:     p1             first string
 *                 p2             second string
 *                 n              string length, <0 if zero-terminated  
 *
 * Return:         <0             if p1 < p2
 *                 =0             if p1 = p2
 *                 >0             if p1 > p2
 *
 */
int 
   portable_strnicmp(
      const char*   IN   p1,
      const char*   IN   p2,
      ioffset       IN   n);

#ifndef EMB_NO_TIME_CALLS

/*@@portable_ticks_per_sec
 *
 * Returns number of clock ticks per second
 *
 * Parameters:     none
 *
 * Return:         number of clock ticks per second
 *
 */
unumber
   portable_ticks_per_sec(
      void);

/*@@portable_time
 *
 * time() equivalent
 *
 * Parameters:     none
 *
 * Return:         time value in time() notation
 *
 */
time_t
   portable_time(
      void);

/*@@portable_time_ex
 *
 * Returns current time in tm format
 *
 * Parameters:     tm_time        variable to fill in
 *
 * Return:         none
 *
 */
void
   portable_time_ex(
      struct tm*   IN   tm_time);

#endif /* EMB_NO_TIME_CALLS */

#ifndef EMB_NO_FILE_CALLS

/*@@portable_fopen
 *
 * Performs file open 
 *
 * Parameters:     fname          file name
 *                 fmode          file mode
 *
 * Return:         file handle
 *
 */
FILE*
   portable_fopen(
      const char*   IN   fname,
      const char*   IN   fmode);

/*@@portable_fsize
 *
 * Returns file size by its handle
 *
 * Parameters:     f              file handle
 *
 * Return:         size
 *
 */
usize
   portable_fsize(
         FILE*   IN   f);

#endif /* EMB_NO_FILE_CALLS */

/*@@portable_get_app_heap_params
 *
 * Returns requested parameters of application heap 
 *
 * Parameters:     ppbuf          heap buffer
 *                 psize          heap size, bytes
 *
 * Return:         true           if application heap is required
 *                 false          if is not required
 *
 */
bool
   portable_get_app_heap_params(
      byte**   OUT   ppbuf,
      usize*   OUT   psize);


#ifdef __cplusplus
}
#endif


#endif

