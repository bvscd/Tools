#ifndef _RIA_HTTP
#define _RIA_HTTP


#include "ria_core.h"


/*
 * Maintain different HTTP protocol operation principles
 *
 */
#ifdef ANDROID
#define RIA_HTTP_ATOMIC
#endif 
#ifdef RIA_USE_ASYNC_CALLS
#ifdef RIA_HTTP_ATOMIC
#error HTTP configuration mismatch
#endif
#endif 


/*
 * HTTP string resources
 *
 */
enum {
   ria_str_user_agent,
   ria_str_site_address,
   ria_str_last_url,
   ria_str_max
};

/*
 * HTTP cookie info
 *
 */
typedef struct ria_cookie_s {
   const char*   pname;
   usize         cname;
   const char*   pvalue;
   usize         cvalue;
   const char*   pdomain;
   usize         cdomain;
   const char*   ppath;
   usize         cpath;
   time_t        expire;
} ria_cookie_t; 

/*
 * HTTP async states
 *
 */
#ifdef USE_RIA_ASYNC_CALLS   
typedef enum ria_http_async_state_e {
   ria_http_async_connect = 1,
   ria_http_async_receive = 2,
   ria_http_async_rcvmore = 3
} ria_http_async_state_t;  
#endif   

/*
 * HTML normalizer
 *
 */
typedef struct ria_html_s {
   umask     state;
   unumber    detector;
   unumber   tag_depth;
   char      quote;
} ria_html_t; 

/*
 * HTTP context
 *
 */
typedef struct ria_http_s {
   buf_t           pool;
   buf_t           cookie;
   buf_t           hdrs;
   buf_t           resp;
   buf_t           temp;
   void*           proto;
   void*           connect;
   void*           request;
   unumber         redirects;
   unumber         http_code;
   usize           strs[ria_str_max];
   ria_html_t      html;
   ria_config_t*   config;
#ifdef RIA_HTTP_ATOMIC
   const char*     tofile;
   const char*     todump;
   bool            normalize;
   bool            reentry;
   void*           pdata;
   usize           cdata;
#endif   
#ifdef USE_RIA_ASYNC_CALLS   
   ria_async_t     async;
#endif   
   heap_ctx_t*     mem;
   bool            hdrs_recv;
   umask           cleanup;
} ria_http_t;

/*@@ria_http_create
 *
 * Creates HTTP context
 *
 * Parameters:     ctx            HTTP context
 *                 config         configuration pointer
 *                 mem            memory context
 *
 * Return:         true           if successful
 *                 false          if failed
 *
 */
bool 
   ria_http_create(     
      ria_http_t*     IN OUT   ctx,
      ria_config_t*   IN       config,
      heap_ctx_t*     IN OUT   mem);

/*@@ria_http_destroy
 *
 * Destroys HTTP context
 *
 * Parameters:     ctx            HTTP context
 *
 * Return:         true           if successful
 *                 false          if failed
 *
 */
bool 
   ria_http_destroy(     
      ria_http_t*   IN OUT   ctx);

/*@@ria_http_get_string
 *
 * Fetches HTTP string value
 *
 * Parameters:     str            string pointer
 *                 id             string id
 *                 ctx            HTTP context
 *
 * Return:         true           if successful
 *                 false          if failed
 *
 */
bool 
   ria_http_get_string(     
      char const**        OUT   str,
      unumber             IN    id,
      const ria_http_t*   IN    ctx);

/*@@ria_http_set_string
 *
 * Sets HTTP string value
 *
 * Parameters:     pstr           string pointer
 *                 cstr           string size
 *                 id             string id
 *                 ctx            HTTP context
 *
 * Return:         true           if successful
 *                 false          if failed
 *
 */
bool 
   ria_http_set_string(     
      const char*   IN    pstr,
      usize         IN    cstr,
      unumber       IN    id,
      ria_http_t*   IN    ctx);

/*@@ria_http_init
 *
 * Initializes HTTP protocol stack
 *
 * Parameters:     ctx            HTTP context
 *
 * Return:         true           if successful
 *                 false          if failed
 *
 */
bool 
   ria_http_init(     
      ria_http_t*   IN    ctx);

/*@@ria_http_connect
 *
 * Connects to site
 *
 * Parameters:     pending        pending connect flag
 *                 psite          site address
 *                 csite          site address size
 *                 ctx            HTTP context
 *
 * Return:         true           if successful
 *                 false          if failed
 *
 */
bool 
   ria_http_connect(     
      bool*         OUT   pending,
      const char*   IN    psite,
      usize         IN    csite,
      ria_http_t*   IN    ctx);

/*@@ria_http_send
 *
 * Sends GET/POST request
 *
 * Parameters:     pending        pending send flag
 *                 purl           URL to send to
 *                 curl           URL size
 *                 pval           value to POST (NULL for GET)
 *                 cval           value size (0 for GET)
 *                 ctx            HTTP context
 *
 * Return:         true           if successful
 *                 false          if failed
 *
 */
bool 
   ria_http_send(     
      bool*         OUT   pending, 
      const char*   IN    purl,
      usize         IN    curl,
      const char*   IN    pval,
      usize         IN    cval,
      ria_http_t*   IN    ctx);

/*@@ria_http_receive
 *
 * Receives response
 *
 * Parameters:     pending        pending receive flag
 *                 tofile         destination filename
 *                 todump         destination dumpfile
 *                 normalize      true if make text normalization
 *                 reentry        re-entry after pending receive
 *                 ctx            HTTP context
 *
 * Return:         true           if successful
 *                 false          if failed
 *
 */
bool 
   ria_http_receive(     
      bool*         OUT   pending,
      const char*   IN    tofile,
      const char*   IN    todump,
      bool          IN    normalize,
      bool          IN    reentry,
      ria_http_t*   IN    ctx);

/*@@ria_http_get_header
 *
 * Fetches HTTP header value
 *
 * Parameters:     dst            destination buffer
 *                 phdr           header name 
 *                 chdr           header name size
 *                 ctx            HTTP context
 *
 * Return:         true           if successful
 *                 false          if failed
 *
 */
bool 
   ria_http_get_header(     
      buf_t*              IN OUT   dst,
      const char*         OUT      phdr,
      usize               IN       chdr,
      const ria_http_t*   IN       ctx);

/*@@ria_http_set_header
 *
 * Sets HTTP header value
 *
 * Parameters:     phdr           header name 
 *                 chdr           header name size
 *                 pval           value 
 *                 cval           value size
 *                 ctx            HTTP context
 *
 * Return:         true           if successful
 *                 false          if failed
 *
 */
bool 
   ria_http_set_header(     
      const char*   OUT      phdr,
      usize         IN       chdr,
      const char*   OUT      pval,
      usize         IN       cval,
      ria_http_t*   IN       ctx);

/*@@ria_http_parse_cookie
 *
 * Parses HTTP cookie record
 *
 * Parameters:     ok             success flag
 *                 dst            cookie info
 *                 prec           cookie record
 *                 crec           cookie record size
 *
 * Return:         true           if successful
 *                 false          if failed
 *
 */
bool 
   ria_http_parse_cookie(     
      bool*           OUT      ok,
      ria_cookie_t*   IN OUT   dst,
      const char*     IN       prec,
      usize           IN       crec);

/*@@ria_http_get_cookie
 *
 * Gets HTTP cookie value
 *
 * Parameters:     dst            cookie info
 *                 src            cookie buffer
 *                 pname          cookie name
 *                 cname          cookie name size, bytes
 *
 * Return:         true           if successful
 *                 false          if failed
 *
 */
bool 
   ria_http_get_cookie(    
      ria_cookie_t*   IN OUT   dst,
      const buf_t*    IN       src,
      const char*     IN       pname,
      usize           IN       cname); 

/*@@ria_http_pack_cookies
 *
 * Packs HTTP cookies into single header format
 *
 * Parameters:     src            cookie buffer
 *
 * Return:         true           if successful
 *                 false          if failed
 *
 */
bool 
   ria_http_pack_cookies(    
      buf_t*   IN OUT   src);
      
/*@@ria_http_set_cookie
 *
 * Sets HTTP cookie value
 *
 * Parameters:     dst            cookie buffer
 *                 src            cookie info
 *
 * Return:         true           if successful
 *                 false          if failed
 *
 */
bool 
   ria_http_set_cookie(    
      buf_t*                IN OUT   dst, 
      const ria_cookie_t*   IN       src);


#endif
