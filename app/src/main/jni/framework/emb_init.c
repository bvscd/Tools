#include "emb_heap.h"
#include "emb_init.h"

/******************************************************************************
 *   Globals 
 */

/*
 * Error descriptions
 *
 */
static const err_info_t 
   _err_info[] =
{
   { err_none,             "<SUCCESS>"                  },
   { err_bad_param,        "bad parameter"              },
   { err_no_memory,        "no memory"                  },
   { err_invalid_pointer,  "invalid pointer"            },
   { err_heap_corrupted,   "heap corrupted"             },
   { err_out_of_bounds,    "out of bounds"              },
   { err_unexpected_call,  "unexpected call"            },
   { err_bad_object,       "bad object"                 },
   { err_not_supported,    "not supported"              },
   { err_buffer_too_small, "buffer too_small"           },
   { err_bad_length,       "bad length"                 },
   { err_data_corrupted,   "data corrupted"             },
   { err_internal,         "internal error"             }
};

/*
 * Module description
 *
 */
static module_info_t 
   _module_info = 
{
   MODULE_BASE, 
   "framework", 
   NULL, 
   NULL,
   NULL,
   0,
   _err_info, 
   sizeof(_err_info)/sizeof(_err_info[0]), 
   NULL
};

/*
 * Internal (system) heap 
 *
 */
static byte
   _sys_heap_buf[DEF_SYS_HEAP_SIZE];

static heap_ctx_t
   _sys_heap;

/*
 * Application heap
 *
 */
static heap_ctx_t
   _app_heap;

/*
 * Internal sync variables 
 *
 */
static generic_mutex_t
   _framework_sync;
static uint32
   _framework_sync_init = 0;
static unumber
   _framework_ref_count = 0;
static umask
   _framework_cleanup = 0;

/*
 * Cleanup flags 
 *
 */
enum {
   cf_framework_sync     = 0x01,
   cf_framework_sys_heap = 0x02,
   cf_framework_app_heap = 0x04
};


/******************************************************************************
 *   Framework library initialization/destruction
 */

/*****************************************************************************/
heap_ctx_t*
   framework_init(
      void)
/*
 * Initializes framework library
 * 
 */
{

   heap_ctx_t* ret = NULL;
   byte* pbuf;
   usize cbuf;

   /*
    * Sync
    *
    */
   while (sync_interlocked_exchange32(&_framework_sync_init, 1) != 0) {
      Delay(1);
   }
   _framework_ref_count++;
   if (_framework_ref_count > 1) {
      ret = &_app_heap;
      goto exit;
   }

   sync_mutex_create(&_framework_sync);
   _framework_cleanup |= cf_framework_sync;

   if (!heap_create(_sys_heap_buf, sizeof(_sys_heap_buf), 0, &_sys_heap))
      goto exit;
   else
      _framework_cleanup |= cf_framework_sys_heap;
   if (portable_get_app_heap_params(&pbuf, &cbuf)) {
      if (!heap_create(
              pbuf, 
              cbuf, 
#ifdef USE_MALLOC
              heap_no_trace|heap_use_malloc,
#else
              heap_no_trace,
#endif
              &_app_heap))
         goto exit;
      else
         _framework_cleanup |= cf_framework_app_heap;
      ret = &_app_heap;
   }
   else
      ret = &_sys_heap;

exit:   
   sync_interlocked_exchange32(&_framework_sync_init, 0);
   if (ret == NULL)
      framework_done();
   return ret;

}

/*****************************************************************************/
bool
   framework_done(                                
      void)
/* 
 * Destroys framework library
 * 
 */
{

   bool ret = true;
   usize i;

   module_info_t *p, *pp;

   /*
    * Sync
    *
    */
   while (sync_interlocked_exchange32(&_framework_sync_init, 1) != 0) {
      Delay(1);
   }
   if (_framework_ref_count == 0)
      goto exit;
   _framework_ref_count--;
   if (_framework_ref_count > 0)
      goto exit;

   /*
    * Shutdowns all registered modules
    * 
    */
 
   while (_module_info.next != NULL) {
      for (p=&_module_info, pp=NULL; p->next!=NULL; pp=p, p=p->next)
         if (p->next == &_module_info) {
            p->next = NULL;
            break;
         }
      if (pp != NULL)
         pp->next = NULL;
      for (i=p->csub; i>0; i--) {
         if (p->psub[i-1] == NULL)
            continue;
         if (p->psub[i-1]->done != NULL)
            ret = (*p->psub[i-1]->done)() && ret;
      }
      if (p->done != NULL)
         ret = (*p->done)() && ret;
   }

   if (_framework_cleanup & cf_framework_app_heap)
      ret = heap_destroy(&_app_heap) && ret;
   if (_framework_cleanup & cf_framework_sys_heap)
      ret = heap_destroy(&_sys_heap) && ret;
   if (_framework_cleanup & cf_framework_sync)
      sync_mutex_destroy(&_framework_sync);
   _framework_cleanup = 0x00;

exit:   
   sync_interlocked_exchange32(&_framework_sync_init, 0);
   return ret;

}


/******************************************************************************
 *  Module/error handling 
 */

/******************************************************************************/
bool
   module_register(
      module_info_t*   IN   module)
/*
 * Registers module in framework
 *
 */
{

   bool  ret;
   usize i;

   module_info_t* p = &_module_info;

   if (module == NULL)
      ERR_SET(err_bad_param);
   if (_framework_ref_count == 0)
      ERR_SET(err_unexpected_call);

   /*
    * Sync
    *
    */
   sync_mutex_lock(&_framework_sync);

   /*
    * Registration
    *
    */
   for (ret=true; ; p=p->next) {

      /*
       * Check duplications
       *
       */
      if (p->next == module)
         break;
      if (p->id == module->id) {
         ERR_SET_NO_RET(err_bad_param);
         break;
      }
      if (p->next == &_module_info) {
         ERR_SET_NO_RET(err_bad_param);
         break;
      }
      if (p->next != NULL)
         continue;

      /*
       * Initialize
       *
       */
      module->next = NULL;
      if (module->init != NULL)
         ret = (*module->init)(&_sys_heap) && ret;
      if (!ret)
         break;
      for (i=0; i<module->csub; i++) {
         if (module->psub[i] == NULL)
            continue;
         if (module->psub[i]->init != NULL) 
            ret = (*module->psub[i]->init)(&_sys_heap) && ret;
         if (!ret)
            break;
      }
      if (!ret)
         break;
            
      /*
       * Register 
       *
       */
      p->next = module;
      break;

   }

   /*
    * Sync
    *
    */
   ret = (p->next == module) && ret;
   sync_mutex_unlock(&_framework_sync);
   return ret;

}

/******************************************************************************/
const char*
   err_get_description(
      int   IN   code)
/*
 * Returns error description by its code
 *
 */
{

   static const char _unknown[] = "unknown error";

   const char*    ret = _unknown;
   module_info_t* p   = &_module_info;

   /*
    * Sync
    *
    */
   sync_mutex_lock(&_framework_sync);

   for (; p!=NULL; p=p->next) {
      usize i;
      for (i=0; i<p->cerr; i++)
         if (p->perr[i].err == code) {
            ret = (p->perr[i].txt == NULL) ? "" : p->perr[i].txt;
            break;
         }
      if (p->next == &_module_info)
         break;
   }

   /*
    * Sync
    *
    */
   sync_mutex_unlock(&_framework_sync);
   return ret;

}

/*****************************************************************************/
err_ctx_t*
   err_get_context(
      void)
/*
 * Returns error context (the only per system)
 * 
 */
{
   static err_ctx_t err;
   return &err;
}

