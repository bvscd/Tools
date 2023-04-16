#ifndef _RIA_FUNC
#define _RIA_FUNC


#include "ria_core.h"
#include "ria_exec.h"

/*
 * Multi-header delimiter
 *
 */
extern const char* _multiheader;

/*@@ria_implement_<something>
 *
 * <something> implementation
 *
 * Parameters:     dst            result buffer
 *                 flags          control flags
 *                 ppar           pointer to parameters
 *                 cpar           size of parameters
 *                 ctx            execution context
 *
 * Return:         true           if successful,
 *                 false          if failed
 *
 */
#define DECLARE_IMPLEMENT(_f)                                                 \
        bool                                                                  \
           ria_implement_##_f(                                                \
              buf_t*              IN OUT   dst,                               \
              umask*              OUT      flags,                             \
              byte*               IN       ppar,                              \
              usize               IN       cpar,                              \
              ria_exec_state_t*   IN OUT   ctx)

DECLARE_IMPLEMENT(add_parsing_rule);
DECLARE_IMPLEMENT(create_parser_for_file);
DECLARE_IMPLEMENT(dehtml);
DECLARE_IMPLEMENT(extract_string);
DECLARE_IMPLEMENT(extract_string_from_file);
DECLARE_IMPLEMENT(get_binary_to_file);
DECLARE_IMPLEMENT(get_header);
DECLARE_IMPLEMENT(get_html);
DECLARE_IMPLEMENT(get_html_with_dump);
DECLARE_IMPLEMENT(get_html_to_file);
DECLARE_IMPLEMENT(get_html_to_file_with_dump);
DECLARE_IMPLEMENT(int_to_string);
DECLARE_IMPLEMENT(last_response);
DECLARE_IMPLEMENT(length);
DECLARE_IMPLEMENT(load_cookie);
DECLARE_IMPLEMENT(load_from_file);
DECLARE_IMPLEMENT(post);
DECLARE_IMPLEMENT(post_to_file);
DECLARE_IMPLEMENT(post_to_file_with_dump);
DECLARE_IMPLEMENT(post_with_dump);
DECLARE_IMPLEMENT(save_cookie);
DECLARE_IMPLEMENT(save_to_file);
DECLARE_IMPLEMENT(set_header);
DECLARE_IMPLEMENT(string_to_int);
DECLARE_IMPLEMENT(substring);

/*@@ria_pack_func_param
 *
 * Packs function parameter 
 *
 * Parameters:     pdst           buffer pointer
 *                 cdst           buffer counter
 *                 kind           parameter kind
 *                 ppar           parameter pointer
 *
 * Return:         true           if successful
 *                 false          if failed
 *
 */
bool 
   ria_pack_func_param(
      byte**            IN OUT   pdst,
      usize*            IN OUT   cdst,
      ria_data_kind_t   IN       kind,
      void*             IN       ppar);


#endif
