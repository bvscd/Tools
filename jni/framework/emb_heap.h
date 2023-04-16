#ifndef _EMB_HEAP_DEFINED
#define _EMB_HEAP_DEFINED

#include "emb_defs.h"
#include "emb_list.h"
                
#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 *   Heap API
 */

#if 1
#define USE_POOL
#define USE_MCL
#endif

/*
 * Heap flags
 *
 */
typedef enum heap_flag_e {
   heap_no_trace   = 0x01,                  /* No trace output               */
   heap_tracking   = 0x02,                  /* Tracking in progress          */
   heap_use_malloc = 0x04,                  /* Use malloc() if needed        */
} heap_flag_t;

/*
 * Heap context, supports heap in externally allocated buffer
 *
 */
typedef struct heap_ctx_s {
   generic_mutex_t   mutex;                 /* Sync mutex                    */
   usize             cbuf;                  /* Attached buffer size, units   */
   void*             pbuf;                  /* Attached buffer               */
#ifdef USE_MALLOC
   list_entry_t      exts;                  /* Allocation extensions         */
#endif
#ifdef USE_POOL
   list_entry_t      pools;                 /* List of chunk pools           */
#endif
#ifdef USE_DEBUG
   usize             alloc;                 /* Allocation count              */
   usize             max_alloc;             /* Peak allocation count         */
#endif
   umask             flags;                 /* See heap_flag_t               */
   umask             cleanup;               /* Internal use                  */
} heap_ctx_t;

/*@@heap_create
 *
 * Creates heap context
 *
 * Parameters:     pbuf           buffer to attach to heap
 *                 cbuf           above-mentioned buffer length, bytes
 *                 flags          heap flags, reserved
 *                 ctx            heap context
 *
 * Return:         true           if successful,
 *                 false          if failed
 *
 */
bool
   heap_create(
      void*         IN       pbuf,
      usize         IN       cbuf,
      umask         IN       flags,
      heap_ctx_t*   IN OUT   ctx);

/*@@heap_destroy
 *
 * Destroys heap context
 *
 * Parameters:     ctx            heap context
 *
 * Return:         true           if successful,
 *                 false          if failed
 *
 */
bool
   heap_destroy(
      heap_ctx_t*   IN OUT   ctx);

/*@@heap_alloc
 *
 * Allocates memory from heap 
 *
 * C/C++ Syntax:   
 * bool 
 *    heap_alloc(                                                
 *       void**        OUT      ppbuf,                     
 *       uint          IN       cbuf,                      
 *       heap_ctx_t*   IN OUT   ctx);
 *
 * Parameters:     ppbuf          pointer to allocated buffer pointer
 *                 cbuf           requested buffer length, bytes
 *                 ctx            heap context
 *
 * Return:         true           if successful,
 *                 false          if failed
 * 
 */
#ifdef USE_DEBUG

bool
   heap_alloc_helper(
      void**        OUT      ppbuf,
      usize         IN       cbuf,
      const char*   IN       file, 
      unumber       IN       line, 
      heap_ctx_t*   IN OUT   ctx);

#define /* bool */ heap_alloc(                                                \
                      /* void**        OUT    */   ppbuf,                     \
                      /* usize         IN     */   cbuf,                      \
                      /* heap_ctx_t*   IN OUT */   ctx)                       \
       ( heap_alloc_helper((ppbuf), (cbuf), __FILE__, __LINE__, (ctx)) )

#else /* ifdef USE_DEBUG */

bool
   heap_alloc_helper(
      void**        OUT      ppbuf,
      usize         IN       cbuf,
      heap_ctx_t*   IN OUT   ctx);

#define /* bool */ heap_alloc(                                                \
                      /* void**        OUT    */   ppbuf,                     \
                      /* usize         IN     */   cbuf,                      \
                      /* heap_ctx_t*   IN OUT */   ctx )                      \
       ( heap_alloc_helper((ppbuf), (cbuf), (ctx)) )                   

#endif /* ifdef USE_DEBUG */

/*@@heap_alloc_in_range
 *
 * Allocates amount of memory in specificed range from heap 
 *
 * C/C++ Syntax:   
 * bool 
 *    heap_alloc_in_range(                                                
 *       void**        OUT      ppbuf,                     
 *       uint*         OUT      calloc,                      
 *       uint          IN       cmin,                      
 *       uint          IN       cmax,                      
 *       heap_ctx_t*   IN OUT   ctx);
 *
 * Parameters:     ppbuf          pointer to allocated buffer pointer
 *                 calloc         allocated size, bytes
 *                 cmin           minimal required size, bytes
 *                 cmax           maximal required size, bytes
 *                 ctx            heap context
 *
 * Return:         true           if successful,
 *                 false          if failed
 * 
 */
#ifdef USE_DEBUG

bool
   heap_alloc_in_range_helper(
      void**        OUT      ppbuf,
      usize*        OUT      calloc,
      usize         IN       cmin,
      usize         IN       cmax,
      const char*   IN       file, 
      unumber       IN       line, 
      heap_ctx_t*   IN OUT   ctx);

#define /* bool */ heap_alloc_in_range(                                       \
                      /* void**        OUT    */   ppbuf,                     \
                      /* usize*        OUT    */   calloc,                    \
                      /* usize         IN     */   cmin,                      \
                      /* usize         IN     */   cmax,                      \
                      /* heap_ctx_t*   IN OUT */   ctx)                       \
       (                                                                      \
          heap_alloc_in_range_helper(                                         \
             (ppbuf),                                                         \
             (calloc),                                                        \
             (cmin),                                                          \
             (cmax),                                                          \
             __FILE__,                                                        \
             __LINE__,                                                        \
             (ctx))                                                           \
       )

#else /* ifdef USE_DEBUG */

bool
   heap_alloc_in_range_helper(
      void**        OUT      ppbuf,
      usize*        OUT      calloc,
      usize         IN       cmin,
      usize         IN       cmax,
      heap_ctx_t*   IN OUT   ctx);

#define /* bool */ heap_alloc_in_range(                                       \
                      /* void**        OUT    */   ppbuf,                     \
                      /* usize*        OUT    */   calloc,                    \
                      /* usize         IN     */   cmin,                      \
                      /* usize         IN     */   cmax,                      \
                      /* heap_ctx_t*   IN OUT */   ctx )                      \
       ( heap_alloc_in_range_helper((ppbuf), (calloc), (cmin), (cmax), (ctx)) )                   

#endif /* ifdef USE_DEBUG */

#ifdef USE_POOL
/*@@heap_alloc_from_pool
 *
 * Allocates memory from pool of fixed size chunks  
 *
 * Parameters:     ppbuf          pointer to allocated buffer pointer
 *                 cbuf           requested buffer length, bytes
 *                 qty            estimation of max items in pool
 *                 ctx            heap context
 *
 * Return:         true           if successful,
 *                 false          if failed
 * 
 */
bool
   heap_alloc_from_pool(
      void**        OUT      ppbuf,
      usize         IN       cbuf,
      usize         IN       qty,
      heap_ctx_t*   IN OUT   ctx);
#endif /* ifdef USE_POOL */

/*@@heap_free
 *
 * Returns memory to heap 
 *
 * Parameters:     pbuf           buffer to return to heap
 *                 ctx            heap context
 *
 * Return:         true           if successful,
 *                 false          if failed
 * 
 */
bool
   heap_free(
      void*         IN       pbuf,
      heap_ctx_t*   IN OUT   ctx);

#ifdef USE_DEBUG
/*@@heap_clear_statistics
 *
 * Clears heap statistics
 *
 * C/C++ Syntax:   
 * void 
 *    heap_clear_statistics(                                                
 *       heap_ctx_t*   IN OUT   ctx);
 *
 * Parameters:     ctx            heap context
 *
 * Return:         none
 * 
 */
#define /* void */ heap_clear_statistics(                                     \
                      /* heap_ctx_t*   IN OUT */   ctx)                       \
       {                                                                      \
          assert((ctx) != NULL);                                              \
          (ctx)->alloc = (ctx)->max_alloc = 0;                                \
       }
#endif /* ifdef USE_DEBUG */

/*@@heap_exceptions_on
 *
 * Turns on no_memory exceptions
 *
 * C/C++ Syntax:   
 * void 
 *    heap_exceptions_on(                                                
 *       heap_ctx_t*   IN OUT   ctx);
 *
 * Parameters:     ctx            heap context
 *
 * Return:         none
 * 
 */
#define /* void */ heap_exceptions_on(                                        \
                      /* heap_ctx_t*   IN OUT */   ctx)                       \
       {                                                                      \
          assert((ctx) != NULL);                                              \
          (ctx)->flags &= ~heap_no_trace;                                     \
       }

/*@@heap_exceptions_off
 *
 * Turns off no_memory exceptions
 *
 * C/C++ Syntax:   
 * void 
 *    heap_exceptions_off(                                                
 *       heap_ctx_t*   IN OUT   ctx);
 *
 * Parameters:     ctx            heap context
 *
 * Return:         none
 * 
 */
#define /* void */ heap_exceptions_off(                                       \
                      /* heap_ctx_t*   IN OUT */   ctx)                       \
       {                                                                      \
          assert((ctx) != NULL);                                              \
          (ctx)->flags |= heap_no_trace;                                      \
       }
                                                                              
/*@@heap_exceptions_are_on
 *
 * Checks whether no_memory exceptions are on
 *
 * C/C++ Syntax:   
 * bool 
 *    heap_exceptions_are_on(                                                
 *       heap_ctx_t*   IN OUT   ctx);
 *
 * Parameters:     ctx            heap context
 *
 * Return:         true           if exceptions are on
 *                 false          if exceptions are off
 * 
 */
#define /* bool */ heap_exceptions_are_on(                                    \
                      /* heap_ctx_t*   IN OUT */   ctx)                       \
       (                                                                      \
          assert((ctx) != NULL),                                              \
          ((ctx)->flags & heap_no_trace) ? false : true                       \
       )

/*@@heap_stats
 *
 * Returns heap statistics
 *
 * Parameters:     pfree          free bytes storage
 *                 pbusy          used bytes storage
 *                 out            output device
 *                 ctx            heap context
 *
 * Return:         true           if successful,
 *                 false          if failed
 * 
 */
bool
   heap_stats(
      usize*        OUT      pfree,
      usize*        OUT      pbusy,
      FILE*         IN       out, 
      heap_ctx_t*   IN OUT   ctx);

/*@@heap_get_unit
 *
 * Returns heap unit size
 *
 * Parameters:     ctx            heap context
 *
 * Return:         unit size, bytes
 * 
 */
usize
   heap_get_unit(
      heap_ctx_t* ctx);

/*@@heap_is_pointer_within
 *
 * Quick check of pointer validity
 *
 * Parameters:     ptr            pointer to check
 *                 ctx            heap context
 *
 * Return:         true           if pointer is from heap
 *                 false          if pointer is external
 * 
 */
bool
   heap_is_pointer_within(
      void*               IN   ptr,
      const heap_ctx_t*   IN   ctx);


#ifdef __cplusplus
}
#endif


#endif

