/******************************************************************************
 *   Switching to target environment
 */

#if defined(WISE12)
#include "StdPxe.h"
#include "ApiLink.h"
#include "GrSys.h"
#endif 

#include "emb_defs.h"

#if defined(JAVA_ST_JLIB)

extern int64
   JLibOs_CurrentTimeMillis(
      void);

extern unsigned long
   JLibSys_GetTickCount(
      void);

#endif 

/******************************************************************************
 *  System/RTL functions, portable implementation
 */

/******************************************************************************/
clock_t
   portable_clock(
      void)
/*
 * clock() equivalent
 *
 */
{

#if defined(WIN32_APP) || defined(LINUX_APP) || defined(JAVA_MT_XMOA) ||      \
    defined(ANDROID)

   return clock();

#elif defined (WISE12)

   /* Obsolete
    * return (clock_t)System_GetTickCount();   
    *
    */
   return (clock_t)SysUtil_GetTickCount();   

#elif defined(JAVA_ST_JLIB)

   return (clock_t)JLibSys_GetTickCount();

#else
#error Not implemented yet 
#endif

}

/******************************************************************************/

#if !defined(LINUX_APP) && !defined(WIN32_APP) && !defined(JAVA_MT_XMOA) &&    \
    !defined(ANDROID)
static output_fn stdout_fn = NULL;
static output_fn stderr_fn = NULL;
#endif

int 
   portable_fprintf(
      FILE*         IN   f,
      const char*   IN   format,
      ...)
/*
 * fprintf() equivalent
 *
 */
{

   int i;
   va_list arglist;

   assert(format != NULL);

   va_start(arglist, format);

#if defined(WIN32_APP) || defined(LINUX_APP) || defined(JAVA_MT_XMOA) ||      \
    defined(ANDROID)
    
   i = vfprintf(f, format, arglist);

#else

#define MAX_IO_BUF 256

   {

      static char print_buf [MAX_IO_BUF];
      static char stdout_buf[MAX_IO_BUF];
      static uint stdout_cnt = 0;
      static char stderr_buf[MAX_IO_BUF];
      static uint stderr_cnt = 0;

      output_fn fn;

      char* p, *pe, *buf;
      uint* cnt;

      switch ((uint)f) {
      case (uint)Stdout:
         if (stdout_fn == NULL)
            return 0;
         fn  = stdout_fn;
         buf = stdout_buf; 
         cnt = &stdout_cnt;
         break;
      case (uint)Stderr:
         if (stderr_fn == NULL)
            return 0;
         fn  = stderr_fn; 
         buf = stderr_buf; 
         cnt = &stderr_cnt;
         break;
      default:
         ERR_SET_NO_RET(err_bad_param);
         return 0;
      }

      i = vsprintf(print_buf, format, arglist); 
      p = print_buf;
      do {
         pe = strchr(p, '\r');
         if (pe == NULL)
            pe = strchr(p, '\n');
         if (pe != NULL) {
            *pe = 0x00;
            if (*cnt == 0)
               (*fn)(f, p);
            else
               if (*cnt+strlen(p) >= MAX_IO_BUF) {
                  (*fn)(f, buf);
                  (*fn)(f, p);
                  *cnt = 0;
               }
               else {
                  strcat(buf, p);
                  (*fn)(f, buf);
                  *cnt = 0;
               }
            p = pe + 1;
         }
         else {
            if (*cnt+strlen(p) >= MAX_IO_BUF) {
               (*fn)(f, buf);
               *cnt = 0;
            }
            if (*cnt == 0)
               strcpy(buf, p);
            else
               strcat(buf, p);
            *cnt += strlen(p);
            p = pe;
         }
      } while (p != NULL);

   }

#undef MAX_IO_BUF 

#endif

   va_end(arglist);
   return i;

}

/******************************************************************************/
bool
   portable_redirect_output(
      FILE*       IN   dev,
      output_fn   IN   out)
/*
 * Sets output callback for device
 *
 */
{

#if defined(WIN32_APP) || defined(LINUX_APP) || defined(JAVA_MT_XMOA) ||      \
    defined(ANDROID)

   UNUSED(dev);
   UNUSED(out);
   ERR_SET(err_unexpected_call);

#else 

   switch ((uint)dev) {
   case (uint)Stdout:
      stdout_fn = out;
      break;
   case (uint)Stderr:
      stderr_fn = out;
      break;
   default:
      ERR_SET(err_bad_param);   
   }

   return true;

#endif

}

/******************************************************************************/
const char*
   portable_strnchr(
      const char*   IN   p,
      char          IN   c,
      ioffset       IN   n)
/*
 * strnchr() equivalent
 *
 */
{

   usize i;

   assert(p != NULL);
   
   if (n < 0)
      return StrChr(p, c);

   for (i=0; i<(usize)n; i++) { 
      if (p[i] == 0x00)
         break;  
      if (p[i] == c)
         return p+i;    
   }         
   return NULL;      

}

/******************************************************************************/
int 
   portable_strnicmp(
      const char*   IN   p1,
      const char*   IN   p2,
      ioffset       IN   n)
/*
 * strnicmp() equivalent
 *
 */
{

   char x1, x2;
   bool b1, b2;

   assert(p1 != NULL);
   assert(p2 != NULL);

   for (; (*p1!=0x00)&&(*p2!=0x00)&&(n!=0); p1++, p2++) {

      b1 = (*p1 > 'Z') && (*p1 < 'a');
      b2 = (*p2 > 'Z') && (*p2 < 'a');

      if ((*p1 >= 'A') && (*p1 <= 'Z')) 
         x1 = (char)((b2) ? *p1 : *p1 + 0x20);
      else 
         x1 = *p1;
      if ((*p2 >= 'A') && (*p2 <= 'Z')) 
         x2 = (char)((b1) ? *p2 : *p2 + 0x20);
      else 
         x2 = *p2;

      if (n > 0)
         n--;

      if (x1 == x2)
         continue;
      if (x1 < x2)
         return -1;
      else
         return 1;

   }

   if (n == 0)
      return 0;
   if (*p1 == *p2)
      return 0;
   if (*p1 < *p2)
      return -1;
   else
      return 1;
   
}

/******************************************************************************/
unumber
   portable_ticks_per_sec(
      void)
/*
 * Returns number of clock ticks per second
 *
 */
{

#if defined(WIN32_APP) || defined(LINUX_APP) || defined(JAVA_MT_XMOA) ||      \
    defined(ANDROID)

   return CLOCKS_PER_SEC;

#elif defined(OSREX)

   return 200;

#elif defined(JAVA_ST_JLIB)

   return 1000;

#elif defined(WISE12)

   return 1000;

#else
#error Not implemented yet
#endif

}

/******************************************************************************/
#ifndef EMB_NO_TIME_CALLS
time_t
   portable_time(
      void)
/*
 * time() equivalent
 *
 */
{

#if defined(WIN32_APP) || defined(LINUX_APP) || defined(JAVA_MT_XMOA) ||      \
    defined(ANDROID)

   return time(NULL);

#elif defined (WISE12)

   /* Obsolete
    * return (time_t)(PdkSys_GetLocalTime() + 0x386D4380);   
    *
    */
   return (time_t)(SysUtil_GetLocalTime() + 0x386D4380);   

#elif defined(JAVA_ST_JLIB)

   return (time_t)(JLibOs_CurrentTimeMillis()/1000);   

#else
#error Not implemented yet 
#endif

}
#endif

/******************************************************************************/
#ifndef EMB_NO_TIME_CALLS
void
   portable_time_ex(
      struct tm*   IN   tm_time)
/*
 * Returns current time in tm format
 *
 */
{

#if defined(WIN32_APP) || defined(LINUX_APP) || defined(ANDROID)
#define SIMPLE_IMPLEMENTATION
#endif
#if defined(JAVA_ST_JLIB) || defined(JAVA_MT_XMOA)
#define SIMPLE_IMPLEMENTATION
#endif

#if defined(SIMPLE_IMPLEMENTATION)

   struct tm* tm_tmp = NULL;
   time_t time_tmp = portable_time();

   assert(tm_time != NULL);

   MemSet(tm_time, 0x00, sizeof(tm_time));
   tm_tmp = gmtime(&time_tmp);
   if (tm_tmp != NULL) 
      MemCpy(tm_time, tm_tmp, sizeof(*tm_tmp));

#elif defined (WISE12)

   int w1, b1, b2, b3;

   assert(tm_time != NULL);
   MemSet(tm_time, 0x00, sizeof(*tm_time));

   /* Obsolete
    * System_GetDate(&w1, &b1, &b2, &b3);
    *
    */
   SysUtil_GetDate(&w1, &b1, &b2, &b3);
   tm_time->tm_year  = w1 - 1900;
   tm_time->tm_mon   = b1 - 1;
   tm_time->tm_mday  = b2;

   /* Obsolete
    * System_GetTime(&b1, &b2, &b3);
    *
    */
   SysUtil_GetTime(&b1, &b2, &b3);
   tm_time->tm_hour  = b1;
   tm_time->tm_min   = b2;
   tm_time->tm_sec   = b3;

#else
#error Not implemented yet 
#endif

#ifdef SIMPLE_IMPLEMENTATION
#undef SIMPLE_IMPLEMENTATION
#endif

}
#endif

/******************************************************************************/
#ifndef EMB_NO_FILE_CALLS
FILE*
   portable_fopen(
      const char*   IN   fname,
      const char*   IN   fmode)
/*
 * Performs file open 
 *
 */
{
#if defined(WIN32_APP) && defined(_MSC_VER) && (_MSC_VER > 1200)
   FILE* ret;
   if (fopen_s(&ret, fname, fmode) == 0) 
      return ret;
   return NULL;
#else 
   return fopen(fname, fmode);
#endif
}
#endif

/******************************************************************************/
#ifndef EMB_NO_FILE_CALLS
usize
   portable_fsize(
      FILE* f)
/*
 * Returns file size by its handle
 *
 */
{

   usize pos, ret; 

   assert(f != NULL);

   pos = Ftell(f);
   if (Fseek(f, 0, SEEK_END))
      return (usize)-1;     
   ret = Ftell(f);     
   if (Fseek(f, 0, SEEK_SET))
      return (usize)-1;     
   return ret;

}
#endif


/******************************************************************************
 *   Synchronization routines
 */

/*****************************************************************************/
uint32  
   portable_interlocked_exchange32(
      uint32*   IN OUT   shared,
      uint32    IN       exchange)
{

   assert(shared != NULL);

#if defined(WIN32_APP)

#if defined(PLATFORM_32BIT)
   return InterlockedExchange((LONG*)shared, exchange);
#else
#error Not implemented yet 
#endif

#elif defined(OSREX)

   __asm {
      swp exchange, exchange, [shared]
   }
   return exchange;

#elif defined(LINUX_APP) 

   uint32 u;
   do {
      u = *shared;
   } while (__sync_val_compare_and_swap(shared, u, exchange) != u);
   return u;

   /*
    * return xchg_u32(shared, exchange);
    *
    */

   /* 
    *  {
    *     uint32 t;
    *     t       = *shared;
    *     *shared = exchange;
    *     return t;
    *  }
    *
    */

#elif defined(WISE12) || defined(JAVA_ST_JLIB) || defined(JAVA_MT_XMOA)

   /* no sync */
   {
      uint32 t;
      t       = *shared;
      *shared = exchange;
      return t;
   }

#elif defined(ANDROID)

   asm volatile (
      "swp %[ex], %[ex], [%[sh]]" : 
      [ex] "+r" (exchange) : 
      [sh]  "r" (shared));
   return exchange;

   /* Does not work since NDK v7
    * 
    * return __atomic_swap(exchange, shared);
    *
    */
   
#else
#error Not implemented yet 
#endif

}

/*****************************************************************************/
void
   portable_mutex_create(
      generic_mutex_t*   IN OUT   pmux)
/*
 * Initializes a mutex object
 *
 */
{

   assert(pmux != NULL);

#if defined(WIN32_APP)
   pmux->sync = CreateMutexA(NULL, FALSE, NULL);
#elif defined(OSREX)
   rex_init_crit_sect(&pmux->sync);
#elif defined(LINUX_APP) || defined(JAVA_MT_XMOA) || defined(ANDROID)
   pthread_mutex_init(&pmux->sync, NULL);
#elif defined(JAVA_ST_JLIB) || defined(WISE12)
   /* do nothing */
#else
#error Not implemented yet 
#endif

}

/*****************************************************************************/
void
   portable_mutex_destroy(
      generic_mutex_t*   IN OUT   pmux)
/*
 * Releases a mutex object
 *
 */
{

   assert(pmux != NULL);

#if defined(WIN32_APP)
   CloseHandle(pmux->sync);
#elif defined(LINUX_APP) || defined(JAVA_MT_XMOA) || defined(ANDROID)
   pthread_mutex_destroy(&pmux->sync);
#elif defined(OSREX) || defined(WISE12) || defined(JAVA_ST_JLIB)
   /* do nothing */
#else
#error Not implemented yet 
#endif

}

/*****************************************************************************/
void
   portable_mutex_lock(
      generic_mutex_t*   IN OUT   pmux)
/*
 * Locks a mutex object
 *
 */
{

   assert(pmux != NULL);

#if defined(WIN32_APP)
   WaitForSingleObject(pmux->sync, INFINITE);
#elif defined(OSREX)
   rex_enter_crit_sect(&pmux->sync);
#elif defined(LINUX_APP) || defined(JAVA_MT_XMOA) || defined(ANDROID)
   pthread_mutex_lock(&pmux->sync);
#elif defined(JAVA_ST_JLIB) || defined (WISE12)
   /* do nothing */
#else
#error Not implemented yet 
#endif

}

/*****************************************************************************/
void
   portable_mutex_unlock(
      generic_mutex_t*   IN OUT   pmux)
/*
 * Unlocks a mutex object
 *
 */
{

   assert(pmux != NULL);

#if defined(WIN32_APP)
   ReleaseMutex(pmux->sync);
#elif defined(OSREX)
   rex_leave_crit_sect(&pmux->sync);
#elif defined(LINUX_APP) || defined(JAVA_MT_XMOA) || defined(ANDROID)
   pthread_mutex_unlock(&pmux->sync);
#elif defined(JAVA_ST_JLIB) || defined (WISE12)
   /* do nothing */
#else
#error Not implemented yet 
#endif

}

#ifdef USE_DEF_APP_HEAP

/*
 * Default application heap size
 *
 */
#ifndef DEF_APP_HEAP_SIZE
#define DEF_APP_HEAP_SIZE  65536
#endif

/*****************************************************************************/
bool
   portable_get_app_heap_params(
      byte**   OUT   ppbuf,
      usize*   OUT   psize) 
/*
 * Returns requested parameters of application heap 
 *
 */
{

   /*
    * Internal application heap
    *
    */
   static byte _app_heap_buf[DEF_APP_HEAP_SIZE];

   assert(ppbuf != NULL);
   assert(psize != NULL);

   *psize = sizeof(_app_heap_buf);
   *ppbuf = _app_heap_buf;
   return true;

}

#endif /* ifdef USE_DEF_APP_HEAP */

