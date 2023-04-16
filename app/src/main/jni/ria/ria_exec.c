#include "ria_exec.h"
#include "ria_func.h"
#include "ria_core.h"


/******************************************************************************
 *  Forwards 
 */

/*
 * Save execution error pos
 *
 */
#define SET_EXECUTE_ERROR(_ctx)                                               \
       {                                                                      \
          (_ctx)->status = ria_exec_failed;                                   \
          ERR_SET_NO_RET(err_internal);                                       \
       }


/******************************************************************************
 *  Data handling
 */

/*****************************************************************************/
bool 
   ria_get_datatype_from_buf(                                            
      ria_type_t*    OUT   type,
      const buf_t*   IN    buf)
/*
 * Returns data type from buffer 
 *
 */
{

   assert(type != NULL);
   assert(buf  != NULL);

   if (buf == NULL)
      ERR_SET(err_internal);
   if (buf_get_length(buf) < 1)
      ERR_SET(err_internal);
   *type = buf_get_ptr_bytes(buf)[0];

   switch (*type) {
   case ria_unknown:
   case ria_string:
   case ria_int:
   case ria_boolean:
      return true;
   default:
      ERR_SET(err_internal);
   }

}

/*****************************************************************************/
bool 
   ria_get_datainfo_from_buf(                                            
      ria_type_t*    OUT   type,
      byte**         OUT   ptr,
      usize*         OUT   len,
      const buf_t*   IN    buf)
/*
 * Returns information about data from buffer 
 *
 */
{

   assert(ptr  != NULL);
   assert(len  != NULL);
   assert(buf  != NULL);

   if (!ria_get_datatype_from_buf(type, buf))
      return false;
   *len = buf_get_length(buf) - 1;
   *ptr = buf_get_ptr_bytes(buf) + 1;
   return true;

}

/*****************************************************************************/
bool 
   ria_set_datatype_into_buf(                                            
      buf_t*        IN OUT   buf,
      ria_type_t    OUT      type)
/*
 * Sets data type into buffer 
 *
 */
{

   assert(buf  != NULL);

   if (buf == NULL)
      ERR_SET(err_internal);
   if ((type == ria_unknown) && (buf_get_length(buf) > 1)) {
      if (!buf_destroy(buf))
         return false;
      if (!buf_fill(0x00, 0, 1, buf))
         return false;   
   }   
   if (buf_get_length(buf) < 1)
      ERR_SET(err_internal);
   buf_get_ptr_bytes(buf)[0] = (byte)type;
   return true;

}

/*****************************************************************************/
bool 
   ria_prealloc_datatype_in_buf(                                            
      byte**       OUT   ptr,
      ria_type_t   IN    type,
      usize        IN    len,
      buf_t*       IN    buf)
/*
 * Allocates required storage for given data type
 *
 */
{

   assert(ptr  != NULL);
   assert(buf  != NULL);

   if (!buf_expand(len+1, buf))
      return false;
   if (!buf_set_length(len+1, buf))
      return false;

   *ptr = buf_get_ptr_bytes(buf) + 1;
   return ria_set_datatype_into_buf(buf, type);

}

/*****************************************************************************/
bool 
   ria_get_var(                                            
      void**              IN       ptr,
      usize               IN       idx,
      bool                IN       creatable,
      ria_exec_state_t*   IN OUT   ctx)
/*
 * Returns pointer to variable
 *
 */
{

   usize i, c;
   byte**  p;

   buf_t* pvars = (idx >= ria_var_threshold) ? &ctx->globals : &ctx->vars;
   if (idx >= ria_var_threshold)
      idx -= ria_var_threshold;

   assert(ptr != NULL);
   assert(ctx != NULL);

   *ptr = NULL;

   /*
    * Check whether exists; create if allowed
    *
    */
   c = buf_get_length(pvars); 
   p = buf_get_ptr_ptrs(pvars);
   if ((c <= idx) || (p[idx] == NULL)) {
      if (idx >= ria_var_threshold)
         ERR_SET(err_internal);
      if (!creatable)
         return true;
      else {
         byte* p;
         buf_t* pb;
         if (c <= idx) {
            if (!buf_expand(idx+1, pvars))
               return false;
            if (!buf_set_length(idx+1, pvars))
               return false;
            for (i=c; i<idx; i++)
               buf_get_ptr_ptrs(pvars)[i] = NULL;
         }      
         if (!heap_alloc((void**)&pb, sizeof(buf_t), ctx->mem))
            return false;
         if (!buf_create(1, 1, 0, pb, ctx->mem))
            return false;
         if (!ria_prealloc_datatype_in_buf(&p, ria_unknown, 0, pb))
            return false;
         buf_get_ptr_ptrs(pvars)[idx] = (byte*)pb;
         *ptr = pb;
      }
   }
   else
      *ptr = buf_get_ptr_ptrs(pvars)[idx];

   return true;

}

/*****************************************************************************/
bool 
   ria_get_str(                                            
      void**              IN       ptr,
      usize               IN       idx,
      ria_exec_state_t*   IN OUT   ctx)
/*
 * Returns pointer to string
 *
 */
{

   byte* p;

   assert(ptr != NULL);
   assert(ctx != NULL);

   *ptr = NULL;

   p = ctx->ptr_strs;
   for (; idx>0; idx--)
      p += *p + 1;

   *ptr = p + 1;
   return true;

}

/*****************************************************************************/
bool 
   ria_get_par(                                            
      void**              IN       ptr,
      usize               IN       idx,
      ria_exec_state_t*   IN OUT   ctx)
/*
 * Returns pointer to parameter
 *
 */
{

   usize c, i;
   byte* p;

   assert(ptr != NULL);
   assert(ctx != NULL);

   *ptr = NULL;
   
   /*
    * Find parameter by index
    *
    */
   c = buf_get_length(&ctx->params);
   p = buf_get_ptr_bytes(&ctx->params);
   for (; c>0; ) {
      if (c == 1)
         ERR_SET(err_internal);
      if (*(p++) == idx) {
         *ptr = p;
         break;
      }   
      c--;
      i = StrLen((char*)p);
      if (i >= c)
         ERR_SET(err_internal);
      p += i + 1;
      c -= i + 1;   
   }  

   return true;

}

/*****************************************************************************/
bool 
   ria_get_tmp(                                            
      void**              IN       ptr,
      ria_exec_state_t*   IN OUT   ctx)
/*
 * Returns pointer to free temporary variable
 *
 */
{

   usize i;

   assert(ptr != NULL);
   assert(ctx != NULL);

   *ptr = NULL;

   /*
    * Try to find free temporary var
    *
    */
   for (i=buf_get_length(&ctx->tmps); i>0; i--) {
      ria_type_t type;
      buf_t* pb = (buf_t*)(buf_get_ptr_ptrs(&ctx->tmps)[i-1]);
      if (!ria_get_datatype_from_buf(&type, pb))
         return false;
      if (type == ria_unknown) {
         *ptr = pb;
         break; 
      }
   }

   /*
    * Create new if not found
    *
    */
   if (*ptr == NULL) {
      byte* p;
      buf_t* pb;
      i = buf_get_length(&ctx->tmps);
      if (!buf_expand(i+1, &ctx->tmps))
         return false;
      if (!buf_set_length(i+1, &ctx->tmps))
         return false;
      if (!heap_alloc((void**)&pb, sizeof(buf_t), ctx->mem))
         return false;
      if (!buf_create(1, 1, 0, pb, ctx->mem))
         return false;
      if (!ria_prealloc_datatype_in_buf(&p, ria_unknown, 0, pb))
         return false;
      buf_get_ptr_ptrs(&ctx->tmps)[i] = (byte*)pb;
      *ptr = pb;
   }

   return true;

}

/*****************************************************************************/
bool 
   ria_push_to_stack(                                            
      ria_data_kind_t     IN       kind,
      void*               IN       ptr,
      ria_exec_state_t*   IN OUT   ctx)
/*
 * Pushes data to stack
 *
 */
{

   usize c;
   byte** pp;

   assert(ptr != NULL);
   assert(ctx != NULL);

   c = buf_get_length(&ctx->stack);
   if (c & 0x01)
      ERR_SET(err_internal);
   if (!buf_expand(c+2, &ctx->stack))
      return false;

   pp = buf_get_ptr_ptrs(&ctx->stack);
   pp[c+0] = (byte*)(usize)kind;
   pp[c+1] = ptr;
   return buf_set_length(c+2, &ctx->stack);

}

/*****************************************************************************/
bool 
   ria_pop_from_stack(    
      ria_type_t*         OUT      type,
      ria_data_kind_t*    OUT      kind,
      void**              OUT      ptr,
      ria_exec_state_t*   IN OUT   ctx)
/*
 * Pushes data to stack
 *
 */
{

   usize c;
   byte** pp;

   assert(kind != NULL);
   assert(ptr  != NULL);
   assert(ctx  != NULL);

   c = buf_get_length(&ctx->stack);
   if ((c == 0) || (c & 0x01))
      ERR_SET(err_internal);

   pp = buf_get_ptr_ptrs(&ctx->stack);
   *kind = (ria_data_kind_t)(usize)pp[c-2];
   *ptr  = pp[c-1];
   if (*ptr == NULL)
      ERR_SET(err_internal);

   /*
    * Detect type
    *
    */
   switch (*kind) {
   case ria_data_var:
   case ria_data_tmp:
      if (!ria_get_datatype_from_buf(type, (buf_t*)*ptr))
         ERR_SET(err_internal);
      break;
   case ria_data_int:
      *type = ria_int;
      break;         
   default:
      *type = ria_string;
   }

   if (c == 2)
      return buf_destroy(&ctx->stack);
   else
      return buf_set_length(c-2, &ctx->stack);

}

/*****************************************************************************/
bool 
   ria_get_operand_info( 
      ria_type_t*         OUT      type,
      byte**              OUT      p,
      usize*              OUT      c,
      ria_data_kind_t     IN       kind,
      void*               IN       oper,
      ria_exec_state_t*   IN OUT   ctx)
/*
 * Returns operand info
 *
 */
{

   assert(type != NULL);
   assert(p    != NULL);
   assert(c    != NULL);
   ctx;

   switch (kind) {
   case ria_data_var:
   case ria_data_tmp:
      if (!ria_get_datainfo_from_buf(type, p, c, (buf_t*)oper))
         return false;
      break;
   case ria_data_str:
   case ria_data_par:
      *p = (byte*)oper;
      *c = StrLen((char*)*p) + 1;
      *type = ria_string;
      break;
   case ria_data_int:
      *p = (byte*)oper + 1;
      *c = (*((byte*)oper) & 0x03) + 1;
      *type = ria_int;      
      break;   
   default:
      ERR_SET(err_internal);
   }

   return true;

}


/******************************************************************************
 *  Execution 
 */

/*****************************************************************************/
bool 
   ria_copy(                                            
      ria_data_kind_t     IN       kind3,
      void*               IN       data3,
      ria_data_kind_t     IN       kind1,
      void*               IN       data1,
      ria_exec_state_t*   IN OUT   ctx)
/*
 * Copies data
 *
 */
{

   buf_t* pr;
   byte*  p1;
   byte*  p3;
   usize  c1;
   ria_type_t type;

   assert(data3 != NULL);
   assert(data1 != NULL);
   assert(ctx   != NULL);
   ctx;

   /*
    * Get source
    *
    */
   if (!ria_get_operand_info(&type, &p1, &c1, kind1, data1, ctx))
      return false;

   /*
    * Prepare result
    *
    */
   switch (kind3) {
   case ria_data_var:
   case ria_data_tmp:
      pr = (buf_t*)data3;
      if (!ria_prealloc_datatype_in_buf(&p3, type, c1, pr))
         return false;
      MemMove(p3, p1, c1);
      break;
   case ria_data_res:
      pr = (buf_t*)data3;
      if (type == ria_string) {
         if (c1 == 0)
            ERR_SET(err_internal);
         c1--;    
      }
      if (!buf_load(p1, 0, c1, pr))
         return false;
      break;
   default:
      ERR_SET(err_internal);
   }

   return true;

}

/*****************************************************************************/
bool 
   ria_concat(                                            
      ria_data_kind_t     IN       kind3,
      void*               IN       data3,
      ria_data_kind_t     IN       kind1,
      void*               IN       data1,
      ria_data_kind_t     IN       kind2,
      void*               IN       data2,
      ria_exec_state_t*   IN OUT   ctx)
/*
 * Concatenates string data
 *
 */
{

   buf_t* pr;
   byte*  p1;
   byte*  p2;
   byte*  p3;
   usize  c1;
   usize  c2;
   ria_type_t type;

   assert(data3 != NULL);
   assert(data2 != NULL);
   assert(data1 != NULL);
   assert(ctx   != NULL);
   ctx;

   /*
    * Get operands
    *
    */
   if (!ria_get_operand_info(&type, &p1, &c1, kind1, data1, ctx))
      return false;
   if (type != ria_string)
      ERR_SET(err_internal);
   if (!ria_get_operand_info(&type, &p2, &c2, kind2, data2, ctx))
      return false;
   if (type != ria_string)
      ERR_SET(err_internal);

   /*
    * Prepare result
    *
    */
   switch (kind3) {
   case ria_data_var:
   case ria_data_tmp:
      pr = (buf_t*)data3;
      /* Operand size includes zero terminator */
      c1--, c2--;
      if (!ria_prealloc_datatype_in_buf(&p3, ria_string, c1+c2+1, pr))
         return false;
      MemMove(p3, p1, c1);
      p3 += c1;
      MemMove(p3, p2, c2);
      p3[c2] = 0x00;
      break;
   default:
      ERR_SET(err_internal);
   }

   return true;

}

/*****************************************************************************/
bool 
   ria_math(                                            
      ria_data_kind_t     IN       kind3,
      void*               IN       data3,
      ria_data_kind_t     IN       kind1,
      void*               IN       data1,
      ria_data_kind_t     IN       kind2,
      void*               IN       data2,
      char                IN       oper,
      ria_exec_state_t*   IN OUT   ctx)
/*
 * Makes math over integers
 *
 */
{

   buf_t*  pr;
   byte*   p1 = NULL;
   byte*   p2 = NULL;
   byte*   p3 = NULL;
   usize   c1 = 0;
   usize   c2 = 0;
   unumber u1 = 0;
   unumber u2 = 0;
   bool    unary = false;
   ria_type_t type;

   assert(data3 != NULL);
   assert(data1 != NULL);
   assert(ctx   != NULL);
   ctx;

   /*
    * Get operands
    *
    */
   if (!ria_get_operand_info(&type, &p1, &c1, kind1, data1, ctx))
      return false;
   if (type != ria_int)
      ERR_SET(err_internal);
   if (data2 == NULL)
      unary = true;
   else {      
      if (!ria_get_operand_info(&type, &p2, &c2, kind2, data2, ctx))
         return false;
      if (type != ria_int)
         ERR_SET(err_internal);
   }
   if (unary)
      switch (oper) {
      case '-':
         break;
      default:
         ERR_SET(err_internal);
      }         

   /*
    * Prepare result
    *
    */
   switch (kind3) {
   case ria_data_var:
   case ria_data_tmp:
      pr = (buf_t*)data3;
      if (!ria_prealloc_datatype_in_buf(&p3, ria_int, 4, pr))
         return false;
      OS_B_U_COUNT(u1, p1, c1);  
      if (!unary) 
         OS_B_U_COUNT(u2, p2, c2);   
      switch (oper) {
      case '-':
         if (unary)
            u1 = -(signed)u1;
         else   
            u1 -= u2;
         break;
      case '+':
         u1 += u2;
         break;
      default:
         ERR_SET(err_internal);
      }
      U_OS_B_COUNT(p3, 4, u1);
      break;
   default:
      ERR_SET(err_internal);
   }

   return true;

}

/*****************************************************************************/
bool 
   ria_bool(                                            
      ria_data_kind_t     IN       kind3,
      void*               IN       data3,
      ria_data_kind_t     IN       kind1,
      void*               IN       data1,
      ria_data_kind_t     IN       kind2,
      void*               IN       data2,
      char                IN       oper,
      ria_exec_state_t*   IN OUT   ctx)
/*
 * Makes math over booleans
 *
 */
{

   buf_t*  pr;
   byte*   p1 = NULL;
   byte*   p2 = NULL;
   byte*   p3 = NULL;
   usize   c1 = 0;
   usize   c2 = 0;
   bool    unary = false;
   ria_type_t type;

   assert(data3 != NULL);
   assert(data1 != NULL);
   assert(ctx   != NULL);
   ctx;

   /*
    * Get operands
    *
    */
   if (!ria_get_operand_info(&type, &p1, &c1, kind1, data1, ctx))
      return false;
   if (type != ria_boolean)
      ERR_SET(err_internal);
   if (data2 == NULL)
      unary = true;
   else {      
      if (!ria_get_operand_info(&type, &p2, &c2, kind2, data2, ctx))
         return false;
      if (type != ria_boolean)
         ERR_SET(err_internal);
   }
   if (unary)
      switch (oper) {
      case '!':
         break;
      default:
         ERR_SET(err_internal);
      }         

   /*
    * Prepare result
    *
    */
   switch (kind3) {
   case ria_data_var:
   case ria_data_tmp:
      pr = (buf_t*)data3;
      if (!ria_prealloc_datatype_in_buf(&p3, ria_boolean, 1, pr))
         return false;
      switch (oper) {
      case '!':
         *p3 = !(*p1);
         break;
      case '|':
         *p3 = (*p1) || (*p2);
         break;
      case '&':
         *p3 = (*p1) && (*p2);
         break;
      default:
         ERR_SET(err_internal);
      }
      break;
   default:
      ERR_SET(err_internal);
   }

   return true;

}

/*****************************************************************************/
bool 
   ria_cmp(                          
      ria_data_kind_t     IN       kind3,
      void*               IN       data3,
      ria_data_kind_t     IN       kind1,
      void*               IN       data1,
      ria_data_kind_t     IN       kind2,
      void*               IN       data2,
      byte                IN       opcode,
      ria_exec_state_t*   IN OUT   ctx)
/*
 * Compares two enitites
 *
 */
{

   buf_t* pr;
   byte*  p1;
   byte*  p2;
   byte*  p3;
   usize  c1;
   usize  c2;
   ria_type_t type1;
   ria_type_t type2;
   int i; 

   assert(data3 != NULL);
   assert(data2 != NULL);
   assert(data1 != NULL);
   assert(ctx   != NULL);
   ctx;

   /*
    * Get operands
    *
    */
   if (!ria_get_operand_info(&type1, &p1, &c1, kind1, data1, ctx))
      return false;
   if (!ria_get_operand_info(&type2, &p2, &c2, kind2, data2, ctx))
      return false;
   if (type1 != type2)
      ERR_SET(err_internal);

   /*
    * Prepare result
    *
    */
   switch (kind3) {
   case ria_data_var:
   case ria_data_tmp:
      pr = (buf_t*)data3;
      if (!ria_prealloc_datatype_in_buf(&p3, ria_boolean, 1, pr))
         return false;
      if (type1 == ria_int) {
         i = 0;
         if (c1 < c2) {
            for (; c1<c2; c2--, p2++) 
               if (p2[0] != 0) {
                  i = -1;
                  break;
               }
         }
         else {
            for (; c2<c1; c1--, p1++) 
               if (p1[0] != 0) {
                  i = 1;
                  break;
               }
         }
         if (i == 0)
            i = MemCmp(p1, p2, c1);
      }
      else 
         if (c1 < c2) {
            i = MemCmp(p1, p2, c1);
            i = (i == 0) ? -1 : i;
         }
         else {
            i = MemCmp(p1, p2, c2);
            if (c1 > c2) 
               i = (i == 0) ? 1 : i;
         }
      switch (opcode) {
      case ria_opcode_less:
         *p3 = (i <  0) ? true : false;
         break;
      case ria_opcode_more:
         *p3 = (i >  0) ? true : false;
         break;
      case ria_opcode_less_eq:
         *p3 = (i <= 0) ? true : false;
         break;
      case ria_opcode_more_eq:
         *p3 = (i >= 0) ? true : false;
         break;
      case ria_opcode_eq:
         *p3 = (i == 0) ? true : false;
         break;
      case ria_opcode_not_eq:
         *p3 = (i != 0) ? true : false;
         break;
      }
      break;
   default:
      ERR_SET(err_internal);
   }

   return true;

}

/*****************************************************************************/
bool 
   ria_invoke_func(                          
      void*               IN       res,
      function_fn*        IN       impl,
      byte*               IN       ppar,
      usize               IN       cpar,
      usize               IN       cunwind,
      ria_type_t          IN       restype,
      ria_exec_state_t*   IN OUT   ctx)
/*
 * Invokes predefined function
 *
 */
{

   buf_t* pt;
   usize  c;
   umask  flags;

   assert(res  != NULL);
   assert(impl != NULL);
   assert(ctx  != NULL);
   assert((cpar == 0) || (ppar != NULL));

   /*
    * Invoke 
    *
    */
   pt = (buf_t*)res;
   flags = 0;
   if (!(*impl)(pt, &flags, ppar, cpar, ctx)) 
      return false;
      
#ifdef USE_RIA_ASYNC_CALLS   
   /*
    * Check pending execution
    *
    */
   if (ctx->status == ria_exec_pending) {
      /*
       * Save state
       *
       */
      ctx->pending.resbuf  = res;
      ctx->pending.restype = restype; 
      ctx->pending.ppar    = ppar;
      ctx->pending.cpar    = cpar;
      ctx->pending.cunwind = cunwind;
      ctx->pending.func    = impl;
      return true;
   }      
#endif      

   /*
    * Unwind the stack
    *
    */
   c = buf_get_length(&ctx->stack);
   if ((c & 0x01) || (c < cunwind*2))
      ERR_SET(err_internal);
   if (!buf_set_length(c-2*cunwind, &ctx->stack))
      return false;

   /*
    * Fix result if needed
    *
    */
   if (flags & ria_func_dst_ready)
      return true;
   c = buf_get_length(pt);
   if (!buf_expand(c+1, pt))
      return false;
   if (!buf_load(buf_get_ptr_bytes(pt), 1, c, pt))
      return false;
   return ria_set_datatype_into_buf(pt, restype);

}

/*****************************************************************************/
bool 
   ria_call(                          
      ria_data_kind_t     IN       kind3,
      void*               IN       data3,
      byte                IN       b0,
      byte                IN       b1,
      ria_exec_state_t*   IN OUT   ctx)
/*
 * Calls predefined function
 *
 */
{

   usize k, c;
   unumber i, j;
   function_fn* impl;
   byte** pp;
   byte*  p;
   ria_type_t type;

   assert(data3 != NULL);
   assert(ctx   != NULL);

   /*
    * Detect function 
    *
    */
   i = (b1 == 0x00) ? b0 : (b0 << 8) | b1;
   if (!ria_get_function_info(&type, &j, &impl, i))
      return false;

   /*
    * Check output 
    *
    */
   switch (kind3) {
   case ria_data_var:
   case ria_data_tmp:
      break;
   default:
      ERR_SET(err_internal);
   }

   /*
    * Repack parameters
    *
    */
   c = buf_get_length(&ctx->stack);
   if ((c & 0x01) || (c < j*2))
      ERR_SET(err_internal);
   pp = buf_get_ptr_ptrs(&ctx->stack) + c - 2*j;
   for (i=0, p=(byte*)pp, k=2*sizeof(void*); i<j; i++, k+=2*sizeof(void*)) 
      if (!ria_pack_func_param(&p, &k, (ria_data_kind_t)(usize)pp[i*2], pp[i*2+1]))
         return false;

   /*
    * Invoke 
    *
    */
   return ria_invoke_func(data3, impl, (byte*)pp, p-(byte*)pp, j, type, ctx);

}

/*****************************************************************************/
bool 
   ria_execute_command(                                            
      ria_exec_state_t*   IN OUT   ctx)
/*
 * Executes single command
 *
 */
{

   void* pd1 = NULL;
   void* pd2 = NULL;
   void* pd3 = NULL;
   ria_data_kind_t kind1 = 0;
   ria_data_kind_t kind2 = 0;
   ria_data_kind_t kind3 = 0;
   ria_type_t type1;
   ria_type_t type2;
   byte b0, b1 = 0;
   byte* pd;
   usize u;

   assert(ctx != NULL);

   if (ctx->cexec < 1) {
      SET_EXECUTE_ERROR(ctx);
      return true;
   }

   RIA_TRACE_START;

   switch (ctx->pexec[0]) {

   /* Push */
   case ria_opcode_pushi4:
      if (ctx->cexec < 5) {
         SET_EXECUTE_ERROR(ctx);
         return true;
      }
   case ria_opcode_pushi3:
      if (ctx->cexec < 4) {
         SET_EXECUTE_ERROR(ctx);
         return true;
      }
   case ria_opcode_pushs2:
   case ria_opcode_pushi2:
      if (ctx->cexec < 3) {
         SET_EXECUTE_ERROR(ctx);
         return true;
      }
   case ria_opcode_pushv:
   case ria_opcode_pushs:
   case ria_opcode_pushp:
   case ria_opcode_pushi1:
      if (ctx->cexec < 2) {
         SET_EXECUTE_ERROR(ctx);
         return true;
      }
      switch (ctx->pexec[0]) {
      case ria_opcode_pushv:
         RIA_TRACE_MSG("PUSHV");
         if (!ria_get_var(&pd1, ctx->pexec[1], false, ctx))
            return false;
         kind1 = ria_data_var;
         break;
      case ria_opcode_pushs:
         RIA_TRACE_MSG("PUSHS");
         if (!ria_get_str(&pd1, ctx->pexec[1], ctx))
            return false;
         kind1 = ria_data_str;
         break;
      case ria_opcode_pushs2:
         RIA_TRACE_MSG("PUSHS2");
         if (!ria_get_str(&pd1, (ctx->pexec[1]<<8)|ctx->pexec[0], ctx))
            return false;
         kind1 = ria_data_str;
         break;
      case ria_opcode_pushp:
         RIA_TRACE_MSG("PUSHP");
         if (!ria_get_par(&pd1, ctx->pexec[1], ctx))
            return false;
         kind1 = ria_data_par;
         break;
      case ria_opcode_pushi1:
      case ria_opcode_pushi2:
      case ria_opcode_pushi3:
      case ria_opcode_pushi4:
         RIA_TRACE_MSG("PUSHI(X)");
         pd1 = (byte*)ctx->pexec;
         kind1 = ria_data_int;
         break;
      }
      if (pd1 == NULL) {
         SET_EXECUTE_ERROR(ctx);
         return true;
      }            
      if (!ria_push_to_stack(kind1, pd1, ctx))
         return false;
      switch (ctx->pexec[0]) {
      case ria_opcode_pushi4:
         ctx->pexec += 5;
         ctx->cexec -= 5;
         break;
      case ria_opcode_pushi3:
         ctx->pexec += 4;
         ctx->cexec -= 4;
         break;
      case ria_opcode_pushi2:
      case ria_opcode_pushs2:
         ctx->pexec += 3;
         ctx->cexec -= 3;
         break;
      default:
         ctx->pexec += 2;
         ctx->cexec -= 2;
      }
      break;

   /* Pop */
   case ria_opcode_pop:
      RIA_TRACE_MSG("POP");
      if (!ria_pop_from_stack(&type1, &kind1, &pd1, ctx))
         return false;
      if (!ria_get_var(&pd3, ctx->pexec[1], true, ctx))
         return false;
      if (pd3 == NULL) {
         SET_EXECUTE_ERROR(ctx);
         return true;
      }
      if (!ria_copy(ria_data_var, pd3, kind1, pd1, ctx))
         return false;
      if (kind1 == ria_data_tmp)
         if (!ria_set_datatype_into_buf((buf_t*)pd1, ria_unknown))
            return false;
      ctx->pexec += 2;
      ctx->cexec -= 2;
      break;

   /* Unary arithmetics*/
   case ria_opcode_neg:
      RIA_TRACE_MSG("NEG");
      if (!ria_pop_from_stack(&type1, &kind1, &pd1, ctx))
         return false;
      if (!ria_get_tmp(&pd3, ctx))
         return false;
      kind3 = ria_data_tmp;
      if (type1 == ria_int) 
         switch (ctx->pexec[0]) {
         case ria_opcode_neg:
            if (!ria_math(kind3, pd3, kind1, pd1, ria_data_tmp, NULL, '-', ctx))
               return false;
            break;
         default:
            ERR_SET(err_internal);
         }
      else
         ERR_SET(err_internal);
      if (kind1 == ria_data_tmp)
         if (!ria_set_datatype_into_buf((buf_t*)pd1, ria_unknown))
            return false;
      if (!ria_push_to_stack(kind3, pd3, ctx))
         return false;
      ctx->pexec++;
      ctx->cexec--;
      break;

   /* Binary arithmetics*/
   case ria_opcode_add: /* or LOR  */
   case ria_opcode_sub: /* or LAND */
      if (!ria_pop_from_stack(&type2, &kind2, &pd2, ctx))
         return false;
      if (!ria_pop_from_stack(&type1, &kind1, &pd1, ctx))
         return false;
      if (type1 != type2)
         ERR_SET(err_internal);
      if (!ria_get_tmp(&pd3, ctx))
         return false;
      kind3 = ria_data_tmp;
      if (type1 == ria_string) 
         switch (ctx->pexec[0]) {
         case ria_opcode_add:
            RIA_TRACE_MSG("CONCAT");
            if (!ria_concat(kind3, pd3, kind1, pd1, kind2, pd2, ctx))
               return false;
            break;
         default:
            ERR_SET(err_internal);
         }
      else 
      if (type1 == ria_int) 
         switch (ctx->pexec[0]) {
         case ria_opcode_add:
            RIA_TRACE_MSG("+");
            if (!ria_math(kind3, pd3, kind1, pd1, kind2, pd2, '+', ctx))
               return false;
            break;
         case ria_opcode_sub:
            RIA_TRACE_MSG("-");
            if (!ria_math(kind3, pd3, kind1, pd1, kind2, pd2, '-', ctx))
               return false;
            break;
         default:
            ERR_SET(err_internal);
         }
      else
      if (type1 == ria_boolean) 
         switch (ctx->pexec[0]) {
         case ria_opcode_land:
            RIA_TRACE_MSG("&&");
            if (!ria_bool(kind3, pd3, kind1, pd1, kind2, pd2, '&', ctx))
               return false;
            break;
         case ria_opcode_lor:
            RIA_TRACE_MSG("||");
            if (!ria_bool(kind3, pd3, kind1, pd1, kind2, pd2, '|', ctx))
               return false;
            break;
         default:
            ERR_SET(err_internal);
         }
      else
         ERR_SET(err_internal);
      if (kind1 == ria_data_tmp)
         if (!ria_set_datatype_into_buf((buf_t*)pd1, ria_unknown))
            return false;
      if (kind2 == ria_data_tmp)
         if (!ria_set_datatype_into_buf((buf_t*)pd2, ria_unknown))
            return false;
      if (!ria_push_to_stack(kind3, pd3, ctx))
         return false;
      ctx->pexec++;
      ctx->cexec--;
      break;

   /* Ret */
   case ria_opcode_ret:
      if (!ria_pop_from_stack(&type1, &kind1, &pd1, ctx))
         return false;
      if (!ria_copy(ria_data_res, &ctx->result, kind1, pd1, ctx))
         return false;
      if (kind1 == ria_data_tmp)
         if (!ria_set_datatype_into_buf((buf_t*)pd1, ria_unknown))
            return false;
   case ria_opcode_retn:
      if (ctx->pexec[0] == ria_opcode_retn)
         buf_set_length(0, &ctx->result);   
      ctx->status = ria_exec_ok;
      ctx->pexec++;
      ctx->cexec--;
      break;

   /* Calls */
   case ria_opcode_call2i:
   case ria_opcode_call2p:
      if (ctx->cexec < 3) {
         SET_EXECUTE_ERROR(ctx);
         return true;
      }
      b1 = ctx->pexec[2];
   case ria_opcode_calli:
   case ria_opcode_callp:
      if (ctx->cexec < 2) {
         SET_EXECUTE_ERROR(ctx);
         return true;
      }
      b0 = ctx->pexec[1];
      if (!ria_get_tmp(&pd3, ctx))
         return false;
      kind3 = ria_data_tmp;
      RIA_TRACE_MSG("CALL ");
      RIA_TRACE_INT((b1 == 0x00) ? b0 : (b0 << 8) | b1);
      if (!ria_call(kind3, pd3, b0, b1, ctx))
         return false;
      switch (ctx->pexec[0]) {   
      case ria_opcode_callp:
      case ria_opcode_call2p:
         if (!ria_push_to_stack(kind3, pd3, ctx))
            return false;
      }            
      if (b1 == 0) {
         ctx->pexec += 2;
         ctx->cexec -= 2;
      }
      else {
         ctx->pexec += 3;
         ctx->cexec -= 3;
      }
      break;

   /* Comparations */
   case ria_opcode_less:
   case ria_opcode_more:
   case ria_opcode_less_eq:
   case ria_opcode_more_eq:
   case ria_opcode_eq:
   case ria_opcode_not_eq:
      switch (ctx->pexec[0]) {
      case ria_opcode_less:
         RIA_TRACE_MSG("<");
         break;
      case ria_opcode_more:
         RIA_TRACE_MSG(">");
         break;
      case ria_opcode_less_eq:
         RIA_TRACE_MSG("<=");
         break;
      case ria_opcode_more_eq:
         RIA_TRACE_MSG(">=");
         break;
      case ria_opcode_eq:
         RIA_TRACE_MSG("==");
         break;
      case ria_opcode_not_eq:
         RIA_TRACE_MSG("!=");
         break;
      }
      if (!ria_pop_from_stack(&type2, &kind2, &pd2, ctx))
         return false;
      if (!ria_pop_from_stack(&type1, &kind1, &pd1, ctx))
         return false;
      if (type1 != type2)
         ERR_SET(err_internal);
      if (!ria_get_tmp(&pd3, ctx))
         return false;
      kind3 = ria_data_tmp;
      if (!ria_cmp(kind3, pd3, kind1, pd1, kind2, pd2, ctx->pexec[0], ctx))
         return false;
      if (kind1 == ria_data_tmp)
         if (!ria_set_datatype_into_buf((buf_t*)pd1, ria_unknown))
            return false;
      if (kind2 == ria_data_tmp)
         if (!ria_set_datatype_into_buf((buf_t*)pd2, ria_unknown))
            return false;
      if (!ria_push_to_stack(kind3, pd3, ctx))
         return false;
      ctx->pexec++;
      ctx->cexec--;
      break;

   /* Conditional jumps */
   case ria_opcode_jif:
   case ria_opcode_jif2:
   case ria_opcode_jit:
   case ria_opcode_jit2:
      if (!ria_pop_from_stack(&type1, &kind1, &pd1, ctx))
         return false;
      if (type1 != ria_boolean)
         ERR_SET(err_internal);
      if (!ria_get_operand_info(&type1, &pd, &u, kind1, pd1, ctx))
         return false;
      if (u != 1)
         ERR_SET(err_internal);
      switch (ctx->pexec[0]) {
      case ria_opcode_jif:
         u = (int8)(((*pd == 0) ? ctx->pexec[1] : 0));
         u = ((int)u < 0) ? u : u+2;
         break;
      case ria_opcode_jit:
         u = (int8)(((*pd != 0) ? ctx->pexec[1] : 0));
         u = ((int)u < 0) ? u : u+2;
         break;
      case ria_opcode_jif2:
         u = (int16)(((*pd == 0) ? (ctx->pexec[1] << 8) | ctx->pexec[2] : 0));
         u = ((int)u < 0) ? u : u+3;
         break;
      case ria_opcode_jit2:
         u = (int16)(((*pd != 0) ? (ctx->pexec[1] << 8) | ctx->pexec[2] : 0));
         u = ((int)u < 0) ? u : u+3;
         break;
      }
      ctx->pexec += (int)u;
      ctx->cexec -= (int)u;
      break;

   /* Unconditional jumps */
   case ria_opcode_jmp:
      u = (int8)(ctx->pexec[1]);
      u = ((int)u < 0) ? u : u+2;
      ctx->pexec += (int)u;
      ctx->cexec -= (int)u;
      break;
   case ria_opcode_jmp2:
      u = (int16)(((ctx->pexec[1] << 8) | ctx->pexec[2]));
      u = ((int)u < 0) ? u : u+3;
      ctx->pexec += (int)u;
      ctx->cexec -= (int)u;
      break;

   default:
      ERR_SET(err_internal);
   }

   RIA_TRACE_MSG("\n");
   RIA_TRACE_STOP;

/*
   ria_opcode_add     = 0x20,
   ria_opcode_lor     = 0x20,
   ria_opcode_sub     = 0x27,
   ria_opcode_mul     = 0x28,
   ria_opcode_land    = 0x28,
   ria_opcode_div     = 0x29,
   ria_opcode_rem     = 0x2A,
   ria_opcode_band    = 0x2B,
   ria_opcode_bor     = 0x2C,
   ria_opcode_xor     = 0x2D,
   ria_opcode_bneg    = 0x2E,
   ria_opcode_not     = 0x2E,
   ria_opcode_neg     = 0x2F,
*/

   return true;

}


/******************************************************************************
 *  Execution state
 */

enum {
   cf_ria_exec_state_params  = 0x001,
   cf_ria_exec_state_vars    = 0x002,
   cf_ria_exec_state_stack   = 0x004,
   cf_ria_exec_state_tmps    = 0x008,
   cf_ria_exec_state_result  = 0x010,
   cf_ria_exec_state_http    = 0x020,
   cf_ria_exec_state_globals = 0x040,
   cf_ria_exec_state_parser  = 0x080
};

/*****************************************************************************/
bool 
   ria_exec_state_clean_locals(     
      ria_exec_state_t*   IN OUT   ctx)
/*
 * Cleans local state 
 *
 */
{

   bool ret = true;
   usize i;

   assert(ctx != NULL);

   if (ctx->cleanup & cf_ria_exec_state_vars) {
      for (i=buf_get_length(&ctx->vars); i>0; i--) {
         buf_t* pb = (buf_t*)(buf_get_ptr_ptrs(&ctx->vars)[i-1]);
         if (pb == NULL)
            continue;
         ret = buf_destroy(pb) && ret;
         ret = heap_free(pb, ctx->mem) && ret;
      }
      ret = buf_destroy(&ctx->vars) && ret;
   }
   if (ctx->cleanup & cf_ria_exec_state_tmps) {
      for (i=buf_get_length(&ctx->tmps); i>0; i--) {
         buf_t* pb = (buf_t*)(buf_get_ptr_ptrs(&ctx->tmps)[i-1]);
         if (pb == NULL)
            continue;
         ret = buf_destroy(pb) && ret;
         ret = heap_free(pb, ctx->mem) && ret;
      }
      ret = buf_destroy(&ctx->tmps) && ret;
   }
   if (ctx->cleanup & cf_ria_exec_state_stack)
      ret = buf_destroy(&ctx->stack) && ret;
   if (ctx->cleanup & cf_ria_exec_state_result)
      ret = buf_destroy(&ctx->result) && ret;

   return ret;

}

/*****************************************************************************/
bool 
   ria_exec_state_destroy(     
      ria_exec_state_t*   IN OUT   ctx)
/*
 * Destroys execution state context
 *
 */
{

   bool ret = true;

   assert(ctx != NULL);

   if (ctx->cleanup & cf_ria_exec_state_globals)
      ret = buf_destroy(&ctx->globals) && ret;
   if (ctx->cleanup & cf_ria_exec_state_http)
      ret = ria_http_destroy(&ctx->http) && ret;
   if (ctx->cleanup & cf_ria_exec_state_params)
      ret = buf_destroy(&ctx->params) && ret;
   if (ctx->cleanup & cf_ria_exec_state_parser)
      ret = ria_parser_destroy(&ctx->parser) && ret;
   ret = ria_exec_state_clean_locals(ctx);

   ctx->cleanup = 0;
   return ret;

}

/*****************************************************************************/
bool 
   ria_exec_state_create(     
      ria_exec_state_t*   IN OUT   ctx,
      ria_config_t*       IN       config, 
      heap_ctx_t*         IN OUT   mem)
/*
 * Creates execution state context
 *
 */
{

   assert(ctx != NULL);
   assert(mem != NULL);

   MemSet(ctx, 0x00, sizeof(*ctx));
   ctx->mem = mem;

   if (!buf_create(sizeof(byte), 0, 0, &ctx->params, ctx->mem))
      goto failed;
   else
      ctx->cleanup |= cf_ria_exec_state_params;
   if (!buf_create(sizeof(byte*), 0, 0, &ctx->vars, ctx->mem))
      goto failed;
   else
      ctx->cleanup |= cf_ria_exec_state_vars;
   if (!buf_create(sizeof(byte*), 0, 0, &ctx->stack, ctx->mem))
      goto failed;
   else
      ctx->cleanup |= cf_ria_exec_state_stack;
   if (!buf_create(sizeof(byte*), 0, 0, &ctx->tmps, ctx->mem))
      goto failed;
   else
      ctx->cleanup |= cf_ria_exec_state_tmps;
   if (!buf_create(sizeof(byte), 0, 0, &ctx->result, ctx->mem))
      goto failed;
   else
      ctx->cleanup |= cf_ria_exec_state_result;
   if (!buf_create(sizeof(byte*), 0, 0, &ctx->globals, ctx->mem))
      goto failed;
   else
      ctx->cleanup |= cf_ria_exec_state_globals;
   if (!ria_http_create(&ctx->http, config, ctx->mem))
      goto failed;
   else
      ctx->cleanup |= cf_ria_exec_state_http;
   if (!ria_parser_create(&ctx->parser, ctx->mem))
      goto failed;
   else
      ctx->cleanup |= cf_ria_exec_state_parser;
   return true;

failed:
   ria_exec_state_destroy(ctx);
   return false;

}


/******************************************************************************
 *  Configuration
 */

enum {
   cf_ria_config_workdir = 0x01,
   cf_ria_config_tempdir = 0x02
};

/*****************************************************************************/
bool 
   ria_config_destroy(     
      ria_config_t*   IN OUT   ctx)
/*
 * Destroys configuration context
 *
 */
{

   bool ret = true;

   assert(ctx != NULL);

   if (ctx->cleanup & cf_ria_config_workdir)
      ret = buf_destroy(&ctx->workdir) && ret;
   if (ctx->cleanup & cf_ria_config_tempdir)
      ret = buf_destroy(&ctx->tempdir) && ret;

   ctx->cleanup = 0;
   return ret;

}

/*****************************************************************************/
bool 
   ria_config_create(     
      ria_config_t*   IN OUT   ctx,
      heap_ctx_t*     IN OUT   mem)
/*
 * Creates configuration context
 *
 */
{

   assert(ctx != NULL);
   assert(mem != NULL);

   MemSet(ctx, 0x00, sizeof(*ctx));

   if (!buf_create(sizeof(byte), 0, 0, &ctx->workdir, mem))
      goto failed;
   else
      ctx->cleanup |= cf_ria_config_workdir;
   if (!buf_create(sizeof(byte), 0, 0, &ctx->tempdir, mem))
      goto failed;
   else
      ctx->cleanup |= cf_ria_config_tempdir;
   return true;

failed:
   ria_config_destroy(ctx);
   return false;

}


/******************************************************************************
 *  API
 */

enum {
   cf_ria_executor_state  = 0x01,
   cf_ria_executor_config = 0x02
};

/*****************************************************************************/
bool 
   ria_executor_create(     
      ria_executor_ctx_t*   IN OUT   ctx,
      heap_ctx_t*           IN OUT   mem)
/*
 * Creates execution context
 *
 */
{

   assert(ctx != NULL);
   MemSet(ctx, 0x00, sizeof(*ctx));

   if (!ria_config_create(&ctx->config, mem))
      goto failed;
   else
      ctx->cleanup |= cf_ria_executor_config;
   if (!ria_exec_state_create(&ctx->state, &ctx->config, mem))
      goto failed;
   else
      ctx->cleanup |= cf_ria_executor_state;
   return true;

failed:
   ria_executor_destroy(ctx);
   return false;

}

/*****************************************************************************/
bool 
   ria_executor_destroy(     
      ria_executor_ctx_t*   IN OUT   ctx)
/*
 * Destroys execution context
 *
 */
{

   bool ret = true;

   assert(ctx != NULL);

   if (ctx->cleanup & cf_ria_executor_state)
      ret = ria_exec_state_destroy(&ctx->state) && ret;
   if (ctx->cleanup & cf_ria_executor_config)
      ret = ria_config_destroy(&ctx->config) && ret;

   ctx->cleanup = 0;
   return ret;

}

/*****************************************************************************/
bool 
   ria_set_option(
      ria_option_t          IN       option,
      const byte*           IN       pval,
      usize                 IN       cval,
      ria_executor_ctx_t*   IN OUT   ctx)
/*
 * Sets execution option
 *
 */
{

   buf_t* dst;

#if defined(WIN32_APP)
   static const char _slash = '\\';
#elif defined(ANDROID)   
   static const char _slash = '/';
#elif defined(WISE12)  
   static const char _slash = '/';
#else
#error Target platform is not defined   
#endif

   assert(pval != NULL);
   assert(ctx  != NULL);
   
   switch (option) {
   case ria_option_workdir:
      dst = &ctx->config.workdir;
      break;
   case ria_option_tempdir:
      dst = &ctx->config.tempdir;
      break;
   default:
      ERR_SET(err_bad_param);   
   }

   if (!buf_load(pval, 0, cval, dst))
      return false;
   if (pval[cval-1] == _slash)
      return true;   
   return buf_append(&_slash, 1, dst);

} 

/*****************************************************************************/
bool 
   ria_set_exec_param(
      const char*           IN       param,
      unumber               IN       idx,
      ria_executor_ctx_t*   IN OUT   ctx)
/*
 * Sets parameter for scenario script 
 *
 */
{

   byte *p, *q;
   usize c, l;

   assert(param != NULL);
   assert(ctx   != NULL);

   if (idx >= 0x100)
      ERR_SET(err_bad_param);
      
   /*
    * Delete parameter if already exists
    *
    */
   if (!ria_get_par((void**)&p, idx, &ctx->state))
      return false;
   if (p != NULL) {
      c = StrLen((char*)p);
      l = buf_get_length(&ctx->state.params);
      q = buf_get_ptr_bytes(&ctx->state.params) + l;
      MemMove(p-1, p+c+1, (q-p)-c-1);
      if (!buf_set_length(l-c-2, &ctx->state.params))
         return false;
   }                                              
       
   /*
    * Append new parameter
    *
    */
   c = StrLen(param);
   l = buf_get_length(&ctx->state.params);
   if (!buf_expand(l+c+2, &ctx->state.params))
      return false;
   p = buf_get_ptr_bytes(&ctx->state.params) + l;
   *(p++) = (byte)idx;
   MemCpy(p, param, c);
   p[c] = 0x00;
   return buf_set_length(l+c+2, &ctx->state.params);

}

/*****************************************************************************/
bool 
   ria_execute_script(                                            
      ria_exec_status_t*    OUT      status,
      mem_blk_t*            IN OUT   result,
      const char*           IN       name,
      const mem_blk_t*      IN OUT   module,
      ria_executor_ctx_t*   IN OUT   ctx)
/*
 * Executes compiled scenario script 
 *
 */
{

   usize u, l;

   assert(ctx    != NULL);
   assert(name   != NULL);
   assert(module != NULL);
   assert(result != NULL);
   assert(status != NULL);

   /*
    * Initialize context
    *
    */
#ifdef USE_RIA_ASYNC_CALLS   
   switch (ctx->state.status) {
   case ria_exec_pending:
   case ria_exec_proceed:
      ERR_SET(err_unexpected_call); 
   }      
#endif      
   ctx->state.status = ria_exec_unknown;
   ctx->state.flags  = 0;
   ctx->state.pstart = module->p;
   if (!ria_exec_state_clean_locals(&ctx->state))
      return false;
      
   /*
    * Attach result buffer
    *
    */
   if (!buf_attach(result->p, result->c, result->c, true, &ctx->state.result))
      return false;    

   /*
    * Load header
    *
    */
   if (module->c <= ria_header_size)
      ERR_SET(err_bad_param);
   u = (module->p[1] << 16) | (module->p[2] << 8) | (module->p[3]);
   if (module->c <= ria_header_size + u)
      ERR_SET(err_bad_param);
   ctx->state.ptr_strs = module->p + u;

   /*
    * Find entry point
    *
    */
   l = StrLen(name);
   for (ctx->state.cexec=0, 
        ctx->state.pexec=module->p+ria_header_size; 
        ctx->state.cexec<module->p[0]; 
        ctx->state.cexec++) {
      if (*ctx->state.pexec == l)
         if (!MemCmp(ctx->state.pexec+1, name, l)) {
            ctx->state.cexec = 
                (ctx->state.pexec[l+2] << 16)|
                (ctx->state.pexec[l+3] <<  8)|
                 ctx->state.pexec[l+4];
            ctx->state.pexec = module->p + ctx->state.cexec;
            ctx->state.cexec = u - ctx->state.cexec;
            break;
         }
      ctx->state.pexec += *ctx->state.pexec + ria_fextra_size;
   }
   if (ctx->state.cexec == module->p[0])
      ERR_SET(err_bad_param);

   RIA_TRACE_START;
   RIA_TRACE_MSG("Call execute ");
   RIA_TRACE_STR(name, StrLen(name));
   RIA_TRACE_MSG("\n");
   RIA_TRACE_STOP;

   /*
    * Execute command by command
    *
    */   
   for (; ctx->state.cexec>0; ) {
      if (!ria_execute_command(&ctx->state))
         return false;
      *status = ctx->state.status;  
#ifdef USE_RIA_ASYNC_CALLS   
      if (*status == ria_exec_pending)
         return true;
#endif         
      if (*status == ria_exec_failed)
         return true;
      if (*status == ria_exec_ok)
         break;
   }
   if (ctx->state.status == ria_exec_unknown) {
      ctx->state.status = ria_exec_ok;
      *status = ctx->state.status;  
   }      

   /*
    * Finalize
    *
    */
   if (*status == ria_exec_ok) 
      if (ctx->state.flags & ria_ef_parser_ready)
         *status = ria_exec_ok_parser_ready;
   result->c = buf_get_length(&ctx->state.result);
   return buf_detach(&ctx->state.result);

}

/*****************************************************************************/
#ifdef USE_RIA_ASYNC_CALLS   
bool 
   ria_continue_script(       
      ria_exec_status_t*    OUT      status,
      ria_buf_t*            IN OUT   result,
      ria_executor_ctx_t*   IN OUT   ctx)
/*
 * Continues execution of pending scenario script 
 *
 */
 {

   assert(ctx    != NULL);
   assert(result != NULL);
   assert(status != NULL);

   /*
    * Check for status
    *
    */
   if (ctx->state.status == ria_exec_pending) {
      *status = ria_exec_pending;
      return true;
   }      
   if (ctx->state.status != ria_exec_proceed)
      ERR_SET(err_unexpected_call); 
      
   /*
    * Attach result buffer
    *
    */
   if (!buf_attach(result->p, result->c, result->c, true, &ctx->state.result))
      return false;    
  
   RIA_TRACE_MSG("Call continue\n");
      
   /*
    * Reinvoke interrupted function
    *
    */
   if (!ria_invoke_func(
           ctx->state.pending.resbuf, 
           ctx->state.pending.func, 
           ctx->state.pending.ppar, 
           ctx->state.pending.cpar, 
           ctx->state.pending.cunwind, 
           ctx->state.pending.restype, 
           &ctx->state))
      return false;           
   if (ctx->state.status == ria_exec_pending)
      return true;
   ctx->state.status = ria_exec_unknown;

   /*
    * Proceed execution command by command
    *
    */   
   for (; ctx->state.cexec>0; ) {
      if (!ria_execute_command(&ctx->state))
         return false;
      *status = ctx->state.status;  
      if (*status == ria_exec_pending)
         return true;
      if (*status == ria_exec_failed)
         return true;
      if (*status == ria_exec_ok)
         break;
   }
   if (ctx->state.status == ria_exec_unknown) {
      ctx->state.status = ria_exec_ok;
      *status = ctx->state.status;  
   }      

   /*
    * Finalize
    *
    */
   if (*status == ria_exec_ok) 
      if (ctx->state.flags & ria_ef_parser_ready)
         *status = ria_exec_ok_parser_ready;
   result->c = buf_get_length(&ctx->state.result);
   return buf_detach(&ctx->state.result);
 
 }
 #endif      

/*****************************************************************************/
bool 
   ria_parser_action(       
      mem_blk_t*            IN OUT   result,
      const char*           IN       name,
      usize*                IN OUT   pos,
      ria_executor_ctx_t*   IN OUT   ctx)
/*
 * Executes parser action
 *
 */
{

   static const char _json_excludes[] = 
      "\"\"[]{}";

   ria_parser_rule_t*   rule;
   ria_extract_params_t params;
   
   usize savepos = 0;
   bool ok;
 
   assert(result != NULL);
   assert(name   != NULL);
   assert(pos    != NULL);
   assert(ctx    != NULL);
   
   /*
    * Sanity checks 
    *
    */
   if ((ctx->state.flags & ria_ef_parser_ready) == 0x00)
      ERR_SET(err_unexpected_call);
   if (result->c < 1)
      ERR_SET(err_bad_length);      
      
   /*
    * Find parsing rule
    *
    */
   if (!ria_parser_find_rule(&rule, &ctx->state.parser, name, StrLen(name)))
      return false; 
   if (rule == NULL)
      ERR_SET(err_bad_param);   

   /*
    * Setup parsing 
    *
    */
   params.mode   = ria_string_extract_begin_end; 
   params.pbegin = rule->pbegin;
   params.cbegin = rule->cbegin;
   params.pend   = rule->pend;
   params.cend   = rule->cend;
   params.pos    = pos;
   params.tmp    = &ctx->state.parser.tmp;
   switch (ctx->state.parser.type) {
   case ria_parser_json:
      params.pexcludes = _json_excludes;
      params.cexcludes = sizeof(_json_excludes)-1;
      break;
   default:
      params.pexcludes = NULL;
      params.cexcludes = 0;
   }   

   /*
    * Make hint dependent pre-actions
    *
    */
   if (rule->chint > 0) {
      switch (rule->phint[0]) {
      /* 
       * Iteration rule
       *
       */
      case '+':
         /* 
          * Reset iterations if required
          *
          */
         if ((*pos > (rule->flags & ria_rule_f_max_pos_mask)) || 
             (*pos < rule->pos))
            rule->flags = 0;
         /* 
          * Start iteration if not started yet
          *
          */
         if ((rule->flags & ria_rule_f_iteration_started) == 0) 
            params.mode = ria_string_extract_begin_only_pos_only;
         else 
         /* 
          * Check for finish
          *
          */
         if (*pos == (rule->flags & ria_rule_f_max_pos_mask)) {
            result->c   = 0;
            rule->flags = 0;
            return true;
         }   
         /* 
          * Try to iterate with hint value
          *
          */
         else {
            params.mode   = ria_string_extract_begin_end_pos_only;
            params.cbegin = 0;
            params.pend   = rule->phint + 1;
            params.cend   = rule->chint - 1;
         }
         break;

      /* 
       * Detection rule
       *
       */
      case '?':
         savepos = *pos; 
      }
   }    
      
   /*
    * Do string extraction, use http.temp for result
    *
    */
   if (!ria_extract_string_from_file(
           &ok,
           &ctx->state.http.temp,
           (char*)buf_get_ptr_bytes(&ctx->state.parser.src),
           buf_get_length(&ctx->state.parser.src),
           &params,
           &ctx->state))
      return false;           

   /*
    * Make hint dependent post-actions
    *
    */
   if (rule->chint > 0) {
      switch (rule->phint[0]) {
      /* 
       * Iteration rule
       *
       */
      case '+':
  
         /* 
          * Finalize iteration setup
          *
          */
         if ((rule->flags & ria_rule_f_iteration_started) == 0) {
            if (!ok) {
               /*
                * Search begin marker was unsuccessful
                *
                */
               result->c = 0;
               return true; 
            }
            rule->pos = *pos;
            /*
             * Try to find end marker 
             *
             */
            params.cbegin = 0; 
            params.mode = ria_string_extract_begin_end_pos_only;
            if (!ria_extract_string_from_file(
                    &ok,
                    &ctx->state.http.temp,
                    (char*)buf_get_ptr_bytes(&ctx->state.parser.src),
                    buf_get_length(&ctx->state.parser.src),
                    &params,
                    &ctx->state))
               return false;           
            if (!ok) {
               /*
                * Search end marker was unsuccessful
                *
                */
               result->c = 0;
               return true; 
            }
            rule->flags &= ~ria_rule_f_max_pos_mask;
            rule->flags |= *pos & ria_rule_f_max_pos_mask;
            rule->flags |= ria_rule_f_iteration_started;
            *pos = rule->pos;
            result->p[0] = '+';
            result->c    = 1;
            return true; 
         }
  
         /* 
          * Is it final iteration? Search by end marker
          *
          */
         if (*pos >= (rule->flags & ria_rule_f_max_pos_mask)) {
            result->c   = 0;
            rule->flags = 0;
         }   
         else {
            rule->pos    = *pos;
            result->p[0] = '+';
            result->c    = 1;
         }            
         return true; 
         
      /* 
       * Detection rule
       *
       */
      case '?':
         if (ok) {
            /*
             * Detection is ok
             *
             */
            result->p[0] = '+';
            result->c    = 1;
         }
         else 
            result->c = 0;
         *pos = savepos;
         return true;     
      }
      
   }    
   
   /*
    * Save result
    *
    */
   if (buf_get_length(&ctx->state.http.temp) > result->c)
      ERR_SET(err_bad_length);
   result->c = buf_get_length(&ctx->state.http.temp);
   MemCpy(result->p, buf_get_ptr_bytes(&ctx->state.http.temp), result->c);   
   return true;     

}


