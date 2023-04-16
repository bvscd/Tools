#ifndef _RIA_PARS
#define _RIA_PARS


#include "ria_core.h"


/*
 * Forwards
 *
 */
struct ria_exec_state_s; 

/*
 * Parser types
 *
 */
typedef enum ria_parser_type_e {
   ria_parser_unknown,
   ria_parser_json
} ria_parser_type_t;

/*
 * String extraction modes
 *
 */
typedef enum ria_string_extract_mode_e {
   ria_string_extract_begin_end_pos_only,
   ria_string_extract_begin_end,
   ria_string_extract_begin_only_pos_only
} ria_string_extract_mode_t;

/*
 * Rule flags
 *
 */
typedef enum ria_rule_flags_e {
   ria_rule_f_max_pos_mask      = 0x00FFFFFF,
   ria_rule_f_iteration_started = 0x01000000
} ria_rule_flags_t;

/*
 * String extraction context
 *
 */
typedef struct ria_extract_params_s {
   usize*                      pos;
   const char*                 pbegin;
   usize                       cbegin;
   const char*                 pend;
   usize                       cend;
   const char*                 pexcludes;
   usize                       cexcludes;
   buf_t*                      tmp;
   ria_string_extract_mode_t   mode;
} ria_extract_params_t;

/*
 * Parsing rule 
 *
 */
typedef struct ria_parser_rule_s {
   list_entry_t   linkage;
   umask          flags;
   usize          pos;
   const char*    pname;
   usize          cname;
   const char*    pbegin;
   usize          cbegin;
   const char*    pend;
   usize          cend;
   const char*    phint;
   usize          chint;
} ria_parser_rule_t;

/*
 * Parser 
 *
 */
typedef struct ria_parser_s {
   ria_parser_type_t   type;
   buf_t               src;
   buf_t               tmp;
   list_entry_t        rules;
   heap_ctx_t*         mem;
   umask               cleanup;
} ria_parser_t;

/*@@ria_parser_create
 *
 * Creates parser context
 *
 * Parameters:     ctx            parser context
 *                 mem            memory context
 *
 * Return:         true           if successful
 *                 false          if failed
 *
 */
bool 
   ria_parser_create(     
      ria_parser_t*   IN OUT   ctx,
      heap_ctx_t*     IN OUT   mem);

/*@@ria_parser_destroy
 *
 * Destroys parser context
 *
 * Parameters:     ctx            parser context
 *
 * Return:         true           if successful
 *                 false          if failed
 *
 */
bool 
   ria_parser_destroy(     
      ria_parser_t*   IN OUT   ctx);

/*@@ria_parser_reset
 *
 * Resets parser context
 *
 * Parameters:     ctx            parser context
 *                 psrc           attached source
 *                 csrc           source size, chars
 *                 is_src_file    true if source is file
 *                 ptype          parser type string
 *                 ctype          parser type size, chars
 *
 * Return:         true           if successful
 *                 false          if failed
 *
 */
bool 
   ria_parser_reset(     
      ria_parser_t*   IN OUT   ctx,
      const char*     IN       psrc,
      usize           IN       csrc,
      bool            IN       is_src_file,
      const char*     IN       ptype,
      usize           IN       ctype);

/*@@ria_parser_add_rule
 *
 * Adds parsing rule
 *
 * Parameters:     ctx            parser context
 *                 pname          rule name
 *                 cname          rule name size, chars
 *                 pbegin         rule begin
 *                 cbegin         rule begin size, chars
 *                 pend           rule end
 *                 cend           rule end size, chars
 *                 phint          rule hint
 *                 chint          rule hint size, chars
 *
 * Return:         true           if successful
 *                 false          if failed
 *
 */
bool 
   ria_parser_add_rule(     
      ria_parser_t*   IN OUT   ctx,
      const char*     IN       pname,
      usize           IN       cname,
      const char*     IN       pbegin,
      usize           IN       cbegin,
      const char*     IN       pend,
      usize           IN       cend,
      const char*     IN       phint,
      usize           IN       chint);

/*@@ria_parser_find_rule
 *
 * Finds parsing rule
 *
 * Parameters:     rule           found rule
 *                 ctx            parser context
 *                 pname          rule name
 *                 cname          rule name size, chars
 *
 * Return:         true           if successful
 *                 false          if failed
 *
 */
bool 
   ria_parser_find_rule(     
      ria_parser_rule_t**   OUT      rule,
      ria_parser_t*         IN OUT   ctx,
      const char*           IN       pname,
      usize                 IN       cname);
      
/*@@ria_extract_string_from_file
 *
 * Extracts string from file
 *
 * Parameters:     ok             success flag
 *                 dst            destination buffer
 *                 pfilename      file name
 *                 cfilename      file name size, chars
 *                 params         extraction parameters
 *                 ctx            execution state
 *
 * Return:         true           if successful
 *                 false          if failed
 *
 */
bool 
   ria_extract_string_from_file(
      bool*                      OUT      ok,
      buf_t*                     IN OUT   dst,
      const char*                IN       pfilename,
      usize                      IN       cfilename,
      ria_extract_params_t*      IN OUT   params,      
      struct ria_exec_state_s*   IN OUT   ctx);

#endif
