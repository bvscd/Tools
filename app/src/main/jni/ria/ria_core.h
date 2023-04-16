#ifndef _RIA_CORE
#define _RIA_CORE


#include "emb_defs.h"
#include "emb_heap.h"
#include "emb_buff.h"


/*
 * Support for async operations
 *
 */
//#define USE_RIA_ASYNC_CALLS 
#ifdef USE_RIA_ASYNC_CALLS
#define RIA_ASYNC_CONNECT
#define RIA_ASYNC_RECEIVE
#endif

/*
 * Forwards
 *
 */
struct ria_exec_state_s;

/*
 *  Tracing 
 *
 */

#define USE_RIA_TRACE

#ifdef USE_RIA_TRACE
#if defined(ANDROID)

void 
   do_android_log(
      char*         IN OUT   buf,
      uint*         IN OUT   cval,
      uint          IN       cbuf,
      const char*   IN       str,
      uint          IN       len,
      bool          IN       flush);

#define RIA_TRACE_START                                                       \
   {                                                                          \
      char _sz[100];                                                          \
      uint _cc = 0;                                                 
#define RIA_TRACE_MSG(_s)                                                     \
   {                                                                          \
      do_android_log(_sz, &_cc, sizeof(_sz), _s, StrLen(_s), false);          \
   } 
#define RIA_TRACE_STR(_p, _c)                                                 \
   {                                                                          \
      do_android_log(_sz, &_cc, sizeof(_sz), _p, _c, false);                  \
   }
#define RIA_TRACE_INT(_i)                                                     \
   {                                                                          \
      char _si[15];                                                           \
      Sprintf1(_si, sizeof(_si), "%d", _i);                                   \
      do_android_log(_sz, &_cc, sizeof(_sz), _si, StrLen(_si), false);        \
   } 
#define RIA_TRACE_STOP                                                        \
      do_android_log(_sz, &_cc, sizeof(_sz), NULL, 0, true);                  \
   }
   
#else /* ANDROID */

#define RIA_TRACE_START                                                       
#define RIA_TRACE_MSG(s)                                                      \
   {                                                                          \
      Fprintf(Stdout, s);                                                     \
      Fflush(Stdout);                                                         \
   } 
#define RIA_TRACE_STR(_p, _c)                                                 \
   {                                                                          \
      char _sz[100];                                                          \
      usize _i, _j;                                                           \
      for (_i=_j=0; (_i<sizeof(_sz)-1)&&(_j<(unsigned)(_c)); _j++) {          \
         if (_p[_j] == '%') {                                                 \
            if (_i == sizeof(_sz)-2)                                          \
               break;                                                         \
            _sz[_i++] = '%';                                                  \
         }                                                                    \
         _sz[_i++] = _p[_j];                                                  \
      }                                                                       \
      _sz[_i] = 0x00;                                                         \
      Fprintf(Stdout, _sz);                                                   \
      Fflush(Stdout);                                                         \
   }
#define RIA_TRACE_INT(_i)                                                     \
   {                                                                          \
      Fprintf(Stdout, "%d", _i);                                              \
      Fflush(Stdout);                                                         \
   } 
#define RIA_TRACE_STOP

#endif /* !ANDROID */       
#else  /* !USE_RIA_TRACE */
#define RIA_TRACE_START
#define RIA_TRACE_MSG(s)                                                      
#define RIA_TRACE_STR(_p, _c)                                                 
#define RIA_TRACE_INT(_i)                                                     
#define RIA_TRACE_STOP
#endif

/*
 * Executable module format:
 * COOOLN...NKPPP...LN...NKPPPE...ELS...S...LS...S
 * C - number of functions in module (1 byte)
 * O - offset of strings table (3 bytes)
 * L - length of next field (1 byte)
 * N - function name
 * K - number of function parameters (1 byte)
 * P - offset of function entry point (3 bytes)
 * E - executable code
 * S - string data
 *
 */
enum {
   ria_header_size = 0x04,
   ria_fextra_size = 0x05
};

/* 
 * Internal contants for executor
 *
 */
enum {
   ria_var_threshold = 128
};

/*
 * Data types 
 *
 */
typedef enum _ria_type_e {
   ria_unknown = 0x00,
   ria_string  = 0x01,
   ria_int     = 0x02,
   ria_boolean = 0x03
} ria_type_t;

/*

  Push-value-of-variable-to-stack (pushv x)
  000000-- xxxxxxxx
           index of variable (local: 0..127, global: 128...255)

  Push-value-of-parameter-to-stack (pushp x)
  000011-- xxxxxxxx
           index of parameter

  Push-string-constant-to-stack (pushs x)
  000001-x yyyyyyyy yyyyyyyy
         0 - string index is 1 octet
         1 - string index is 2 octets
         
  Push-integer-constant-to-stack (pushi x);
  000010xx yyyyyyyy ... yyyyyyyy
        number of consecutive bytes with value minus 1

  Evaluate-operation: (op x)
  0010xxxx         string        int      bool
      0000 -         +            +        ||
      0001 -         <            <
      0010 -         >            >
      0011 -         <=           <=
      0100 -         >=           >=
      0101 -         ==           ==       ==
      0110 -         !=           !=       !=
      0111 -                      -
      1000 -                      *        &&
      1001 -                      /
      1010 -                      %
      1011 -                      &
      1100 -                      |
      1101 -                      ^
      1110 -                      ~        !     
      1111 -                      -(u)

  Pop-stack-top-element: (pop x)
  0011---- xxxxxxxx
           index of variable (local: 0..127, global: 128...255)

  Evaluate-predefined-function: (call x)
  0001--yx yyyyyyyy yyyyyyyy
         0 - function index is 1 octet
         1 - function index is 2 octets
        0 - push return value to stack
        1 - ignore return value
              
           00000001 - load_cookie(site, user, key)->string
           00000010 - get_html(url)->status code, cache response
           00000011 - last_response()->string
           00000100 - get_header(name)->string
           00000101 - extract_string(src, pos, begin, end)->string
           00000110 - get_html_to_file(filename, url)->status code
           00000111 - extract_string_from_file(file, pos, begin, end)->string
           00001000 - substring(str, pos, len)->string
           00001001 - save_cookie(site, user, key)->none
           00001010 - get_binary_to_file(filename, url)->status code
           00001011 - dehtml(str)->string
           00001100 - get_html_with_dump(url, dump)->status code, cache resp
           00001101 - get_html_to_file_with_dump(file, url, dump)->status code
           00001110 - post(url, values)->status code, cache response
           00001111 - post_to_file(file, url, values)->status code
           00010000 - post_with_dump(url, values, dump)->status code, cache resp
           00010001 - post_to_file_with_dump(file, url, values, dump)->status code
           00010010 - set_header(name, value)->status code
           00010011 - length(str)->length
           00010100 - int_to_string(int)->string
           00010101 - create_parser_for_file(filename, type)->none
           00010110 - add_parsing_rule(name, begin, end, hint)->none
           
  Conditional-jump: (jit/jif x)
  0100x--y zzzzzzzz zzzzzzzz
         0 - jump offset is 1 octet
         1 - jump offset is 2 octets
      0 - jump if stack top is not true
      1 - jump if stack top is true

  Unconditional-jump: (jmp x)
  0101---y zzzzzzzz zzzzzzzz
         0 - jump offset is 1 octet
         1 - jump offset is 2 octets

  Stop execution: (ret)
  0110---x
         0 - pop stack top
         1 - do not touch stack

*/

/*
 * Operation codes
 *
 */
enum {
   ria_opcode_pushv   = 0x00,
   ria_opcode_pushs   = 0x04,
   ria_opcode_pushs2  = 0x05,
   ria_opcode_pushi1  = 0x08,
   ria_opcode_pushi2  = 0x09,
   ria_opcode_pushi3  = 0x0A,
   ria_opcode_pushi4  = 0x0B,
   ria_opcode_pushp   = 0x0C,
   ria_opcode_callp   = 0x10,
   ria_opcode_call2p  = 0x11,
   ria_opcode_calli   = 0x12,
   ria_opcode_call2i  = 0x13,
   ria_opcode_add     = 0x20,
   ria_opcode_lor     = 0x20,
   ria_opcode_less    = 0x21,
   ria_opcode_more    = 0x22,
   ria_opcode_less_eq = 0x23,
   ria_opcode_more_eq = 0x24,
   ria_opcode_eq      = 0x25,
   ria_opcode_not_eq  = 0x26,
   ria_opcode_sub     = 0x27,
   ria_opcode_land    = 0x27,
   ria_opcode_mul     = 0x28,
   ria_opcode_div     = 0x29,
   ria_opcode_rem     = 0x2A,
   ria_opcode_band    = 0x2B,
   ria_opcode_bor     = 0x2C,
   ria_opcode_xor     = 0x2D,
   ria_opcode_bneg    = 0x2E,
   ria_opcode_not     = 0x2E,
   ria_opcode_neg     = 0x2F,
   ria_opcode_pop     = 0x30,
   ria_opcode_jif     = 0x40,
   ria_opcode_jif2    = 0x41,
   ria_opcode_jit     = 0x48,
   ria_opcode_jit2    = 0x49,
   ria_opcode_jmp     = 0x50,
   ria_opcode_jmp2    = 0x51,
   ria_opcode_ret     = 0x60,
   ria_opcode_retn    = 0x61
};

/*
 * Function codes
 *
 */
enum {
   ria_func_load_cookie                = 0x01,
   ria_func_get_html                   = 0x02,
   ria_func_last_response              = 0x03,
   ria_func_get_header                 = 0x04,
   ria_func_extract_string             = 0x05,
   ria_func_get_html_to_file           = 0x06,
   ria_func_extract_string_from_file   = 0x07,
   ria_func_substring                  = 0x08,
   ria_func_save_cookie                = 0x09,
   ria_func_get_binary_to_file         = 0x0A,
   ria_func_dehtml                     = 0x0B,
   ria_func_get_html_with_dump         = 0x0C,
   ria_func_get_html_to_file_with_dump = 0x0D,
   ria_func_post                       = 0x0E,
   ria_func_post_to_file               = 0x0F,
   ria_func_post_with_dump             = 0x10,
   ria_func_post_to_file_with_dump     = 0x11,
   ria_func_set_header                 = 0x12,
   ria_func_length                     = 0x13,
   ria_func_int_to_string              = 0x14,
   ria_func_create_parser_for_file     = 0x15,
   ria_func_add_parsing_rule           = 0x16,
   ria_func_load_from_file             = 0x17,
   ria_func_save_to_file               = 0x18,
   ria_func_string_to_int              = 0x19
};

/*
 * Function control flags
 *
 */
enum {
   ria_func_dst_ready = 0x01,  /* Destination buffer ready-to-use after call */
}; 

/*
 * Predefined function implementation
 *
 * Parameters:     dst            result buffer
 *                 flags          control flags, see above
 *                 ppar           pointer to parameters
 *                 cpar           size of parameters
 *                 ctx            execution context
 *
 * Return:         true           if successful,
 *                 false          if failed
 *
 */
typedef 
   bool function_fn(
      buf_t*                     IN OUT   dst,
      umask*                     OUT      flags,
      byte*                      IN       ppar,
      usize                      IN       cpar,
      struct ria_exec_state_s*   IN OUT   ctx);

/*
 * Callback function for async operations
 *
 */
#ifdef USE_RIA_ASYNC_CALLS   
/*
 * Callback function 
 *
 * Parameters:     ctx            execution context
 *
 * Return:         true           if successful,
 *                 false          if failed
 *
 */
typedef 
   bool callback_fn(
      struct ria_exec_state_s*   IN OUT   ctx);
#endif

/*
 * Async operation context
 *
 */
#ifdef USE_RIA_ASYNC_CALLS   
typedef struct ria_async_s {
   generic_mutex_t            sync;
   uint                       state;
   void*                      param;
   struct ria_exec_state_s*   owner;
   callback_fn*               callback;
} ria_async_t;  
#endif   

#ifdef USE_RIA_ASYNC_CALLS   

/*@@ria_async_create
 *
 * Creates async operation context
 *
 * Parameters:     ctx            async context
 *
 * Return:         none
 *
 */
void 
   ria_async_create(     
      ria_async_t*   IN OUT   ctx);

/*@@ria_async_destroy
 *
 * Destroys async operation context
 *
 * Parameters:     ctx            async context
 *
 * Return:         none
 *
 */
void 
   ria_async_destroy(     
      ria_async_t*   IN OUT   ctx);

/*@@ria_async_set_state
 *
 * Sets async operation state
 *
 * Parameters:     ctx            async context
 *                 state          state data
 *                 owner          owner data
 *
 * Return:         none
 *
 */
void 
   ria_async_set_state(     
      ria_async_t*               IN OUT   ctx,
      uint                       IN       state,
      struct ria_exec_state_s*   IN       owner);
      
/*@@ria_async_set_param
 *
 * Sets async operation parameter
 *
 * Parameters:     ctx            async context
 *                 param          parameter
 *
 * Return:         none
 *
 */
void 
   ria_async_set_param(     
      ria_async_t*   IN OUT   ctx,
      void*          IN       param);
      
/*@@ria_async_set_status
 *
 * Set async operation status
 *
 * Parameters:     ctx            async context
 *                 status         status to set
 *
 * Return:         resulting status
 *
 */
uint 
   ria_async_set_status(     
      ria_async_t*   IN OUT   ctx,
      uint           IN       status);

#endif

/*
 * Configuration 
 *
 */
typedef struct ria_config_s {
   buf_t   workdir;
   buf_t   tempdir;
   umask   cleanup;
} ria_config_t; 

/*
 * Configuration options
 *
 */
typedef enum ria_option_e {
   ria_option_workdir,
   ria_option_tempdir
} ria_option_t; 

/*
 * Compiler context
 *
 */
typedef struct ria_compiler_ctx_s {
   bool          ok;
   const byte*   pstart;
   const byte*   perror;
   const char*   errmsg;
   ria_type_t    expr_type;
   buf_t         globalp;
   buf_t         globalc;
   buf_t         varp;
   buf_t         varc;
   buf_t         strp;
   buf_t         strc;
   buf_t         table;
   heap_ctx_t*   mem;
   umask         cleanup;
} ria_compiler_ctx_t;

/*@@ria_compiler_create
 *
 * Creates compiler context
 *
 * Parameters:     ctx            compiler context
 *                 mem            memory context
 *
 * Return:         true           if successful
 *                 false          if failed
 *
 */
bool 
   ria_compiler_create(     
      ria_compiler_ctx_t*   IN OUT   ctx,
      heap_ctx_t*           IN OUT   mem);

/*@@ria_compiler_destroy
 *
 * Destroys compiler context
 *
 * Parameters:     ctx            compiler context
 *
 * Return:         true           if successful
 *                 false          if failed
 *
 */
bool 
   ria_compiler_destroy(     
      ria_compiler_ctx_t*   IN OUT   ctx);

/*@@ria_compile_script_module
 *
 * Compiles scenario script module into executable representation 
 *
 * Parameters:     exec           buffer with executable data
 *                 script         buffer with script data
 *                 ctx            compiler context
 *
 * Return:         true           if successful
 *                 false          if failed
 *
 */
bool 
   ria_compile_script_module(     
      mem_blk_t*            IN OUT   exec,
      mem_blk_t*            IN OUT   script,
      ria_compiler_ctx_t*   IN OUT   ctx);

/*@@ria_get_function_info
 *
 * Returns information about predefind function 
 *
 * Parameters:     type           result type
 *                 params         number of params
 *                 impl           implementation pointer
 *                 func           function code
 *
 * Return:         true           if successful
 *                 false          if failed
 *
 */
bool 
   ria_get_function_info(                   
      ria_type_t*     OUT   type,
      unumber*        OUT   params,
      function_fn**   OUT   impl,
      unumber         IN    func);


#endif

