#ifndef _EMB_INIT_DEFINED
#define _EMB_INIT_DEFINED

#include "emb_defs.h"
#include "emb_heap.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 *   Framework library initialization
 */

/*@@framework_init
 *
 * Initializes framework library
 *
 * Parameters:     none
 *
 * Return:         NULL           if failed
 *                 default heap   if successful
 *                 
 * 
 */
heap_ctx_t*
   framework_init(
      void);

/*@@framework_done
 *
 * Destroys framework library
 *
 * Parameters:     none
 *
 * Return:         true           if successful
 *                 false          if failed
 * 
 */
bool
   framework_done(
      void);


#ifdef __cplusplus
}
#endif


#endif

