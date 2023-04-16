#include <stddef.h>
#include "emb_list.h"
#include "emb_heap.h"
#include "ria_exec.h"
#include "ria_papi.h"
#include "ria_uapi.h"


/******************************************************************************
 *   Engines list
 */

#define MSG_SIZE  512
#define ERR_SIZE  50
#define HEAP_SIZE 165000
 
static uint32          _init_sync = 0;
static unumber         _ref = 0;
static unumber         _ctr = 0;
static generic_mutex_t _sync;
static list_entry_t    _engines;
static heap_ctx_t*     _heap;

typedef struct ria_engine_s {
   list_entry_t       linkage;
   ria_handle_t       id;
   bool               locked;
   char               errmsg[MSG_SIZE];
   buf_t              exec;
   ria_compiler_ctx_t compiler;
   ria_executor_ctx_t executor;
   umask              cleanup;
} ria_engine_t;

#define LIST_TO_ENGINE(_p)                                                    \
       ( (ria_engine_t*)((byte*)(_p) - OFFSETOF(ria_engine_t, linkage)) )

#define DUMP_SYS_ERROR(_eng)                                                  \
           {                                                                  \
              err_ctx_t* _perr = GET_ERR_CONTEXT;                             \
              if ((_perr != NULL) && (_perr->err == err_none)) {              \
                 Sprintf(                                                     \
                    (_eng)->errmsg,                                           \
                    sizeof((_eng)->errmsg),                                   \
                    "\nNo errors\n");                                         \
              }                                                               \
              else                                                            \
              if (_perr != NULL) {                                            \
                 Sprintf(                                                     \
                    (_eng)->errmsg,                                           \
                    sizeof((_eng)->errmsg),                                   \
                    "\nError! %s, file %s, line %d\n",                        \
                    err_get_description(_perr->err),                          \
                    _perr->file,                                              \
                    _perr->line);                                             \
              }                                                               \
           }

/*
 * Memory allocator
 *
 */
#if defined(WIN32_APP) 
#define Malloc(_s) malloc(_s)
#define Free(_s)   free(_s)
#elif defined(ANDROID)
#define Malloc(_s) malloc(_s)
#define Free(_s)   free(_s)
#elif defined(WISE12)
#define Malloc(_s) SYSHEAP_DEBUG_ALLOC(_s)
#define Free(_s)   SYSHEAP_DEBUG_FREE(_s)
#else
#error Target platform is unknown
#endif 
       

/*****************************************************************************/

enum {
   cf_ria_engine_compiler = 0x01,
   cf_ria_engine_executor = 0x02,
   cf_ria_engine_exec     = 0x04
};

/*****************************************************************************/
static bool
   ria_engine_destroy(
      ria_engine_t*   IN OUT   engine)
/*
 * Destroys internal RIA engine object
 *
 */
{

   bool ret = true;
   
   assert(engine != NULL);

   if (engine->cleanup & cf_ria_engine_compiler)
      ret = ria_compiler_destroy(&engine->compiler) && ret;
   if (engine->cleanup & cf_ria_engine_executor)
      ret = ria_executor_destroy(&engine->executor) && ret;
   if (engine->cleanup & cf_ria_engine_exec)
      ret = buf_destroy(&engine->exec) && ret;
      
   engine->cleanup = 0x00;  
   return ret;
   
}      

/*****************************************************************************/
static bool
   ria_engine_create(
      ria_engine_t*   IN OUT   engine)
/*
 * Initializes internal RIA engine object
 *
 */
{

   assert(engine != NULL);

   MemSet(engine, 0x00, sizeof(*engine));
   list_init_entry(&engine->linkage);

   if (!ria_compiler_create(&engine->compiler, _heap))
      goto failed;
   else
      engine->cleanup |= cf_ria_engine_compiler;
   if (!ria_executor_create(&engine->executor, _heap))
      goto failed;
   else
      engine->cleanup |= cf_ria_engine_executor;
   if (!buf_create(sizeof(byte), 0, 0, &engine->exec, _heap))
      goto failed;
   else
      engine->cleanup |= cf_ria_engine_exec;
   return true;
   
failed:
   ria_engine_destroy(engine);
   return false;   
   
}      

/*****************************************************************************/
static ria_engine_t*
   ria_lock_engine(
      handle   IN   engine)
/*
 * Locks engine by handle
 *
 */
{

   list_entry_t* pl;
   ria_engine_t* pe = NULL;
   
   /*
    * Check for initialization
    *
    */
   if (_ref == 0) {
      ERR_SET_NO_RET(err_unexpected_call);
      return NULL;
   }   

   /*
    * Find and lock
    *
    */
   sync_mutex_lock(&_sync);
   list_for_each(&_engines, &pl) {
      pe = LIST_TO_ENGINE(pl);
      if (pe->id == engine)
         break;
      pe = NULL;
   }
   if (pe != NULL) {
      if (pe->locked) 
         pe = NULL;
      else   
         pe->locked = true;
   }         
   sync_mutex_unlock(&_sync);
   return pe;

}

/*****************************************************************************/
static bool
   ria_unlock_engine(
      ria_engine_t*   engine)
/*
 * Unlocks engine by pointer
 *
 */
{

   assert(engine != NULL);

   /*
    * Check for initialization
    *
    */
   if (_ref == 0)
      ERR_SET(err_unexpected_call);
   
   /*
    * Unlock
    *
    */
   sync_mutex_lock(&_sync);
   engine->locked = false;
   sync_mutex_unlock(&_sync);
   return true;

}
   

/******************************************************************************
 *   API
 */

/*****************************************************************************/
bool 
   portable_get_app_heap_params(
      byte**   OUT ppbuf, 
      usize*   OUT pcbuf)
{
   UNUSED(ppbuf);
   UNUSED(pcbuf);
   return false;
}      

/*****************************************************************************/
ria_handle_t
   ria_uapi_init(
      const char*   IN   tempdir)
/*
 * Initializes RIA engine
 *
 */
{

   ria_engine_t* engine;
   
   assert(tempdir != NULL);
   
   /*
    * Make shared initialization 
    * If two threads are initializing/deinitializing simultaneously,
    * it seems to be a high-concurrency system, 
    * so deadlock should not happen due to wait cycles
    *
    */
   while (sync_interlocked_exchange32(&_init_sync, 1) == 1) 
      Delay(10);
   if (_ref == 0) {   
      _heap = Malloc(HEAP_SIZE+sizeof(*_heap));
      if (_heap == NULL) {
         ERR_SET_NO_RET(err_internal);
         goto init_failed;
      }   
      if (!heap_create(_heap+1, HEAP_SIZE, 0, _heap)) {
         Free(_heap);
         goto init_failed;
      }         
      list_init_head(&_engines);
      sync_mutex_create(&_sync);
   }   
   _ref++;
   sync_interlocked_exchange32(&_init_sync, 0);

   /*
    * Create engine
    *
    */
   if (!heap_alloc((void**)&engine, sizeof(*engine), _heap))
      return NULL; 
   if (!ria_engine_create(engine)) {
      heap_free(engine, _heap);
      return NULL;
   }  
   if (!ria_set_option(
           ria_option_tempdir,
           (byte*)tempdir,
           StrLen(tempdir),
           &engine->executor)) {
      ria_engine_destroy(engine);
      heap_free(engine, _heap);
      return NULL;        
   }        
   
   /*
    * Add to list
    *
    */
   sync_mutex_lock(&_sync);
   engine->id = (ria_handle_t)(usize)(++_ctr);
   list_insert_tail(&_engines, &engine->linkage);
   sync_mutex_unlock(&_sync);
   return engine->id;

init_failed:
   sync_interlocked_exchange32(&_init_sync, 0);
   return 0;

} 

/*****************************************************************************/
bool
   ria_uapi_shutdown(
      ria_handle_t   IN   engine)
/*
 * Shutdowns RIA engine
 *
 */
{
 
   ria_engine_t* pe;
   bool ret;

   /*
    * Lock engine
    *
    */
   pe = ria_lock_engine(engine);
   if (pe == NULL)
      return false;
   
   /*
    * Remove engine from list
    *
    */
   sync_mutex_lock(&_sync);
   list_remove_entry_simple(&pe->linkage);
   sync_mutex_unlock(&_sync);
   
   /*
    * Destroy engine 
    *
    */
   ret = ria_engine_destroy(pe);
   ret = heap_free(pe, _heap) && ret;

   /*
    * Make shared uninitialization 
    * If two threads are initializing/deinitializing simultaneously,
    * it seems to be a high-concurrency system, 
    * so deadlock should not happen due to wait cycles
    *
    */
   while (sync_interlocked_exchange32(&_init_sync, 1) == 1) 
      Delay(10);
   _ref--;   
   if (_ref == 0) {   
      sync_mutex_destroy(&_sync);
      if (!list_is_empty(&_engines)) {
         ERR_SET_NO_RET(err_internal);
         ret = false;
      }   
      ret = heap_destroy(_heap) && ret;
      Free(_heap);   
   }   
   sync_interlocked_exchange32(&_init_sync, 0);
   return ret;   

}     

/*****************************************************************************/
const char*
   ria_uapi_error_msg(
      ria_handle_t   IN   engine)
/*
 * Returns last cached error message
 *
 */
{

   ria_engine_t* pe;
   const char* ret;

   pe = ria_lock_engine(engine);
   if (pe == NULL)
      return "";
      
   ret = pe->errmsg;
   if (!ria_unlock_engine(pe))
      ERR_SET_NO_RET(err_internal);
   return ret;   
      
}

/*****************************************************************************/
bool
   ria_uapi_alloc(
      void**   OUT   ptr,
      usize    IN    size)
/*
 * Allocates memory from internal RIA heap 
 *
 */
{
   assert(ptr != NULL);
   return heap_alloc(ptr, size, _heap);   
}

/*****************************************************************************/
bool
   ria_uapi_free(
      void*   IN   ptr)
/*
 * Frees memory into internal RIA heap 
 *
 */
{
   assert(ptr != NULL);
   return heap_free(ptr, _heap);
} 

/*****************************************************************************/
bool
   ria_uapi_load(     
      const char*    IN   path,
      ria_handle_t   IN   engine)
/*
 * Loads
 *
 */
{

   ria_engine_t* pe;
   mem_blk_t script;
   mem_blk_t exec;
   void* file;
   usize c;
   byte* p = NULL;
   bool  ret = false;
   
   enum {
      cleanup_file = 0x01,
      cleanup_p    = 0x02
   } cleanup = 0x00;
   
   pe = ria_lock_engine(engine);
   if (pe == NULL)
      return false;
      
   /*
    * Read script from file
    *
    */
   if (!ria_papi_fopen(&file, path, "rt")) {
      Sprintf(pe->errmsg, sizeof(pe->errmsg), "Cannot open script file");
      goto exit;
   }      
   else
      cleanup |= cleanup_file;  
      
   /*
    * Load script data
    *
    */
   if (!ria_papi_fseek(0, ria_file_seek_end, file)) {
      Sprintf(pe->errmsg, sizeof(pe->errmsg), "Cannot define script size");
      goto exit;
   }
   if (!ria_papi_ftell(&c, file)) {
      Sprintf(pe->errmsg, sizeof(pe->errmsg), "Cannot define script size");
      goto exit;
   }
   if (!ria_papi_fseek(0, ria_file_seek_begin, file)) {
      Sprintf(pe->errmsg, sizeof(pe->errmsg), "Cannot define script size");
      goto exit;
   }
   if (!heap_alloc(&p, c*2, _heap)) {
      DUMP_SYS_ERROR(pe);
      goto exit;
   }
   else
      cleanup |= cleanup_p;
   if (!ria_papi_fread(&c, p, c, file)) {
      Sprintf(pe->errmsg, sizeof(pe->errmsg), "Cannot read script file");
      goto exit;   
   }      
      
   script.p = p;   
   script.c = c;
   exec.p   = p + c;
   exec.c   = c;

   /*
    * Compile
    *
    */
   ret = ria_compile_script_module(&exec, &script, &pe->compiler);
   if (!ret) {
      DUMP_SYS_ERROR(pe);
      goto exit;
   }
   else 
   if (!pe->compiler.ok) {
      usize c1, c2;
      Sprintf(
         pe->errmsg, 
         sizeof(pe->errmsg), 
         "Script compilation error <%s> at: \n", 
         pe->compiler.errmsg);
      c1 = strlen(pe->errmsg);
      c2 = sizeof(pe->errmsg) - c1;
      c2 = (c2 < ERR_SIZE+4) ? c2 : ERR_SIZE+4;
      MemCpy(pe->errmsg+c1, pe->compiler.perror, c2-4);
      pe->errmsg[c1+c2-4] = '.';
      pe->errmsg[c1+c2-3] = '.';
      pe->errmsg[c1+c2-2] = '.';
      pe->errmsg[c1+c2-1] = 0x00;
      ret = false;
      goto exit;
   }      
   
   /*
    * Save executable code
    *
    */
   if (!buf_load(exec.p, 0, exec.c, &pe->exec)) {
      DUMP_SYS_ERROR(pe);
      ret = false;
      goto exit;
   }
   
exit:   
   if (cleanup & cleanup_p)
      ret = heap_free(p, _heap) && ret;
   if (cleanup & cleanup_file)
      ret = ria_papi_fclose(file) && ret;   
   return ria_unlock_engine(pe) && ret;

}

/*****************************************************************************/
bool
   ria_uapi_execute(     
      ria_exec_status_t*   OUT      status,    
      char*                IN OUT   presult,
      usize*               IN OUT   cresult,
      const char*          IN       name,
      const char**         IN       pparams,
      usize                IN       cparams,
      ria_handle_t         IN       engine)
/*
 * Executes script
 *
 */
{

   ria_engine_t* pe;
   mem_blk_t exec;
   mem_blk_t result;
   bool ret = false;
   unumber i;
   
   assert(pparams != NULL);
   assert(presult != NULL);
   assert(cresult != NULL);
   assert(status  != NULL);
   
   pe = ria_lock_engine(engine);
   if (pe == NULL)
      return false;
      
   /*
    * Apply params
    *
    */      
   for (i=0; i<cparams; i++) 
      if (!ria_set_exec_param(pparams[i], i, &pe->executor)) {
         DUMP_SYS_ERROR(pe);
         goto exit;
      }         
      
   exec.p   = buf_get_ptr_bytes(&pe->exec);
   exec.c   = buf_get_length(&pe->exec);
   result.p = (byte*)presult;
   result.c = *cresult - 1;
   ret = ria_execute_script(status, &result, name, &exec, &pe->executor);
   if (!ret) {
      DUMP_SYS_ERROR(pe);
      goto exit;
   }
   else
   if (*status == ria_exec_failed) {
      err_ctx_t* perr = GET_ERR_CONTEXT;                             
      Sprintf(
         pe->errmsg, 
         sizeof(pe->errmsg), 
         "Script execution error at pos: 0x%02X, file %s, line %d\n", 
         ria_get_exec_error_pos(&pe->executor),
         perr->file,                                              
         perr->line);                                             
      ret = false;   
      goto exit;
   }

   if (result.p != (byte*)presult) {
      ERR_SET_NO_RET(err_internal);
      DUMP_SYS_ERROR(pe);
      ret = false;
      goto exit;
   }
   result.p[result.c] = 0x00;
   *cresult = result.c + 1;

exit:   
   return ria_unlock_engine(pe) && ret;

}

/*****************************************************************************/
bool
   ria_uapi_continue( 
      ria_exec_status_t*   OUT      status,    
      char*                IN OUT   presult,
      usize*               IN OUT   cresult,
      ria_handle_t         IN       engine)
/*
 * Continues execution of pending script
 *
 */
{

   ria_engine_t* pe;
   bool ret = false;
#ifdef USE_RIA_ASYNC_CALLS   
   ria_buf_t result;
#endif   

   assert(presult != NULL);
   assert(cresult != NULL);
   assert(status  != NULL);
#ifdef _MSC_VER
   cresult, presult, status;
#endif
   
   
   pe = ria_lock_engine(engine);
   if (pe == NULL)
      return false;

#ifndef USE_RIA_ASYNC_CALLS   
   ERR_SET_NO_RET(err_internal);
   DUMP_SYS_ERROR(pe);
   goto exit;
#else

   result.p = (byte*)presult;
   result.c = *cresult - 1;
   ret = ria_continue_script(status, &result, &pe->executor);
   if (!ret) {
      DUMP_SYS_ERROR(pe);
      goto exit;
   }
   else
   if (*status == ria_exec_failed) {
      err_ctx_t* perr = GET_ERR_CONTEXT;                             
      sprintf_s(pe->errmsg, sizeof(pe->errmsg), 
         "Script execution error at pos: 0x%02X, file %s, line %d\n", 
         ria_get_exec_error_pos(&pe->executor),
         perr->file,                                              
         perr->line);                                             
      ret = false;   
      goto exit;
   }
   
   if (result.p != (byte*)presult) {
      ERR_SET_NO_RET(err_internal);
      DUMP_SYS_ERROR(pe);
      ret = false;
      goto exit;
   }
   result.p[result.c] = 0x00;
   *cresult = result.c + 1;
   
#endif

exit:   
   return ria_unlock_engine(pe) && ret;
   
}

/*****************************************************************************/
bool 
   ria_uapi_parse(       
      char*          IN OUT   presult,
      usize*         IN OUT   cresult,
      const char*    IN       name,
      usize*         IN OUT   pos,
      ria_handle_t   IN       engine)
/*
 * Performs parsing action
 *
 */
{

   ria_engine_t* pe;
   mem_blk_t result;
   bool ret = false;
   
   assert(presult != NULL);
   assert(cresult != NULL);
   assert(name    != NULL);
   assert(pos     != NULL);
   
   pe = ria_lock_engine(engine);
   if (pe == NULL)
      return false;
      
   /*
    * Execute
    *
    */      
   result.p = (byte*)presult;
   result.c = *cresult - 1;
   ret = ria_parser_action(&result, name, pos, &pe->executor);
   if (!ret) {
      DUMP_SYS_ERROR(pe);
      goto exit;
   }

   if (result.p != (byte*)presult) {
      ERR_SET_NO_RET(err_internal);
      DUMP_SYS_ERROR(pe);
      ret = false;
      goto exit;
   }
   result.p[result.c] = 0x00;
   *cresult = result.c + 1;

exit:   
   return ria_unlock_engine(pe) && ret;


}