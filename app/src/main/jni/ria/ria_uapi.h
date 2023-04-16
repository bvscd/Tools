#ifndef _RIA_UAPI
#define _RIA_UAPI


#include "emb_defs.h"
#include "ria_exec.h"


/*
 * RIA handle
 *
 */
typedef handle ria_handle_t;

/*@@ria_uapi_init
 *
 * Creates RIA engine
 *
 * Parameters:     tempdir          path to temporary directory
 *
 * Return:         engine handle or 0 if error
 *
 */
ria_handle_t
   ria_uapi_init(
      const char*   IN   tempdir);
   
/*@@ria_uapi_shutdown
 *
 * Destroy RIA engine
 *
 * Parameters:     engine           engine handle
 *
 * Return:         true             if successful
 *                 false            if failed
 *
 */
bool
   ria_uapi_shutdown(
      ria_handle_t   IN   engine);     
      
/*@@ria_uapi_error_msg
 *
 * Returns last cached error message
 *
 * Parameters:     engine         engine handle
 *
 * Return:         message text
 *
 */
const char*
   ria_uapi_error_msg(
      ria_handle_t   IN   engine);

/*@@ria_uapi_alloc
 *
 * Allocates memory from internal RIA heap 
 *
 * Parameters:     ptr              pointer to allocated 
 *                 size             size to allocate
 *
 * Return:         true             if successful
 *                 false            if failed
 *
 */
bool
   ria_uapi_alloc(
      void**   OUT   ptr,
      usize    IN    size);     

/*@@ria_uapi_free
 *
 * Frees memory into internal RIA heap 
 *
 * Parameters:     ptr              pointer to free
 *
 * Return:         true             if successful
 *                 false            if failed
 *
 */
bool
   ria_uapi_free(
      void*   IN   ptr);
      
/*@@ria_uapi_load
 *
 * Loads scenario script
 *
 * Parameters:     path           path to script
 *                 engine         engine handle
 *
 * Return:         true           if successful
 *                 false          if failed
 *
 */
bool
   ria_uapi_load(     
      const char*    IN   path,
      ria_handle_t   IN   engine);
     
/*@@ria_uapi_execute
 *
 * Executes script
 *
 * Parameters:     status         execution status
 *                 presult        buffer with result
 *                 cresult        buffer size on entry, result size on exit
 *                 name           script name
 *                 pparams        parameters array (nul-terminated strings)
 *                 cparams        number of parameters
 *                 engine         engine handle
 *
 * Return:         true           if successful
 *                 false          if failed
 *
 */
bool
   ria_uapi_execute( 
      ria_exec_status_t*   OUT      status,    
      char*                IN OUT   presult,
      usize*               IN OUT   cresult,
      const char*          IN       name,
      const char**         IN       pparams,
      usize                IN       cparams,
      ria_handle_t         IN       engine);
     
/*@@ria_uapi_continue
 *
 * Continues execution of pending script
 *
 * Parameters:     status         execution status
 *                 presult        buffer with result
 *                 cresult        buffer size on entry, result size on exit
 *                 engine         engine handle
 *
 * Return:         true           if successful
 *                 false          if failed
 *
 */
bool
   ria_uapi_continue( 
      ria_exec_status_t*   OUT      status,    
      char*                IN OUT   presult,
      usize*               IN OUT   cresult,
      ria_handle_t         IN       engine);

/*@@ria_uapi_parse
 *
 * Performs parsing action
 *
 * Parameters:     presult        buffer with result
 *                 cresult        buffer size on entry, result size on exit
 *                 name           action name
 *                 pos            parsing position
 *                 engine         engine handle
 *
 * Return:         true           if successful
 *                 false          if failed
 *
 */
bool 
   ria_uapi_parse(       
      char*          IN OUT   presult,
      usize*         IN OUT   cresult,
      const char*    IN       name,
      usize*         IN OUT   pos,
      ria_handle_t   IN       engine);

#endif
