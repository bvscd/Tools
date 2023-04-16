#include "ria_core.h"
#include "ria_func.h"
#ifdef ANDROID
#include <android/log.h>
#endif


#ifdef USE_RIA_TRACE
//#define COMPILER_TRACE
#endif


/******************************************************************************
 *  Forwards 
 */

bool 
   ria_compile_expression(                                            
      mem_blk_t*            IN OUT   exec,
      mem_blk_t*            IN OUT   script,
      ria_compiler_ctx_t*   IN OUT   ctx);

bool 
   ria_compile_func(                                            
      mem_blk_t*            IN OUT   exec,
      mem_blk_t*            IN OUT   script,
      bool                  IN       ignore_ret,
      ria_compiler_ctx_t*   IN OUT   ctx);

bool 
   ria_compile_chunk(                                            
      mem_blk_t*            IN OUT   exec,
      mem_blk_t*            IN OUT   script,
      ria_compiler_ctx_t*   IN OUT   ctx);

/*
 * Save compilation error pos
 *
 */
#define SET_COMPILE_ERROR(_ctx, _pos, _msg)                                   \
       {                                                                      \
          (_ctx)->ok = false;                                                 \
          (_ctx)->perror = (_pos);                                            \
          (_ctx)->errmsg = (_msg);                                            \
       }

#ifdef USE_RIA_TRACE
#ifdef ANDROID
/* 
 * Android logging support
 *
 */
void 
   do_android_log(
      char*         IN OUT   buf,
      uint*         IN OUT   cval,
      uint          IN       cbuf,
      const char*   IN       str,
      uint          IN       len,
      bool          IN       flush)
{

   assert(buf  != NULL);
   assert(cval != NULL);
   assert((len == 0) || (str != NULL));
   
   for (; len>0; ) {
      bool b = false;
      for (; ((*cval)<cbuf-1)&&(len>0); (*cval)++, len--, str++) {
         if (*str == '\n') {
            str++, len--;
            b = true;
            break;
         }   
         buf[*cval] = *str;  
      }
      if (b || ((*cval) == cbuf-1)) {      
         buf[*cval] = 0x00;
         __android_log_print(ANDROID_LOG_INFO, "libria", buf);                    
         *cval = 0;
      }   
   }
   
   if (flush && (*cval != 0)) {
      buf[*cval] = 0x00;
      __android_log_print(ANDROID_LOG_INFO, "libria", buf);                    
      *cval = 0;
   }
   
}      
#endif      
#endif


/******************************************************************************
 *  Utils
 */

/* 
 * Internal contants
 *
 */
enum {
   ria_mask_var_name_len = 0x00FFFFFF,
   ria_mask_var_type     = 0xFF000000,
   ria_shift_var_type    = 24,
   ria_var_unknown       = (unumber)-1
};

/*****************************************************************************/
bool
   ria_find_char(
      byte**             OUT   out, 
      const mem_blk_t*   IN    buf, 
      char               IN    ch,
      unumber            IN    hint)
/*
 * Finds character in buffer skipping quoted strings and using hints
 *
 */
{

   usize i;
   bool escaped, quoted, transit;

   assert(out != NULL);
   assert(buf != NULL);

   *out = NULL;

   /*
    * Scan buffer
    *
    */
   for (i=0, quoted=escaped=false; i<buf->c; i++) {
      transit = ((buf->p[i] == '"') && (!escaped));
      if (quoted) 
         escaped = ((buf->p[i] == '\\') && (!escaped));
      else {
         switch (ch) {
         case ')':
         case ',':
            if (buf->p[i] == '(')
               hint++;
            else
               if (buf->p[i] == ')')
                  hint--;
            break;
         case '}':
            if (buf->p[i] == '{')
               hint++;
            else
               if (buf->p[i] == '}')
                  hint--;
            break;
         }
         if ((buf->p[i] == ch) && (hint == 0)) {
            *out = buf->p + i;
            break;
         }
      }
      if (transit) 
         quoted = !quoted;
   }

   return true;

}

/*****************************************************************************/
bool
   ria_find_end_of_string(
      byte**             OUT   out, 
      const mem_blk_t*   IN    buf)
/*
 * Finds end of string constant
 *
 */
{

   usize i;
   bool escaped;

   assert(out != NULL);
   assert(buf != NULL);

   *out = NULL;

   /*
    * Scan buffer
    *
    */
   for (i=0, escaped=false; i<buf->c; i++) {
      if ((buf->p[i] == '"') && (!escaped)) {
         *out = buf->p + i;
         break;
      }
      escaped = ((buf->p[i] == '\\') && (!escaped));
   }

   return true;

}

/*****************************************************************************/
bool
   ria_find_var_index(
      usize*                OUT      varidx,
      const buf_t*          IN       names,
      const buf_t*          IN       sizes,
      const byte*           IN       pname,
      usize                 IN       cname,
      ria_compiler_ctx_t*   IN OUT   ctx)
/*
 * Searches for given variable index
 *
 */
{

   usize j, c;
   byte** pp;
   usize* pc;

   assert(varidx != NULL);
   assert(names  != NULL);
   assert(sizes  != NULL);
   assert(pname  != NULL);
   assert(ctx    != NULL);

   *varidx  = (usize)ria_var_unknown;

   /*
    * Try to find variable in pool
    *
    */
   c = buf_get_length(names);
   if (buf_get_length(sizes) != c)
      ERR_SET(err_internal);
   pp = buf_get_ptr_ptrs(names);
   pc = buf_get_ptr_usizes(sizes);
   for (j=0; j<c; j++) {
      if ((pc[j] & ria_mask_var_name_len) != cname)
         continue;
      if (!MemCmp(pp[j], pname, cname))
         break;
   }
   if (j < c) {
      *varidx = j;
      ctx->expr_type = (pc[j] & ria_mask_var_type) >> ria_shift_var_type;
   }

   return true;

}

/*****************************************************************************/
bool
   ria_find_var(
      usize*                OUT      varnlen,
      usize*                OUT      varidx,
      const mem_blk_t*      IN       buf,
      ria_compiler_ctx_t*   IN OUT   ctx)
/*
 * Extracts variable name from stream and searches for its index
 *
 */
{

   usize i;

   assert(varnlen != NULL);
   assert(varidx  != NULL);
   assert(buf     != NULL);

   /*
    * Find non-var character
    *
    */
   for (i=1; i<buf->c; i++) {
      if ((buf->p[i] >= 'a') && (buf->p[i] <= 'z'))
         continue;
      switch (buf->p[i]) {
      case '_':
         continue;
      }   
      if (i > 1)
         if ((buf->p[i] >= '0') && (buf->p[i] <= '9'))
            continue;
      break;
   }

   *varnlen = i;

   /*
    * Try to find variable in global pool
    *
    */
   if (!ria_find_var_index(
           varidx, 
           &ctx->globalp, 
           &ctx->globalc, 
           buf->p, 
           i, 
           ctx))
      return false;
   if (*varidx != ria_var_unknown) {
      *varidx += ria_var_threshold;
      return true;
   }      

   /*
    * Try to find variable in local pool
    *
    */
   return ria_find_var_index(varidx, &ctx->varp, &ctx->varc, buf->p, i, ctx);

}

/*****************************************************************************/
bool
   ria_create_var(
      usize*                OUT      varidx,
      const byte*           IN       pname,
      usize                 IN       cname,
      ria_type_t            IN       type,
      bool                  IN       global,
      ria_compiler_ctx_t*   IN OUT   ctx)
/*
 * Creates new variable 
 *
 */
{

   buf_t* names = (global) ? &ctx->globalp : &ctx->varp;
   buf_t* sizes = (global) ? &ctx->globalc : &ctx->varc;
   usize c;

   assert(varidx != NULL);
   assert(pname  != NULL);
   assert(ctx    != NULL);
   
   c = buf_get_length(names);
   if (c+1 == ria_var_threshold) 
      ERR_SET(err_not_supported);
   if (!buf_expand(c+1, sizes))
      return false;
   buf_get_ptr_usizes(sizes)[c] = cname | (type << ria_shift_var_type);
   if (!buf_set_length(c+1, sizes))
      return false;
   if (!buf_expand(c+1, names))
      return false;
   buf_get_ptr_ptrs(names)[c] = (byte*)pname;
   if (!buf_set_length(c+1, names))
      return false;
      
   *varidx = (global) ? c+ria_var_threshold : c;
   return true;      
      
}

/*****************************************************************************/
bool
   ria_find_string(
      usize*                OUT      strlen,
      usize*                OUT      stridx,
      const mem_blk_t*      IN       buf,
      ria_compiler_ctx_t*   IN OUT   ctx)
/*
 * Extracts string constant from stream and searches for its index
 *
 */
{

   usize j, c;
   mem_blk_t tmp;
   byte* p;
   byte** pp;
   usize* pc;

   assert(strlen != NULL);
   assert(stridx != NULL);
   assert(buf    != NULL);

   /*
    * Find end of string
    *
    */
   tmp.p = buf->p + 1;
   tmp.c = buf->c - 1;
   if (!ria_find_end_of_string(&p, &tmp))
      return false;
   if (p == NULL) {
      SET_COMPILE_ERROR(ctx, buf->p, "unterminated string");
      return true;
   }

   *strlen = p - tmp.p;

   /*
    * Try to find string in pool
    *
    */
   c = buf_get_length(&ctx->strp);
   if (buf_get_length(&ctx->strc) != c)
      ERR_SET(err_internal);
   pp = buf_get_ptr_ptrs(&ctx->strp);
   pc = buf_get_ptr_usizes(&ctx->strc);
   for (j=0; j<c; j++) {
      if (pc[j] != *strlen)
         continue;
      if (!MemCmp(pp[j], tmp.p, *strlen))
         break;
   }

   /*
    * Add if not found
    *
    */
   if (j >= c) {
      if (!buf_expand(c+1, &ctx->strc))
         return false;
      buf_get_ptr_usizes(&ctx->strc)[c] = *strlen;
      if (!buf_set_length(c+1, &ctx->strc))
         return false;
      if (!buf_expand(c+1, &ctx->strp))
         return false;
      buf_get_ptr_ptrs(&ctx->strp)[c] = tmp.p;
      if (!buf_set_length(c+1, &ctx->strp))
         return false;
      j = c;
#ifdef COMPILER_TRACE      
      RIA_TRACE_MSG("Adding new string constant\n");
      RIA_TRACE_STR(tmp.p, *strlen);
      RIA_TRACE_MSG("\n");
#endif      
   }

   *stridx = j;
   ctx->expr_type = ria_string;
   return true;

}

/*****************************************************************************/
bool
   ria_find_param(
      usize*                OUT      parnlen,
      usize*                OUT      paridx,
      const mem_blk_t*      IN       buf,
      ria_compiler_ctx_t*   IN OUT   ctx)
/*
 * Extracts variable name from stream and searches for its index
 *
 */
{

   usize i, j;
   ctx;

   assert(parnlen != NULL);
   assert(paridx  != NULL);
   assert(buf     != NULL);

   /*
    * Find non-digit character
    *
    */
   for (i=1, j=0; i<buf->c; i++) {
      if ((buf->p[i] >= '0') && (buf->p[i] <= '9')) {
         j = j*10 + buf->p[i] - '0';
         continue;
      }
      break;
   }

   *parnlen = i;
   if (j > 0xFF)
      *paridx = (usize)-1;
   else
      *paridx = j;
   ctx->expr_type = ria_string;
   return true;

}

/******************************************************************************
 *  Compiler
 */

/*
 * Lexem types
 *
 */
#define FUNCTYPE(_x) ria_lex_##_x = ria_func_##_x
typedef enum ria_lextype_e {
   ria_lex_return = 0x01,
   ria_lex_if     = 0x02,
   ria_lex_while  = 0x03,
   FUNCTYPE(add_parsing_rule),
   FUNCTYPE(create_parser_for_file),
   FUNCTYPE(dehtml),
   FUNCTYPE(extract_string),
   FUNCTYPE(extract_string_from_file),
   FUNCTYPE(get_binary_to_file),
   FUNCTYPE(get_header),
   FUNCTYPE(get_html),
   FUNCTYPE(get_html_with_dump),
   FUNCTYPE(get_html_to_file),
   FUNCTYPE(get_html_to_file_with_dump),
   FUNCTYPE(int_to_string),
   FUNCTYPE(last_response),
   FUNCTYPE(length),
   FUNCTYPE(load_cookie),
   FUNCTYPE(load_from_file),
   FUNCTYPE(post),
   FUNCTYPE(post_to_file),
   FUNCTYPE(post_to_file_with_dump),
   FUNCTYPE(post_with_dump),
   FUNCTYPE(save_cookie),
   FUNCTYPE(save_to_file),
   FUNCTYPE(set_header),
   FUNCTYPE(string_to_int),
   FUNCTYPE(substring)
} ria_lextype_t;

/*
 * Lexem description
 *
 */
typedef struct ria_lexem_s {
   const char*     p;
   usize           c;
   ria_lextype_t   t;
   unumber         params;
} ria_lexem_t;

/*
 * Function lexem description
 *
 */
typedef struct ria_func_lexem_s {
   ria_lexem_t    info;
   unumber        params;
   ria_type_t     ret;
   function_fn*   impl;
} ria_func_lexem_t;

/*
 * Lexems
 *
 */
#define LEXEM(_s) { #_s"(", sizeof(#_s), ria_lex_##_s }
static const ria_lexem_t 
   _lex[] =
{
   LEXEM(return),
   LEXEM(if),
   LEXEM(while)
};

/*
 * Function lexems
 *
 */
#define FUNC_LEXEM(_s, _par, _ret)                                           \
   { LEXEM(_s), _par, ria_##_ret, ria_implement_##_s }
static const ria_func_lexem_t 
   _func_lex[] =
{
   FUNC_LEXEM(add_parsing_rule, 4, unknown),
   FUNC_LEXEM(create_parser_for_file, 2, unknown),
   FUNC_LEXEM(dehtml, 1, string),
   FUNC_LEXEM(extract_string, 4, string),
   FUNC_LEXEM(extract_string_from_file, 4, string),
   FUNC_LEXEM(get_binary_to_file, 2, int),
   FUNC_LEXEM(get_header, 1, string),
   FUNC_LEXEM(get_html, 1, int),
   FUNC_LEXEM(get_html_with_dump, 2, int),
   FUNC_LEXEM(get_html_to_file, 2, int),
   FUNC_LEXEM(get_html_to_file_with_dump, 3, int),
   FUNC_LEXEM(int_to_string, 1, string),
   FUNC_LEXEM(last_response, 0, string),
   FUNC_LEXEM(length, 1, int),
   FUNC_LEXEM(load_cookie, 3, string),
   FUNC_LEXEM(load_from_file, 1, string),
   FUNC_LEXEM(post, 2, int),
   FUNC_LEXEM(post_to_file, 3, int),
   FUNC_LEXEM(post_to_file_with_dump, 4, int),
   FUNC_LEXEM(post_with_dump, 3, int),
   FUNC_LEXEM(save_cookie, 3, unknown),
   FUNC_LEXEM(save_to_file, 2, unknown),
   FUNC_LEXEM(set_header, 2, unknown),
   FUNC_LEXEM(string_to_int, 1, int),
   FUNC_LEXEM(substring, 3, string)
};

/*****************************************************************************/
bool 
   ria_match_op_and_type(
      ria_type_t*   IN OUT   type,
      bool          IN       unary,
      byte          IN       op1,
      byte          IN       op2)
/*
 * Validates operation against data type
 *
 */
{

   assert(type != NULL);

   if (*type == ria_unknown)
      return false;

   switch (op2) {
   case 0x00:
      switch (op1) {
      case '+':
         if (*type == ria_boolean) 
            return false;
         break;
      case '<':
      case '>':
         if (*type == ria_boolean) 
            return false;
         *type = ria_boolean;
         break;
      case '_':
      case '~':
         if (!unary) 
            return false;
      case '-':
      case '*':
      case '/':
      case '%':
      case '&':
      case '|':
      case '^':
         if (*type != ria_int) 
            return false;
         break;
      case '!':
         if (*type != ria_boolean) 
            return false;
         break;
      default:
        return false;
      }
      break;
   case '=':
      switch (op1) {
      case '<':
      case '>':
         if (*type == ria_boolean) 
            return false;
         *type = ria_boolean;
         break;
      case '=':
      case '!':
         *type = ria_boolean;
         break;
      default:
         return false;
      }
      break;
   case '|':
      switch (op1) {
      case '|':
         if (*type == ria_boolean) 
            break;
      default:
         return false;
      }
      break;
   case '&':
      switch (op1) {
      case '&':
         if (*type == ria_boolean) 
            break;
      default:
         return false;
      }
      break;
   default:
      return false;
   }

   return true;

}

/*****************************************************************************/
bool 
   ria_get_function_info(                                            
      ria_type_t*     OUT   type,
      unumber*        OUT   params,
      function_fn**   OUT   impl,
      unumber         IN    func)
/*
 * Returns information about predefind function 
 *
 */
{

   usize i;

   assert(params != NULL);
   assert(impl   != NULL);
   assert(type   != NULL);

   for (i=0; i<sizeof(_func_lex)/sizeof(_func_lex[0]); i++) 
      if ((unsigned)_func_lex[i].info.t == func) {
         if (_func_lex[i].impl == NULL)
            ERR_SET(err_internal);
         *params = _func_lex[i].params;
         *impl   = _func_lex[i].impl;
         *type   = _func_lex[i].ret;
         return true;
      }
   
   ERR_SET(err_internal);

}

/*****************************************************************************/
bool 
   ria_encode_push_var(
      byte**   IN OUT   pp, 
      usize*   IN OUT   pc,
      usize    IN       idx)
/*
 * Encodes push-var command 
 *
 */
{

   assert(pp != NULL);
   assert(pc != NULL);

   if (*pc < 2)
      ERR_SET(err_bad_length);
   if (idx > 0xFF)
      ERR_SET(err_internal);
   (*pp)[0] = ria_opcode_pushv;
   (*pp)[1] = (byte)idx;

   (*pp) += 2;
   (*pc) -= 2;
   return true;

}

/*****************************************************************************/
bool 
   ria_encode_push_string(
      byte**   IN OUT   pp, 
      usize*   IN OUT   pc,
      usize    IN       idx)
/*
 * Encodes push-string command 
 *
 */
{

   assert(pp != NULL);
   assert(pc != NULL);

   if (idx > 0xFFFF)
      ERR_SET(err_internal);
   if (*pc < (unsigned)((idx > 0xFF) ? 3 : 2))
      ERR_SET(err_bad_length);
   if (idx <= 0xFF) {
      (*pp)[0] = ria_opcode_pushs;
      (*pp)[1] = (byte)idx;
      (*pp) += 2;
      (*pc) -= 2;
   }
   else {
      (*pp)[0] = ria_opcode_pushs2;
      (*pp)[1] = (byte)(idx >> 8);
      (*pp)[2] = (byte)(idx >> 0);
      (*pp) += 3;
      (*pc) -= 3;
   }

   return true;

}

/*****************************************************************************/
bool 
   ria_encode_push_param(
      byte**   IN OUT   pp, 
      usize*   IN OUT   pc,
      usize    IN       idx)
/*
 * Encodes push-param command 
 *
 */
{

   assert(pp != NULL);
   assert(pc != NULL);

   if (*pc < 2)
      ERR_SET(err_bad_length);
   if (idx > 0xFF)
      ERR_SET(err_internal);
   (*pp)[0] = ria_opcode_pushp;
   (*pp)[1] = (byte)idx;

   (*pp) += 2;
   (*pc) -= 2;
   return true;

}

/*****************************************************************************/
bool 
   ria_encode_push_int(
      byte**   IN OUT   pp, 
      usize*   IN OUT   pc,
      int      IN       i)
/*
 * Encodes push-param command 
 *
 */
{

   byte opcode;
   unumber c, j;

   assert(pp != NULL);
   assert(pc != NULL);
   
   if ((unumber)i < 0xFF) {
      c = 1;
      opcode = ria_opcode_pushi1;
   }
   else      
   if ((unumber)i < 0xFFFF) {
      c = 2;
      opcode = ria_opcode_pushi2;
   }
   else
   if ((unumber)i < 0xFFFFFF) {
      c = 3;
      opcode = ria_opcode_pushi3;
   }
   else {
      c = 4;
      opcode = ria_opcode_pushi4;
   }

   if (*pc < c+1)
      ERR_SET(err_bad_length);
   (*pp)[0] = opcode;
   for (j=c; j>0; j--, i>>=8) 
      (*pp)[j] = (byte)i;
   (*pc) -= c + 1;      
   (*pp) += c + 1;      
   return true;

}

/*****************************************************************************/
bool 
   ria_encode_pop(
      byte**   IN OUT   pp, 
      usize*   IN OUT   pc,
      usize    IN       idx)
/*
 * Encodes pop command 
 *
 */
{

   assert(pp != NULL);
   assert(pc != NULL);

   if (*pc < 2)
      ERR_SET(err_bad_length);
   if (idx > 0xFF)
      ERR_SET(err_internal);
   (*pp)[0] = ria_opcode_pop;
   (*pp)[1] = (byte)idx;

   (*pp) += 2;
   (*pc) -= 2;
   return true;

}

/*****************************************************************************/
bool 
   ria_encode_ret(
      byte**   IN OUT   pp, 
      usize*   IN OUT   pc)
/*
 * Encodes ret command 
 *
 */
{

   assert(pp != NULL);
   assert(pc != NULL);

   if (*pc < 1)
      ERR_SET(err_bad_length);
   (*pp)[0] = ria_opcode_ret;

   (*pp)++;
   (*pc)--;
   return true;

}

/*****************************************************************************/
bool 
   ria_encode_op(
      byte**       IN OUT   pp, 
      usize*       IN OUT   pc,
      byte         IN       op1,
      byte         IN       op2)
/*
 * Encodes operation command 
 *
 */
{

   byte opcode;

   assert(pp != NULL);
   assert(pc != NULL);

   if (*pc < 1)
      ERR_SET(err_bad_length);

   switch (op1) {
   case '+':
      opcode = ria_opcode_add;
      break;
   case '|':
      if (op2 == op1)
         opcode = ria_opcode_lor;
      else
         opcode = ria_opcode_bor;
      break;
   case '<':
      if (op2 == '=')
         opcode = ria_opcode_less_eq;
      else
         opcode = ria_opcode_less;
      break;
   case '>':
      if (op2 == '=')
         opcode = ria_opcode_more_eq;
      else
         opcode = ria_opcode_more;
      break;
   case '=':
      if (op2 == '=')
         opcode = ria_opcode_eq;
      else
         ERR_SET(err_internal);
      break;
   case '!':
      if (op2 == '=')
         opcode = ria_opcode_not_eq;
      else
         opcode = ria_opcode_not;
      break;
   case '-':
      opcode = ria_opcode_sub;
      break;
   case '*':
      opcode = ria_opcode_mul;
      break;
   case '&':
      if (op2 == op1)
         opcode = ria_opcode_land;
      else
         opcode = ria_opcode_band;
      break;
   case '/':
      opcode = ria_opcode_div;
      break;
   case '%':
      opcode = ria_opcode_rem;
      break;
   case '^':
      opcode = ria_opcode_xor;
      break;
   case '~':
      opcode = ria_opcode_bneg;
      break;
   case '_':
      opcode = ria_opcode_neg;
      break;
   default:
      ERR_SET(err_internal);
   }

   **pp = opcode;
   (*pp)++;
   (*pc)--;
   return true;

}

/*****************************************************************************/
bool 
   ria_encode_call(
      byte**          IN OUT   pp, 
      usize*          IN OUT   pc,
      bool            IN       ignore_ret,
      ria_lextype_t   IN       lex)
/*
 * Encodes function call
 *
 */
{

   byte b0 = 0x00;
   byte b1 = 0x00;

   assert(pp != NULL);
   assert(pc != NULL);

   /*
    * Check for lex
    *
    */
   switch (lex) {
   case ria_lex_add_parsing_rule:
   case ria_lex_create_parser_for_file:
   case ria_lex_dehtml:
   case ria_lex_extract_string:
   case ria_lex_extract_string_from_file:
   case ria_lex_get_binary_to_file:
   case ria_lex_get_header:
   case ria_lex_get_html:
   case ria_lex_get_html_with_dump:
   case ria_lex_get_html_to_file:
   case ria_lex_get_html_to_file_with_dump:
   case ria_lex_int_to_string:
   case ria_lex_last_response:
   case ria_lex_length:
   case ria_lex_load_cookie:
   case ria_lex_load_from_file:
   case ria_lex_post:
   case ria_lex_post_to_file:
   case ria_lex_post_to_file_with_dump:
   case ria_lex_post_with_dump:
   case ria_lex_save_cookie:
   case ria_lex_save_to_file:
   case ria_lex_set_header:
   case ria_lex_string_to_int:
   case ria_lex_substring:
      b0 = lex;
      break;
   default:
      ERR_SET(err_internal);
   }

   if (*pc < (unsigned)((b1==0x00) ? 2 : 3))
      ERR_SET(err_bad_length);
   if (b1 != 0x00)
      (*pp)[0] = (ignore_ret) ? ria_opcode_call2i : ria_opcode_call2p;
   else
      (*pp)[0] = (ignore_ret) ? ria_opcode_calli : ria_opcode_callp;
   (*pp)[1] = b0;
   (*pp) += 2;
   (*pc) -= 2;

   if (b1 != 0x00) {
      (*pp)[0] = b1;
      (*pp)++;
      (*pc)--;
   }
   return true;

}

/*****************************************************************************/
bool 
   ria_encode_jump(
      byte**   IN OUT   pp, 
      usize*   IN OUT   pc,
      usize    IN       offset)
/*
 * Encodes unconditional jump 
 *
 */
{

   unumber c;

   assert(pp != NULL);
   assert(pc != NULL);

   c = (offset > 0x7F) ? 3 : 2;
   if (*pc < c)
      ERR_SET(err_bad_length);
   if (c == 2) {
      (*pp)[0] = ria_opcode_jmp;
      (*pp)[1] = (byte)offset;
   }
   else {
      (*pp)[0] = ria_opcode_jmp2;
      (*pp)[1] = (byte)(offset >> 8);
      (*pp)[2] = (byte)(offset >> 0);
   }

   (*pp) += c;
   (*pc) -= c;
   return true;

}

/*****************************************************************************/
bool 
   ria_encode_jump_if(
      byte**   IN OUT   pp, 
      usize*   IN OUT   pc,
      bool     IN       what,
      usize    IN       offset)
/*
 * Encodes conditional jump 
 *
 */
{

   unumber c;

   assert(pp != NULL);
   assert(pc != NULL);

   c = (offset > 0x7F) ? 3 : 2;
   if (*pc < c)
      ERR_SET(err_bad_length);
   if (c == 2) {
      (*pp)[0] = (what) ? ria_opcode_jit : ria_opcode_jif;
      (*pp)[1] = (byte)offset;
   }
   else {
      (*pp)[0] = (what) ? ria_opcode_jit2 : ria_opcode_jif2;
      (*pp)[1] = (byte)(offset >> 8);
      (*pp)[2] = (byte)(offset >> 0);
   }

   (*pp) += c;
   (*pc) -= c;
   return true;

}

/*****************************************************************************/
bool 
   ria_compile_operand(                                            
      mem_blk_t*            IN OUT   exec,
      mem_blk_t*            IN OUT   script,
      ria_compiler_ctx_t*   IN OUT   ctx)
/*
 * Compiles operand
 *
 */
{

   usize i, j, c;
   byte* p;
   mem_blk_t expr;

   assert(ctx    != NULL);
   assert(script != NULL);
   assert(exec   != NULL);

   /*
    * Operand is:
    * - '(' <expression> ')'
    * - '$' <var>
    * - '@' <parameter>
    * - '"' <string> '"'
    * - <decimal digit>{1,}
    *
    */
   
   /*
    * Check operand type
    *
    */
   if (script->c < 1) {
      SET_COMPILE_ERROR(ctx, script->p, "syntax error");
      return true;
   }
   switch (script->p[0]) {

   /* Nested expression */
   case '(':
      expr.p = script->p + 1;
      expr.c = script->c - 1;
      if (!ria_find_char(&p, &expr, ')', 1))
         return false;
      if (p == NULL) {
         SET_COMPILE_ERROR(ctx, script->p, "unterminated expression");
         return true;
      }
      expr.c = p - expr.p;
#ifdef COMPILER_TRACE      
      RIA_TRACE_MSG("Compiling operand, nested expression ");
      RIA_TRACE_STR(script->p, expr.c+2);
      RIA_TRACE_MSG("\n");
#endif      
      if (!ria_compile_expression(exec, &expr, ctx))
         return false;
      if (!ctx->ok)
         return true;
      script->c -= expr.p - script->p + 1;
      script->p  = expr.p + 1;
      break;

   /* Variable */
   case '$':
      if (!ria_find_var(&i, &j, script, ctx))
         return false;
      if (j == ria_var_unknown) {
         SET_COMPILE_ERROR(ctx, script->p, "unknown variable");
         return true;
      }
      p = exec->p;
      c = exec->c;
      if (!ria_encode_push_var(&p, &c, j))
         return false;
#ifdef COMPILER_TRACE      
      RIA_TRACE_MSG("COMPILED pushv ");
      RIA_TRACE_INT(j);
      RIA_TRACE_MSG("\n");
#endif      
      exec->c = p - exec->p;
      script->c -= i;
      script->p += i;
      break;

   /* Parameter */
   case '@':
      if (!ria_find_param(&i, &j, script, ctx))
         return false;
      p = exec->p;
      c = exec->c;
      if (!ria_encode_push_param(&p, &c, j))
         return false;
#ifdef COMPILER_TRACE      
      RIA_TRACE_MSG("COMPILED pushp ");
      RIA_TRACE_INT(j);
      RIA_TRACE_MSG("\n");
#endif      
      exec->c = p - exec->p;
      script->c -= i;
      script->p += i;
      break;

   /* String constant */
   case '"':
      if (!ria_find_string(&i, &j, script, ctx))
         return false;
      p = exec->p;
      c = exec->c;
      if (!ria_encode_push_string(&p, &c, j))
         return false;
#ifdef COMPILER_TRACE      
      RIA_TRACE_MSG("COMPILED pushs ");
      RIA_TRACE_INT(j);
      RIA_TRACE_MSG("\n");
#endif      
      exec->c = p - exec->p;
      script->c -= i + 2;
      script->p += i + 2;
      break;

   default:
      if ((script->p[0] >= '0') && (script->p[0] <= '9')) {
         /*
          * Numeric constant case
          *
          */
         bool nonzero = false; 
         for (j=0; script->c>0; script->p++, script->c--, nonzero=true) {
            if ((script->p[0] < '0') || (script->p[0] > '9')) 
               break;
            if (nonzero && (j == 0)) {
               SET_COMPILE_ERROR(ctx, script->p, "bad numeric constant");
               return true;
            }   
            j = j*10 + script->p[0] - '0';
         }               
         p = exec->p;
         c = exec->c;
         if (!ria_encode_push_int(&p, &c, (int)j))
            return false;
#ifdef COMPILER_TRACE      
         RIA_TRACE_MSG("COMPILED pushi ");
         RIA_TRACE_INT(j);
         RIA_TRACE_MSG("\n");
#endif         
         exec->c = p - exec->p;
         ctx->expr_type = ria_int;
      }
      else {
         /*
          * Predefined function call 
          *
          */
         if (!ria_compile_func(exec, script, false, ctx))
            return false;
      }

   }

   return true;

}

/*****************************************************************************/
bool 
   ria_compile_expression(                                            
      mem_blk_t*            IN OUT   exec,
      mem_blk_t*            IN OUT   script,
      ria_compiler_ctx_t*   IN OUT   ctx)
/*
 * Compiles expression
 *
 */
{

   char op[2];
   bool unary;
   ria_type_t type;
   mem_blk_t oper;
   byte* p;
   byte* pe;
   usize ce;

   assert(ctx    != NULL);
   assert(script != NULL);
   assert(exec   != NULL);

#ifdef COMPILER_TRACE      
   RIA_TRACE_MSG("Compiling expression ");
   RIA_TRACE_STR(script->p, script->c);
   RIA_TRACE_MSG("\n");
#endif   

   /*
    * Evaluate complex expression:
    * (<op>){0,1} <operand> (<op> <operand>){0,} 
    * where operand can be nested expression
    *
    */

   /* 
    * Check for unary operation:
    * (<op>){0,1}
    * and cache it
    *
    */
   if (script->c < 1) {
      SET_COMPILE_ERROR(ctx, script->p, "syntax error");
      return true;
   }
   type = ria_unknown;
   switch (script->p[0]) {
   case '~':
   case '!':
   case '-':
      /* Handle unary '-' as '_' */
      op[0] = (script->p[0] == '-') ? '_' : script->p[0];
      op[1] = 0x00;
      script->p++;
      script->c--;
      unary = true;
      break;
   default:
      op[0] = 0x00;
      unary = false;
   }

   /* 
    * Evaluate sequence of 
    * <operand> (<op> <operand>){0,} 
    *
    */
   pe = exec->p;
   ce = exec->c;
   while (script->c > 0) {

      /*
       * Compile operand
       *
       */
      oper.p = pe;
      oper.c = ce;
      p = script->p;
      if (!ria_compile_operand(&oper, script, ctx))
         return false;
      if (!ctx->ok)
         return true;
      if (oper.p != pe) 
         ERR_SET(err_internal);
      ce -= oper.c;
      pe += oper.c;
      if (type != ria_unknown) {
         if (type != ctx->expr_type) {
            SET_COMPILE_ERROR(ctx, p, "bad operands type");
            return true;
         }
      }
      else {
         if (ctx->expr_type == ria_unknown) {
            SET_COMPILE_ERROR(ctx, p, "use of uninitialized variable");
            return true;
         }
         type = ctx->expr_type;
      }

      /*
       * Apply cached operation
       *
       */
      if (op[0] != 0x00) {
         if (!ria_match_op_and_type(&type, unary, op[0], op[1])) {
            SET_COMPILE_ERROR(ctx, p, "bad operands type");
            return true;
         }
         ctx->expr_type = type;
         if (!ria_encode_op(&pe, &ce, op[0], op[1])) 
            return true;
#ifdef COMPILER_TRACE      
         RIA_TRACE_MSG("COMPILED op ");
         RIA_TRACE_STR(op, (op[1]==0x00)?1:2);
         RIA_TRACE_MSG("\n");
#endif         
         op[0] = 0x00;
         unary = false;
      }

      /*
       * Extract next operation
       *
       */
      if (script->c > 0) {
         if (script->c < 2) {
            SET_COMPILE_ERROR(ctx, p, "syntax error");
            return true;
         }
         op[0] = script->p[0];
         switch (script->p[0]) {
         case '+':
         case '-':
         case '*':
         case '/':
         case '%':
         case '^':
         case '~':
            op[1] = 0x00;
            break;
         case '<':
         case '>':
         case '!':
            if (script->p[1] == '=')
               op[1] = '=';
            else 
               op[1] = 0x00;
            break;
         case '=':
            if (script->p[1] != '=') {
               SET_COMPILE_ERROR(ctx, p, "syntax error");
               return true;
            }
            op[1] = '=';
            break;
         case '&':
         case '|':
            if (script->p[1] == op[0])
               op[1] = op[0];
            else 
               op[1] = 0x00;
            break;
         default:
            SET_COMPILE_ERROR(ctx, p, "syntax error");
            return true;
         }
         script->p++;
         script->c--;
         if (op[1] != 0x00) {
            script->p++;
            script->c--;
         }
      }

   }

   /*
    * Finalize output buffer
    *
    */
   exec->c = pe - exec->p;
#ifdef COMPILER_TRACE      
   RIA_TRACE_MSG("Expression compilation finished\n");
#endif   
   return true;

}

/*****************************************************************************/
bool 
   ria_compile_assignment(                                            
      mem_blk_t*            IN OUT   exec,
      mem_blk_t*            IN OUT   script,
      ria_compiler_ctx_t*   IN OUT   ctx)
/*
 * Compiles assignment statement
 *
 */
{

   usize c, i, j;
   byte* p;
   mem_blk_t expr;

   assert(ctx    != NULL);
   assert(script != NULL);
   assert(exec   != NULL);

   /*
    * Assignment is:
    * $var=expression
    *
    */

   assert(script->c > 0);
   assert(script->p[0] == '$');

#ifdef COMPILER_TRACE      
   RIA_TRACE_MSG("Compiling assignment ");
#endif

   /*
    * Extract variable info
    *
    */
   if (!ria_find_var(&i, &j, script, ctx))
      return false;
   if ((i >= script->c) || (script->p[i] != '=')) {
      SET_COMPILE_ERROR(ctx, script->p, "syntax error");
      return true;
   }

#ifdef COMPILER_TRACE      
   RIA_TRACE_MSG("to variable ");
   RIA_TRACE_STR(script->p, i);
   RIA_TRACE_MSG("\n");
#endif

   /*
    * Create new if not found 
    *
    */
   if (j == ria_var_unknown) {
      if (!ria_create_var(&j, script->p, i, ria_unknown, false, ctx))
         return false;
#ifdef COMPILER_TRACE      
      RIA_TRACE_MSG("No such variable was found, adding new one\n");
#endif      
   }

   /*
    * Extract expression script
    *
    */
   script->p += i + 1;
   script->c -= i + 1;
   expr = *script;
   if (!ria_find_char(&p, &expr, ';', 0))
      return false;
   if (p == NULL) {
      SET_COMPILE_ERROR(ctx, script->p, "; is absent");
      return true;
   }
   expr.c = p - expr.p;

   /*
    * Compile expression
    *
    */
   p = exec->p;
   c = exec->c;
   if (!ria_compile_expression(exec, &expr, ctx))
      return false;
   if (!ctx->ok)
      return true;
   if (ctx->expr_type == ria_unknown)
      ERR_SET(err_internal);
   script->c -= expr.p - script->p + 1;
   script->p  = expr.p + 1;

   /*
    * Setup variable type
    *
    */
   if (j >= ria_var_threshold) {
      usize* pu = buf_get_ptr_usizes(&ctx->globalc) + j - ria_var_threshold;
      *pu = (*pu & ria_mask_var_name_len) | 
         ((usize)ctx->expr_type << ria_shift_var_type);
   }         
   else {  
      usize* pu = buf_get_ptr_usizes(&ctx->varc) + j;
      *pu = (*pu & ria_mask_var_name_len) |
         ((usize)ctx->expr_type << ria_shift_var_type);
   }         

   /*
    * Add pop-to-var command
    *
    */
   if (p != exec->p)
      ERR_SET(err_internal);
   c -= exec->c;
   p += exec->c;
   if (!ria_encode_pop(&p, &c, j))
      return false;
   exec->c = p - exec->p;
#ifdef COMPILER_TRACE      
   RIA_TRACE_MSG("COMPILED: pop ");
   RIA_TRACE_INT(j);
   RIA_TRACE_MSG("\n");
   RIA_TRACE_MSG("Assignment compilation finished\n");
#endif
   return true;

}

/*****************************************************************************/
bool 
   ria_compile_return(                                            
      mem_blk_t*            IN OUT   exec,
      mem_blk_t*            IN OUT   script,
      ria_compiler_ctx_t*   IN OUT   ctx)
/*
 * Compiles return statement
 *
 */
{

   usize c;
   byte* p;
   mem_blk_t expr;

   assert(ctx    != NULL);
   assert(script != NULL);
   assert(exec   != NULL);

   /*
    * Return statement is
    * return(<expression>);
    * Script to parse is:
    * (expression);
    *
    */

#ifdef COMPILER_TRACE      
   RIA_TRACE_MSG("Compiling return statement\n");
#endif

   /*
    * Extract expression script
    *
    */
   expr = *script;
   if (!ria_find_char(&p, &expr, ';', 0))
      return false;
   if (p == NULL) {
      SET_COMPILE_ERROR(ctx, script->p, "; is absent");
      return true;
   }
   expr.c = p - expr.p;

   /*
    * Compile expression
    *
    */
   p = exec->p;
   c = exec->c;
   if (!ria_compile_expression(exec, &expr, ctx))
      return false;
   if (!ctx->ok)
      return true;
   if (ctx->expr_type == ria_unknown)
      ERR_SET(err_internal);
   script->c -= expr.p - script->p + 1;
   script->p  = expr.p + 1;

   /*
    * Add ret command
    *
    */
   if (p != exec->p)
      ERR_SET(err_internal);
   c -= exec->c;
   p += exec->c;
   if (!ria_encode_ret(&p, &c))
      return false;
   exec->c = p - exec->p;
#ifdef COMPILER_TRACE      
   RIA_TRACE_MSG("COMPILED: ret\n");
   RIA_TRACE_MSG("Return statement compilation finished\n");
#endif
   return true;

}

/*****************************************************************************/
bool 
   ria_compile_if(                                            
      mem_blk_t*            IN OUT   exec,
      mem_blk_t*            IN OUT   script,
      ria_compiler_ctx_t*   IN OUT   ctx)
/*
 * Compiles if statement
 *
 */
{

   static const char _else[] = "else{";

   usize c, k, l;
   byte* p;
   mem_blk_t expr;
   mem_blk_t xtmp;
   byte* pjmp1 = NULL;
   byte* pjmp2 = NULL;

   assert(ctx    != NULL);
   assert(script != NULL);
   assert(exec   != NULL);

   /*
    * If statement is
    * if (<expression>) { <chunk1>...<chunkn> } else { <chunk1>...<chunkm> }
    * or 
    * if (<expression>) { <chunk1>...<chunkn> }
    * Script to parse is:
    * (<expression>) { <chunk1>...<chunkn> } else { <chunk1>...<chunkm> }
    *
    */

#ifdef COMPILER_TRACE      
   RIA_TRACE_MSG("Compiling if statement\n");
#endif

   /*
    * Find end of condition 
    *
    */
   expr = *script;
   if (!ria_find_char(&p, &expr, ')', 0))
      return false;
   if (p == NULL) {
      SET_COMPILE_ERROR(ctx, script->p, "unterminated expression");
      return true;
   }
   expr.c = p - expr.p + 1;
 
   /*
    * Compile expression for condition
    *
    */
   xtmp = *exec;
   p = xtmp.p;
   c = xtmp.c;
   if (!ria_compile_expression(&xtmp, &expr, ctx))
      return false;
   if (!ctx->ok)
      return true;
   if (ctx->expr_type != ria_boolean)
      ERR_SET(err_internal);
   script->c -= expr.p - script->p;
   script->p  = expr.p;

   /*
    * Save pos for putting 1st jump 
    *
    */
   if (p != xtmp.p)
      ERR_SET(err_internal);
   xtmp.p += xtmp.c;
   xtmp.c  = c - xtmp.c;
   pjmp1   = xtmp.p;

   /*
    * Check for main branch
    *
    */
#ifdef COMPILER_TRACE      
   RIA_TRACE_MSG("Compiling main branch\n");
#endif
   if ((script->c < 1) || (script->p[0] != '{')) {
      SET_COMPILE_ERROR(ctx, script->p, "syntax error");
      return true;
   }
   script->p++;
   script->c--;

   /*
    * Duplicated actions
    *
    */
   for (k=0; k<2; k++) {

      /*
       * Extract branch
       *
       */
      expr = *script;
      if (!ria_find_char(&p, &expr, '}', 1))
         return false;
      if (p == NULL) {
         SET_COMPILE_ERROR(ctx, script->p, "syntax error");
         return true;
      }
      expr.c = p - expr.p;

      /*  
       * Compile branch
       *
       */
      for (; expr.c>0; ) {
         p = xtmp.p;
         c = xtmp.c;
         if (!ria_compile_chunk(&xtmp, &expr, ctx))
            return false;
         if (!ctx->ok)
            return true;
         if (p != xtmp.p)
            ERR_SET(err_internal);
         xtmp.p += xtmp.c;
         xtmp.c  = c - xtmp.c;
      }
      script->c -= expr.p - script->p + 1;
      script->p  = expr.p + 1;
   
      if (k == 1)
         break;

      /*
       * Check alternate branch
       *
       */
      if (script->c < sizeof(_else)-1)
         break;
      if (MemCmp(script->p, _else, sizeof(_else)-1))
         break;
#ifdef COMPILER_TRACE      
      RIA_TRACE_MSG("Compiling alternate branch\n");
#endif
      script->c -= sizeof(_else) - 1;
      script->p += sizeof(_else) - 1;

      /*
       * Save pos for putting 2nd jump 
       *
       */
      pjmp2 = xtmp.p;

   }

   /*
    * Add 2nd jump first
    *
    */
   if (pjmp2 != NULL) {
      l = xtmp.p - pjmp2;
      c = (l > 0x7F) ? 3 : 2;
      if (xtmp.c < c) 
         ERR_SET(err_bad_length);
      MemMove(pjmp2+c, pjmp2, xtmp.p-pjmp2);
      if (!ria_encode_jump(&pjmp2, &c, l))
         return false;
#ifdef COMPILER_TRACE      
      RIA_TRACE_MSG("COMPILED: jmp ");
      RIA_TRACE_INT(l);
      RIA_TRACE_MSG("\n");
#endif
      xtmp.c -= (l > 0x7F) ? 3 : 2;
      xtmp.p += (l > 0x7F) ? 3 : 2;
   }

   /*
    * Add 1st jump then
    *
    */
   l = (pjmp2 == NULL) ? xtmp.p-pjmp1 : pjmp2-pjmp1;
   c = (l > 0x7F) ? 3 : 2;
   if (xtmp.c < c) 
      ERR_SET(err_bad_length);
   MemMove(pjmp1+c, pjmp1, xtmp.p-pjmp1);
   if (!ria_encode_jump_if(&pjmp1, &c, false, l))
      return false;
#ifdef COMPILER_TRACE      
   RIA_TRACE_MSG("COMPILED: jif ");
   RIA_TRACE_INT(l);
   RIA_TRACE_MSG("\n");
#endif
   xtmp.c -= (l > 0x7F) ? 3 : 2;
   xtmp.p += (l > 0x7F) ? 3 : 2;

   /*
    * Finalize
    *
    */
   exec->c = xtmp.p - exec->p;
   return true;

}

/*****************************************************************************/
bool 
   ria_compile_while(                                            
      mem_blk_t*            IN OUT   exec,
      mem_blk_t*            IN OUT   script,
      ria_compiler_ctx_t*   IN OUT   ctx)
/*
 * Compiles while statement
 *
 */
{

   usize c, l;
   byte* p;
   mem_blk_t expr;
   mem_blk_t xtmp;
   byte* pexpr;
   byte* pjump;

   assert(ctx    != NULL);
   assert(script != NULL);
   assert(exec   != NULL);

   /*
    * While statement is
    * while (<expression>) { <chunk1>...<chunkn> }
    * Script to parse is:
    * (<expression>) { <chunk1>...<chunkn> } 
    *
    */

#ifdef COMPILER_TRACE      
   RIA_TRACE_MSG("Compiling while statement\n");
#endif

   /*
    * Find end of condition 
    *
    */
   expr = *script;
   if (!ria_find_char(&p, &expr, ')', 0))
      return false;
   if (p == NULL) {
      SET_COMPILE_ERROR(ctx, script->p, "unterminated expression");
      return true;
   }
   expr.c = p - expr.p + 1;
 
   /*
    * Save pos to expression
    *
    */
   pexpr = exec->p; 
   
   /*
    * Compile expression for condition
    *
    */
   xtmp = *exec;
   p = xtmp.p;
   c = xtmp.c;
   if (!ria_compile_expression(&xtmp, &expr, ctx))
      return false;
   if (!ctx->ok)
      return true;
   if (ctx->expr_type != ria_boolean)
      ERR_SET(err_internal);
   script->c -= expr.p - script->p;
   script->p  = expr.p;

   /*
    * Save pos for putting first jump 
    *
    */
   if (p != xtmp.p)
      ERR_SET(err_internal);
   xtmp.p += xtmp.c;
   xtmp.c  = c - xtmp.c;
   pjump   = xtmp.p;

   /*
    * Check for loop body
    *
    */
#ifdef COMPILER_TRACE      
   RIA_TRACE_MSG("Compiling loop body\n");
#endif
   if ((script->c < 1) || (script->p[0] != '{')) {
      SET_COMPILE_ERROR(ctx, script->p, "syntax error");
      return true;
   }
   script->p++;
   script->c--;

   /*
    * Extract body
    *
    */
   expr = *script;
   if (!ria_find_char(&p, &expr, '}', 1))
      return false;
   if (p == NULL) {
      SET_COMPILE_ERROR(ctx, script->p, "syntax error");
      return true;
   }
   expr.c = p - expr.p;

   /*  
    * Compile body
    *
    */
   for (; expr.c>0; ) {
      p = xtmp.p;
      c = xtmp.c;
      if (!ria_compile_chunk(&xtmp, &expr, ctx))
         return false;
      if (!ctx->ok)
         return true;
      if (p != xtmp.p)
         ERR_SET(err_internal);
      xtmp.p += xtmp.c;
      xtmp.c  = c - xtmp.c;
   }
   script->c -= expr.p - script->p + 1;
   script->p  = expr.p + 1;
   
   /*
    * Add 1st jump
    *
    */
   l = xtmp.p - pjump + 3;
   c = (l > 0x7F) ? 3 : 2;
   if (xtmp.c < c) 
      ERR_SET(err_bad_length);
   MemMove(pjump+c, pjump, xtmp.p-pjump);
   if (!ria_encode_jump_if(&pjump, &c, false, l))
      return false;
#ifdef COMPILER_TRACE      
   RIA_TRACE_MSG("COMPILED: jif ");
   RIA_TRACE_INT(l);
   RIA_TRACE_MSG("\n");
#endif
   xtmp.c -= (l > 0x7F) ? 3 : 2;
   xtmp.p += (l > 0x7F) ? 3 : 2;

   /*
    * Add 2nd jump
    *
    */
   l = pexpr - xtmp.p;
   c = 3;
   if (xtmp.c < c) 
      ERR_SET(err_bad_length);
   p = xtmp.p;
   if (!ria_encode_jump(&p, &c, l))
      return false;
#ifdef COMPILER_TRACE      
   RIA_TRACE_MSG("COMPILED: jmp ");
   RIA_TRACE_INT(l);
   RIA_TRACE_MSG("\n");
#endif
   xtmp.c -= 3;
   xtmp.p += 3;

   /*
    * Finalize
    *
    */
   exec->c = xtmp.p - exec->p;
   return true;

}

/*****************************************************************************/
bool 
   ria_compile_func(                                            
      mem_blk_t*            IN OUT   exec,
      mem_blk_t*            IN OUT   script,
      bool                  IN       ignore_ret,
      ria_compiler_ctx_t*   IN OUT   ctx)
/*
 * Compiles predefined function call
 *
 */
{

   usize c, i, j;
   byte* p;
   mem_blk_t eseq;
   mem_blk_t expr;
   mem_blk_t xtmp;

   assert(ctx    != NULL);
   assert(script != NULL);
   assert(exec   != NULL);

   /*
    * Check for predefined function
    *
    */
   for (j=0; j<sizeof(_func_lex)/sizeof(_func_lex[0]); j++) {
      if (script->c < _func_lex[j].info.c) 
         continue;
      if (MemCmp(_func_lex[j].info.p, script->p, _func_lex[j].info.c)) 
         continue;
      /* '(' shall remain */
      script->p += _func_lex[j].info.c - 1;
      script->c -= _func_lex[j].info.c - 1;
      break;
   }
   if (j == sizeof(_func_lex)/sizeof(_func_lex[0])) {
      SET_COMPILE_ERROR(ctx, script->p, "unknown function");
      return true;
   }

   /*
    * Function call is:
    * name(<expression1>, ... <expressionn>) ';'{0,}
    * Script to parse is:
    * (<expression1>, ... <expressionn>) ';'{0,}
    *
    */

#ifdef COMPILER_TRACE      
   RIA_TRACE_MSG("Compiling function ");
   RIA_TRACE_STR(_func_lex[j].info.p, _func_lex[j].info.c-1);
   RIA_TRACE_MSG("\n");
#endif

   /*
    * Find end of function call
    *
    */
   eseq = *script;
   if (!ria_find_char(&p, &eseq, ')', 0))
      return false;
   if (p == NULL) {
      SET_COMPILE_ERROR(ctx, script->p, "unterminated expression");
      return true;
   }
   eseq.c = p - eseq.p;

   /*
    * Iterate thru expressions
    *
    */
   if (eseq.p[0] != '(')
      ERR_SET(err_internal);
   eseq.p++;
   eseq.c--;
   xtmp = *exec;
   for (i=0; eseq.c>0; ) {

      /* 
       * Extract expression
       *
       */
      if (!ria_find_char(&p, &eseq, ',', 0))
         return false;
      if (p == NULL) 
         expr = eseq;
      else {
         expr.p = eseq.p;
         expr.c = p - eseq.p;
      }

      /* 
       * Compile expression
       *
       */
      p = xtmp.p;
      c = xtmp.c;
      if (!ria_compile_expression(&xtmp, &expr, ctx))
         return false;
      if (!ctx->ok)
         return true;
      if (ctx->expr_type == ria_unknown)
         ERR_SET(err_internal);
      eseq.c -= expr.p - eseq.p;
      eseq.p  = expr.p;
      if (eseq.c != 0) {
         if (eseq.p[0] != ',')
            ERR_SET(err_internal);
         eseq.c--;
         eseq.p++;
      }          

      /* 
       * Advance compilation pointer
       *
       */
      if (p != xtmp.p)
         ERR_SET(err_internal);
      xtmp.p += xtmp.c;
      xtmp.c  = c - xtmp.c;
      i++;

   }

   /*
    * Check parameters 
    *
    */
   if (i != _func_lex[j].params) {
      SET_COMPILE_ERROR(ctx, script->p, "wrong number of parameters");
      return true;
   }

   /*
    * Advance parsing pointer
    *
    */
   script->c -= eseq.p - script->p + 1;
   script->p  = eseq.p + 1;

   /*
    * Add call command
    *
    */
   c = xtmp.c;
   p = xtmp.p;
   if (!ria_encode_call(&p, &c, ignore_ret, _func_lex[j].info.t))
      return false;
   exec->c = p - exec->p;
   ctx->expr_type = _func_lex[j].ret;
#ifdef COMPILER_TRACE      
   RIA_TRACE_MSG("COMPILED: call ");
   if (xtmp.p[0] & 0x01) {
      RIA_TRACE_INT((p[-2] << 8)|p[-1]);
   }
   else {
      RIA_TRACE_INT(p[-1]);
   }
   RIA_TRACE_MSG("\n");
   RIA_TRACE_MSG("Function call compilation finished\n");
#endif

   /*
    * ';' could be at the end
    *
    */
   if (script->c > 0) 
      if (script->p[0] == ';') {
         script->c--;
         script->p++;
      }
   return true;

}

/*****************************************************************************/
bool 
   ria_compile_chunk(                                            
      mem_blk_t*            IN OUT   exec,
      mem_blk_t*            IN OUT   script,
      ria_compiler_ctx_t*   IN OUT   ctx)
/*
 * Compiles chunk of scenario script into executable representation 
 *
 */
{

   usize i;

   assert(ctx    != NULL);
   assert(script != NULL);
   assert(exec   != NULL);
   
   /*
    * Initial state. Expected chunks are:
    *  - assignment chunk
    *  - statement chunk
    *  - predefined function call chunk
    *
    */

   /*
    * Check for assignment chunk '$'
    *
    */
   if (script->c < 1)
      ERR_SET(err_bad_length);
   switch (script->p[0]) {
   case '$':
      return ria_compile_assignment(exec, script, ctx);
   }
   
   /*
    * Check for statements 
    *
    */
   for (i=0; i<sizeof(_lex)/sizeof(_lex[0]); i++) {
   
      if (script->c < _lex[i].c) 
         continue;
      if (MemCmp(_lex[i].p, script->p, _lex[i].c)) 
         continue;

      /* '(' shall remain */
      script->p += _lex[i].c - 1;
      script->c -= _lex[i].c - 1;
      switch (_lex[i].t) {
      case ria_lex_return:
         return ria_compile_return(exec, script, ctx);
      case ria_lex_if:
         return ria_compile_if(exec, script, ctx);
      case ria_lex_while:
         return ria_compile_while(exec, script, ctx);
      default:
         ERR_SET(err_internal);
      }

   }

   /*
    * Should be predefined function
    *
    */
   return ria_compile_func(exec, script, true, ctx);

}

/*****************************************************************************/
bool 
   ria_compile_script(                                            
      mem_blk_t*            IN OUT   exec,
      mem_blk_t*            IN OUT   script,
      ria_compiler_ctx_t*   IN OUT   ctx)
/*
 * Compiles single scenario script 
 *
 */
{

   usize i;
   mem_blk_t te;

   assert(ctx    != NULL);
   assert(script != NULL);
   assert(exec   != NULL);

   /*
    * Initialize context 
    *
    */
   buf_set_empty(&ctx->varp);
   buf_set_empty(&ctx->varc);

   /*
    * Compile chunk by chunk
    *
    */
   for (i=0; script->c>0; i+=te.c) {
      te.p = exec->p + i;
      te.c = exec->c - i;
      if (!ria_compile_chunk(&te, script, ctx))
         return false;
      if (!ctx->ok)
         return true;
   }

   /*
    * Finalize
    *
    */
   if ((i == 0) || (exec->p[i-1] != ria_opcode_ret)) {
      if (exec->c < i+1)
         ERR_SET(err_bad_length);
      exec->p[i] = ria_opcode_retn;
      i++;   
   }
   exec->c = i;
   return true;

}


/******************************************************************************
 *  Async operation context
 */
 
#ifdef USE_RIA_ASYNC_CALLS   
 
/*****************************************************************************/
void 
   ria_async_create(     
      ria_async_t*   IN OUT   ctx)
/*
 * Creates async operation context
 *
 */
{

   assert(ctx != NULL);

   MemSet(ctx, 0x00, sizeof(*ctx));
   ctx->callback = NULL;
   ctx->param    = NULL;
   ctx->owner    = NULL;
   ctx->state    = (uint)-1;
   
   sync_mutex_create(&ctx->sync);

} 

/*****************************************************************************/
void 
   ria_async_destroy(     
      ria_async_t*   IN OUT   ctx)
/*
 * Destroys async operation context
 *
 */
{
   sync_mutex_destroy(&ctx->sync);
}

/*****************************************************************************/
void 
   ria_async_set_state(     
      ria_async_t*               IN OUT   ctx,
      uint                       IN       state,
      struct ria_exec_state_s*   IN       owner)
/*
 * Sets async operation state
 *
 */
{

   assert(ctx   != NULL);
   assert(owner != NULL);
   
   sync_mutex_lock(&ctx->sync);
   ctx->state = state;
   ctx->owner = owner;
   ctx->owner->status = ria_exec_transit;
   sync_mutex_unlock(&ctx->sync);
   
} 

/*****************************************************************************/
void 
   ria_async_set_param(     
      ria_async_t*   IN OUT   ctx,
      void*          IN       param)
/*
 * Sets async operation parameter
 *
 */
{

   assert(ctx   != NULL);
   
   sync_mutex_lock(&ctx->sync);
   ctx->param = param;
   sync_mutex_unlock(&ctx->sync);
   
}

/*****************************************************************************/
uint
   ria_async_set_status(     
      ria_async_t*   IN OUT   ctx,
      uint           IN       status)
/*
 * Set async operation status
 *
 */
{

   assert(ctx        != NULL);
   assert(ctx->owner != NULL);
   
   sync_mutex_lock(&ctx->sync);
   
   switch (status) {
   case ria_exec_pending:
      switch (ctx->owner->status) {
      case ria_exec_proceed:
         break;
      case ria_exec_transit:
         ctx->owner->status = ria_exec_pending;
         break;
      default:
         ERR_SET_NO_RET(err_internal);
      }
      break;
      
   case ria_exec_proceed:
      switch (ctx->owner->status) {
      case ria_exec_pending:
      case ria_exec_transit:
         ctx->owner->status = ria_exec_proceed;
         break;
      default:
         ERR_SET_NO_RET(err_internal);
      }
      break;
   
   default:
      ERR_SET_NO_RET(err_internal);
   }
   
   status = ctx->owner->status;
   sync_mutex_unlock(&ctx->sync);
   return status;
   
}

#endif


/******************************************************************************
 *  API
 */

enum {
   cf_ria_compiler_varp    = 0x01,
   cf_ria_compiler_varc    = 0x02,
   cf_ria_compiler_strp    = 0x04,
   cf_ria_compiler_strc    = 0x08,
   cf_ria_compiler_table   = 0x10,
   cf_ria_compiler_globalp = 0x20,
   cf_ria_compiler_globalc = 0x40
};

/*****************************************************************************/
bool 
   ria_compiler_create(     
      ria_compiler_ctx_t*   IN OUT   ctx,
      heap_ctx_t*           IN OUT   mem)
/*
 * Creates compiler context
 *
 */
{

   assert(ctx != NULL);
   assert(mem != NULL);

   MemSet(ctx, 0x00, sizeof(*ctx));
   ctx->mem = mem;

   if (!buf_create(sizeof(byte*), 0, 0, &ctx->varp, ctx->mem))
      goto failed;
   else
      ctx->cleanup |= cf_ria_compiler_varp;
   if (!buf_create(sizeof(usize), 0, 0, &ctx->varc, ctx->mem))
      goto failed;
   else
      ctx->cleanup |= cf_ria_compiler_varc;
   if (!buf_create(sizeof(byte*), 0, 0, &ctx->strp, ctx->mem))
      goto failed;
   else
      ctx->cleanup |= cf_ria_compiler_strp;
   if (!buf_create(sizeof(usize), 0, 0, &ctx->strc, ctx->mem))
      goto failed;
   else
      ctx->cleanup |= cf_ria_compiler_strc;
   if (!buf_create(sizeof(byte*), 0, 0, &ctx->globalp, ctx->mem))
      goto failed;
   else
      ctx->cleanup |= cf_ria_compiler_globalp;
   if (!buf_create(sizeof(usize), 0, 0, &ctx->globalc, ctx->mem))
      goto failed;
   else
      ctx->cleanup |= cf_ria_compiler_globalc;
   if (!buf_create(sizeof(byte), 0, 0, &ctx->table, ctx->mem))
      goto failed;
   else
      ctx->cleanup |= cf_ria_compiler_table;
   return true;

failed:
   ria_compiler_destroy(ctx);
   return false;

}

/*****************************************************************************/
bool 
   ria_compiler_destroy(     
      ria_compiler_ctx_t*   IN OUT   ctx)
/*
 * Destroys compiler context
 *
 */
{

   bool ret = true;

   assert(ctx != NULL);

   if (ctx->cleanup & cf_ria_compiler_varp)
      ret = buf_destroy(&ctx->varp) && ret;
   if (ctx->cleanup & cf_ria_compiler_varc)
      ret = buf_destroy(&ctx->varc) && ret;
   if (ctx->cleanup & cf_ria_compiler_strp)
      ret = buf_destroy(&ctx->strp) && ret;
   if (ctx->cleanup & cf_ria_compiler_strc)
      ret = buf_destroy(&ctx->strc) && ret;
   if (ctx->cleanup & cf_ria_compiler_globalp)
      ret = buf_destroy(&ctx->globalp) && ret;
   if (ctx->cleanup & cf_ria_compiler_globalc)
      ret = buf_destroy(&ctx->globalc) && ret;
   if (ctx->cleanup & cf_ria_compiler_table)
      ret = buf_destroy(&ctx->table) && ret;

   ctx->cleanup = 0;
   return ret;

}

/*****************************************************************************/
bool 
   ria_canonize_script(                                            
      mem_blk_t*   IN OUT   script)
/*
 * Converts scenario script into canonic form
 *
 */
{

   usize i;
   bool escaped, quoted, transit, comment;
   byte *q;

   assert(script != NULL);

   /*
    * Remove blanks
    * Remove comments
    * Canonize character case
    *
    */
   q = script->p; 
   for (i=0, quoted=escaped=comment=false; i<script->c; i++) {
      if (comment) {
         switch (script->p[i]) {
         case '\r':
         case '\n':
            comment = false;
         }
         continue;
      }   
      transit = ((script->p[i] == '"') && (!escaped));
      if (quoted) {
         escaped = ((script->p[i] == '\\') && (!escaped));
      }
      else
         switch (script->p[i]) {
         case ' ' :
         case '\r':
         case '\n':
         case '\t':
            continue;
         case '/':
            if (i+1 < script->c) 
               if (script->p[i+1] == '/') {
                  i++;
                  comment = true;
                  continue;
               }
            break;   
         default:
            if ((script->p[i] >= 'A') && (script->p[i] <= 'Z'))
               script->p[i] = (byte)(script->p[i] - 'A' + 'a'); 
         }
      *(q++) = script->p[i];   
      if (transit)
         quoted = !quoted;
   }

   /*
    * Check spaces in the end
    *
    */
   script->c = q - script->p;
   return true;

}

/*****************************************************************************/
bool 
   ria_compile_script_module(                                            
      mem_blk_t*            IN OUT   exec,
      mem_blk_t*            IN OUT   script,
      ria_compiler_ctx_t*   IN OUT   ctx)
/*
 * Compiles scenario script module into executable representation 
 *
 */
{

   static const char _global[] = "global($";
   static const char _int[]    = "int)";
   static const char _str[]    = "string)";
   static const char _bool[]   = "boolean)";

   usize i, j, k, l, m, c;
   mem_blk_t te;
   mem_blk_t ts;
   byte*  p;
   byte*  q;
   byte** pp;
   usize* pc;
   ria_type_t type;

   assert(ctx    != NULL);
   assert(script != NULL);
   assert(exec   != NULL);

   /*
    * Canonize script
    *
    */
   if (!ria_canonize_script(script))
      return false;    
    
   ts.p = script->p;
   ts.c = script->c;

   /*
    * Initialize context
    *
    */
   ctx->ok  = true;
   ctx->pstart = script->p;
   buf_set_empty(&ctx->strp);
   buf_set_empty(&ctx->strc);
   buf_set_empty(&ctx->table);

   /*
    * Compile script by script
    *
    */
   te.c = 0; 
   for (i=l=0; ts.c>0; i+=te.c) {
   
      /*
       * Detect global variable declaration
       *
       */
      if (ts.c > sizeof(_global)-1)
         if (!StrNICmp((char*)ts.p, _global, sizeof(_global)-1)) {
            ts.c -= sizeof(_global) - 2;
            ts.p += sizeof(_global) - 2;
            if (!ria_find_var(&j, &k, &ts, ctx))
               return false;
            if (k != ria_var_unknown) {
               SET_COMPILE_ERROR(ctx, ts.p, "var was already defined somewhere");
               return true;
            }
            k = j;
            type = ria_unknown;
            if (ts.p[j] == ':') {
               /*
                * Process type assignment
                *
                */
               j++;  
               if (ts.c >= sizeof(_int)+j)
                  if (!StrNICmp((char*)ts.p+j, _int, sizeof(_int)-1)) {
                     type = ria_int;
                     j += sizeof(_int) - 2;
                  }   
               if (type == ria_unknown)      
                  if (ts.c >= sizeof(_str)+j)
                     if (!StrNICmp((char*)ts.p+j, _str, sizeof(_str)-1)) {
                        type = ria_string;
                        j += sizeof(_str) - 2;
                     }   
               if (type == ria_unknown)      
                  if (ts.c >= sizeof(_bool)+j)
                     if (!StrNICmp((char*)ts.p+j, _bool, sizeof(_bool)-1)) {
                        type = ria_boolean;
                        j += sizeof(_bool) - 2;
                     }   
               if (type == ria_unknown) {
                  SET_COMPILE_ERROR(ctx, ts.p+j, "syntax error");
                  return true;
               }   
            }
            if (ts.p[j] != ')') {
               SET_COMPILE_ERROR(ctx, ts.p, "syntax error");
               return true;
            }
            if (!ria_create_var(&k, ts.p, k, type, true, ctx))
               return true;
            ts.c -= j + 1;
            ts.p += j + 1;
            continue;                       
         } 
       
      if (!ria_find_char(&p, &ts, '(', 0))
         return false;
      if (p == NULL) {
         SET_COMPILE_ERROR(ctx, ts.p, "syntax error");
         return true;
      }


      /*
       * Extract script name 
       *
       */
      if (!ria_find_char(&p, &ts, '(', 0))
         return false;
      if (p == NULL) {
         SET_COMPILE_ERROR(ctx, ts.p, "syntax error");
         return true;
      }

      /*
       * Add name to table
       *
       */
      j = buf_get_length(&ctx->table);
      if (!buf_expand(j+p-ts.p+5, &ctx->table))
         return false;
      q = buf_get_ptr_bytes(&ctx->table) + j;
      if (!buf_set_length(j+p-ts.p+5, &ctx->table))
         return false;
      *(q++) = (byte)(p-ts.p);
      MemCpy(q, ts.p, p-ts.p);
      q    += p - ts.p;
      ts.c -= p - ts.p + 1;
      ts.p  = p + 1;

      /*
       * Extract number of parameters 
       *
       */
      if (!ria_find_char(&p, &ts, ')', 1))
         return false;
      if (p == NULL) {
         SET_COMPILE_ERROR(ctx, ts.p, "syntax error");
         return true;
      }
      for (j=0; ts.p<p; ts.p++, ts.c--) {
         if ((ts.p[0] < '0') || (ts.p[0] > '9')) {
            SET_COMPILE_ERROR(ctx, ts.p, "syntax error");
            return true;
         }
         j = j*10 + ts.p[0] - '0';
      }
      *(q++) = (byte)j;
      ts.p++;
      ts.c--;

      /*
       * Write entry point
       *
       */
      *(q++) = (byte)(i >> 16);
      *(q++) = (byte)(i >>  8);
      *(q++) = (byte)(i >>  0);

      /*
       * Extract script body
       *
       */
      if ((ts.c < 1) || (ts.p[0] != '{')) {
         SET_COMPILE_ERROR(ctx, ts.p, "syntax error");
         return true;
      }
      ts.p++;
      ts.c--;
      if (!ria_find_char(&p, &ts, '}', 1))
         return false;
      if (p == NULL) {
         SET_COMPILE_ERROR(ctx, ts.p, "syntax error");
         return true;
      }
      c = ts.c;
      ts.c = p - ts.p;
      c -= ts.c;

      /*
       * Compile
       *
       */
      te.p = exec->p + i;
      te.c = exec->c - i;
      if (!ria_compile_script(&te, &ts, ctx))
         return false;
      if (!ctx->ok)
         return true;
      if (ts.p != p) 
         ERR_SET(err_internal);
      ts.p++;
      ts.c = c - 1;

      l++;

   }

   /*
    * Check number of functions
    *
    */
   if (l > 0xFF)
      ERR_SET(err_not_supported);

   /*
    * Asjust entry points
    *
    */
   m = ria_header_size + buf_get_length(&ctx->table);
   for (k=buf_get_length(&ctx->table), 
        p=buf_get_ptr_bytes(&ctx->table); k>0; ) {
      j = *p;
      if (k < j+1)
         ERR_SET(err_internal);
      k -= j + 1;
      p += j + 1;
      if (k < 4)
         ERR_SET(err_internal);
      c  = (p[1] << 16) | (p[2] << 8) | p[3];
      c += m;
      p[1] = (byte)(c >> 16);
      p[2] = (byte)(c >>  8);
      p[3] = (byte)(c >>  0);
      p += 4;
      k -= 4;
   }

   /*
    * Write the header
    *
    */
   te.p = exec->p + i;
   te.c = exec->c - i;
   if (te.c < m) 
      ERR_SET(err_bad_length);
   MemMove(exec->p+m, exec->p, i);
   i += m;
   exec->p[0] = (byte)l;
   exec->p[1] = (byte)(i >> 16);
   exec->p[2] = (byte)(i >>  8);
   exec->p[3] = (byte)(i >>  0);
   MemCpy(
      exec->p+ria_header_size, 
      buf_get_ptr_bytes(&ctx->table), 
      buf_get_length(&ctx->table));

   /*
    * Embed string constants into executable format
    *
    */
   c  = buf_get_length(&ctx->strp);
   pp = buf_get_ptr_ptrs(&ctx->strp);
   pc = buf_get_ptr_usizes(&ctx->strc);
   for (j=0; j<c; j++) {
      te.p = exec->p + i;
      te.c = exec->c - i;
      if (te.c < pc[j]+2)
         ERR_SET(err_internal);
      if (pc[j] >= 0xFF)
         ERR_SET(err_internal);
      for (k=l=0; k<pc[j]; k++, l++) {
         if ((pp[j])[k] == '\\') {
            if (k+1 == pc[j])
               ERR_SET(err_internal);
            k++;
            switch ((pp[j])[k]) {
            case 't':
               te.p[l+1] = 0x09;
               break;
            case 'r':
               te.p[l+1] = 0x0D;
               break;
            case 'n':
               te.p[l+1] = 0x0A;
               break;
            default:
               te.p[l+1] = (pp[j])[k];
            }
         }
         else
            te.p[l+1] = (pp[j])[k];
      }
      te.p[0]   = (byte)(l + 1);
      te.p[l+1] = 0x00;
      i += l + 2;
   }

   /*
    * Finalize
    *
    */
   exec->c = i;
   return true;

}

