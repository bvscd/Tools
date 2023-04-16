#ifndef _RIA_EXEC
#define _RIA_EXEC


#include "ria_core.h"
#include "ria_http.h"
#include "ria_pars.h"


/*
 * Data kinds
 *
 */
typedef enum ria_data_kind_e {
   ria_data_var = 0x01,
   ria_data_str = 0x02,
   ria_data_par = 0x03,
   ria_data_tmp = 0x04,
   ria_data_res = 0x05,
   ria_data_int = 0x06
} ria_data_kind_t;

/*
 * Execution result
 *
 */
typedef enum ria_exec_status_e {
   ria_exec_unknown         = 0x00, 
   ria_exec_ok              = 0x01,
   ria_exec_failed          = 0x02,
   ria_exec_ok_parser_ready = 0x03,
#ifdef USE_RIA_ASYNC_CALLS   
   ria_exec_pending         = 0x04,
   ria_exec_transit         = 0x05,
   ria_exec_proceed         = 0x06
#endif   
} ria_exec_status_t;

/*
 * Pending state
 *
 */
#ifdef USE_RIA_ASYNC_CALLS   
typedef struct ria_pending_s {
   void*          resbuf;
   function_fn*   func;
   byte*          ppar;
   uint           cpar;
   uint           cunwind;
   ria_type_t     restype;
} ria_pending_t; 
#endif   

/*
 * Execution state flags
 *
 */
typedef enum ria_exec_state_flags_e {
   ria_ef_parser_ready = 0x01
} ria_exec_state_flags_t;

/*
 * Execution state
 *
 */
typedef struct ria_exec_state_s {
   ria_exec_status_t   status;
   umask               flags;
   const byte*         pexec;
   usize               cexec;
   const byte*         pstart;
   byte*               ptr_strs;
   buf_t               globals;
   buf_t               params;
   buf_t               vars;
   buf_t               tmps;
   buf_t               stack;
   buf_t               result;
#ifdef USE_RIA_ASYNC_CALLS   
   ria_pending_t       pending;
#endif
   ria_http_t          http;
   ria_parser_t        parser;
   heap_ctx_t*         mem;
   umask               cleanup;
} ria_exec_state_t; 

/*
 * Execution context
 *
 */
typedef struct ria_executor_ctx_s {
   ria_exec_state_t   state;
   ria_config_t       config;
   umask              cleanup;
} ria_executor_ctx_t;

/*@@ria_executor_create
 *
 * Creates execution context
 *
 * Parameters:     ctx            execution context
 *                 mem            memory context
 *
 * Return:         true           if successful
 *                 false          if failed
 *
 */
bool 
   ria_executor_create(     
      ria_executor_ctx_t*   IN OUT   ctx,
      heap_ctx_t*           IN OUT   mem);

/*@@ria_executor_destroy
 *
 * Destroys execution context
 *
 * Parameters:     ctx            execution context
 *
 * Return:         true           if successful
 *                 false          if failed
 *
 */
bool 
   ria_executor_destroy(     
      ria_executor_ctx_t*   IN OUT   ctx);

/*@@ria_set_option
 *
 * Sets execution option
 *
 * Parameters:     option         option type
 *                 pval           option value 
 *                 cval           option value size, bytes
 *                 ctx            execution context
 *
 * Return:         true           if successful
 *                 false          if failed
 *
 */
bool 
   ria_set_option(
      ria_option_t          IN       option,
      const byte*           IN       pval,
      usize                 IN       cval,
      ria_executor_ctx_t*   IN OUT   ctx);

/*@@ria_set_exec_param
 *
 * Sets parameter for scenario script 
 *
 * Parameters:     param          pointer to parameter in ASCIIZ form
 *                 idx            parameter index
 *                 ctx            execution context
 *
 * Return:         true           if successful
 *                 false          if failed
 *
 */
bool 
   ria_set_exec_param(
      const char*           IN       param,
      unumber               IN       idx,
      ria_executor_ctx_t*   IN OUT   ctx);

/*@@ria_execute_script
 *
 * Executes compiled scenario script 
 *
 * Parameters:     status         execution status 
 *                 result         result buffer
 *                 name           script name
 *                 module         compiled module
 *                 ctx            execution context
 *
 * Return:         true           if successful
 *                 false          if failed
 *
 */
bool 
   ria_execute_script(       
      ria_exec_status_t*    OUT      status,
      mem_blk_t*            IN OUT   result,
      const char*           IN       name,
      const mem_blk_t*      IN OUT   module,
      ria_executor_ctx_t*   IN OUT   ctx);

#ifdef USE_RIA_ASYNC_CALLS   
/*@@ria_continue_script
 *
 * Continues execution of pending scenario script 
 *
 * Parameters:     status         execution status 
 *                 result         result buffer
 *                 ctx            execution context
 *
 * Return:         true           if successful
 *                 false          if failed
 *
 */
bool 
   ria_continue_script(       
      ria_exec_status_t*    OUT      status,
      mem_blk_t*            IN OUT   result,
      ria_executor_ctx_t*   IN OUT   ctx);
#endif      

/*@@ria_get_datainfo_from_buf
 *
 * Fetches service info about data from buffer representation
 *
 * Parameters:     type           data type storage
 *                 ptr            data storage
 *                 len            data size storage
 *                 buf            buffer
 *
 * Return:         true           if successful
 *                 false          if failed
 *
 */
bool 
   ria_get_datainfo_from_buf(                                            
      ria_type_t*    OUT   type,
      byte**         OUT   ptr,
      usize*         OUT   len,
      const buf_t*   IN    buf);
      
/*@@ria_prealloc_datatype_in_buf
 *
 * Allocates required storage for given data type
 *
 * Parameters:     ptr            data storage
 *                 type           data type storage
 *                 len            data size storage
 *                 buf            buffer
 *
 * Return:         true           if successful
 *                 false          if failed
 *
 */
bool 
   ria_prealloc_datatype_in_buf(                                            
      byte**       OUT   ptr,
      ria_type_t   IN    type,
      usize        IN    len,
      buf_t*       IN    buf);

/*@@ria_get_exec_error_pos
 *
 * Returns execution error position
 *
 * Parameters:     ctx            execution context
 *
 * Return:         error position value
 *
 */
#define /* bool */ ria_get_exec_error_pos(                                    \
                      /* const ria_executor_ctx_t*   IN */   _ctx)            \
       ( assert((_ctx)!=NULL), (_ctx)->state.pexec-(_ctx)->state.pstart )

/*@@ria_parser_action
 *
 * Executes parser action
 *
 * Parameters:     result         result buffer
 *                 name           action name
 *                 pos            position storage
 *                 ctx            execution context
 *
 * Return:         true           if successful
 *                 false          if failed
 *
 */
bool 
   ria_parser_action(       
      mem_blk_t*            IN OUT   result,
      const char*           IN       name,
      usize*                IN OUT   pos,
      ria_executor_ctx_t*   IN OUT   ctx);


#endif
