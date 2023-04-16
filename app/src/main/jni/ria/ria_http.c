#include "ria_exec.h"
#include "ria_papi.h"
#include "ria_func.h"


/******************************************************************************
 *  Utils
 */

/*****************************************************************************/
static int
   ria_parse_number(     
      const char**   IN OUT   p,
      usize*         IN OUT   c,
      usize          IN       len,
      char           IN       sep)
/*
 * Parses numeric field
 *
 */
{ 

   usize i, j;

   assert(p != NULL);
   assert(c != NULL);
   
   if (*c < len+1)
      return -1;
   j = 0;
   for (i=0; i<len; i++) {
      if (((*p)[i] < '0') || ((*p)[i] > '9'))
         return -1;
      j = j*10 + (*p)[i] - '0';
   }
   if ((*p)[i] != sep)
      return -1;
   *p += len + 1;
   *c -= len + 1;
   
   return (int)j;                
   
}   

/*****************************************************************************/
static void
   ria_http_parse_time(     
      bool*           OUT      ok,
      time_t*         OUT      dst,
      const char*     IN       ptime,
      usize           IN       ctime)
/*
 * Parses time value dd-mmm-yyyy hh:mm:ss GMT
 *
 */
{

   struct tm t;
   time_t t2, t3;
   unumber i;
   
   /*
    * Day values
    *
    */
   static const char* _days[] =
   {
      "sun", "mon", "tue", "wed", "thu", "fri", "sat"
   }; 

   /*
    * Month values
    *
    */
   static const char* _mons[] =
   {
      "jan", "feb", "mar", "apr", "may", "jun",
      "jul", "aug", "sep", "oct", "nov", "dec"
   }; 

   /*
    * Time modifier
    *
    */
   static const char _gmt[] = "gmt"; 

   assert(ok    != NULL);
   assert(dst   != NULL);
   assert(ptime != NULL);
   
   *ok = false;
   MemSet(&t, 0x00, sizeof(t));
   
   /*
    * Day of week
    *
    */
   if (ctime < 5) 
      return;
   for (i=0; i<sizeof(_days)/sizeof(_days[0]); i++)
      if (!StrNICmp(_days[i], ptime, 3))
         break;
   if (i == sizeof(_days)/sizeof(_days[0]))
      return;
   t.tm_wday = i;
   if ((ptime[3] != ',') || (ptime[4] != ' '))
      return;
   ptime += 5;
   ctime -= 5;                

   /*
    * Day
    *
    */
   t.tm_mday = ria_parse_number(&ptime, &ctime, 2, '-');
   if (t.tm_mday == -1)
      return;
      
   /*
    * Month
    *
    */
   if (ctime < 4) 
      return;
   for (i=0; i<sizeof(_mons)/sizeof(_mons[0]); i++)
      if (!StrNICmp(_mons[i], ptime, 3))
         break;
   if (i == sizeof(_mons)/sizeof(_mons[0]))
      return;
   t.tm_mon = i;
   if (ptime[3] != '-')
      return;
   ptime += 4;
   ctime -= 4;                
            
   /*
    * Year
    *
    */
   t.tm_year = ria_parse_number(&ptime, &ctime, 4, ' ');
   if (t.tm_year == -1)
      return;
   t.tm_year -= 1900;   

   /*
    * Time
    *
    */
   t.tm_hour = ria_parse_number(&ptime, &ctime, 2, ':');
   if (t.tm_hour == -1)
      return;
   t.tm_min = ria_parse_number(&ptime, &ctime, 2, ':');
   if (t.tm_min == -1)
      return;
   t.tm_sec = ria_parse_number(&ptime, &ctime, 2, ' ');
   if (t.tm_sec == -1)
      return;
      
   /*
    * Check modifier
    *
    */
   if (ctime != sizeof(_gmt)-1)
      return;
   if (StrNICmp(ptime, _gmt, ctime))
      return;
      
   /*
    * Convert
    *
    */
#ifdef WISE12
   *ok = false;
#else
   *dst = MkTime(&t);
   if (*dst == (time_t)-1)
      return;   
   t2 = MkTime(localtime(dst));
   t3 = MkTime(gmtime(dst));
   *dst += t2 - t3;
   *ok = true;    
#endif
      
}

/*****************************************************************************/
static bool 
   ria_http_find_cookie_pos(    
      const byte**   OUT   pos,
      usize*         OUT   len,
      const buf_t*   IN    buf, 
      const char*    IN    pname,
      usize          IN    cname)
/*
 * Sets cookie value
 *
 */
{

   const byte* p;
   usize c, l;

   assert(pos   != NULL);
   assert(buf   != NULL);
   assert(pname != NULL);
   
   *pos = NULL;
   *len = 0;
   
   /*
    * Find cookie start position
    * LL [<cookie>=<value>; EEEE]
    *
    */
   p = buf_get_ptr_bytes(buf);
   c = buf_get_length(buf); 
   for (l=0; c!=0;) {

      /*
       * Get chunk
       *
       */
      if (c < 2)
         ERR_SET(err_internal);  
      l = (p[0] << 1) | p[1];
      if (c < l+2)
         ERR_SET(err_internal);  
      if (l >= cname) 
         if (!StrNICmp((char*)(p+2), pname, cname))
            if (p[cname+2] == '=') {
               /*
                * Cookie found
                *
                */
               *len = l + 2;  
               *pos = p;
               break;
            }   
      
      /*
       * Next chunk
       *
       */
      p += l + 2;
      c -= l + 2;
      l = 0;
      
   }   
      
   return true;   

}
 

/******************************************************************************
 *  HTTP 
 */

enum {
   cf_ria_http_pool   = 0x01,
   cf_ria_http_resp   = 0x02,
   cf_ria_http_hdrs   = 0x04,
   cf_ria_http_temp   = 0x08,
   cf_ria_http_cookie = 0x10,
#ifdef USE_RIA_ASYNC_CALLS   
   cf_ria_http_async  = 0x20
#endif   
};

/*****************************************************************************/
bool 
   ria_http_create(     
      ria_http_t*     IN OUT   ctx,
      ria_config_t*   IN       config,
      heap_ctx_t*     IN OUT   mem)
/*
 * Creates HTTP context
 *
 */
{

   unumber i;

   assert(ctx != NULL);
   assert(mem != NULL);

   MemSet(ctx, 0x00, sizeof(*ctx));
   ctx->mem       = mem;
   ctx->connect   = NULL;
   ctx->proto     = NULL;
   ctx->request   = NULL;
   ctx->hdrs_recv = false;
   ctx->config    = config;
   for (i=0; i<ria_str_max; i++)
      ctx->strs[i] = (usize)-1;

   if (!buf_create(1, 0, 0, &ctx->pool, ctx->mem))
      goto failed;
   else
      ctx->cleanup |= cf_ria_http_pool;
   if (!buf_create(1, 0, 0, &ctx->resp, ctx->mem))
      goto failed;
   else
      ctx->cleanup |= cf_ria_http_resp;
   if (!buf_create(1, 0, 0, &ctx->hdrs, ctx->mem))
      goto failed;
   else
      ctx->cleanup |= cf_ria_http_hdrs;
   if (!buf_create(1, 0, 0, &ctx->temp, ctx->mem))
      goto failed;
   else
      ctx->cleanup |= cf_ria_http_temp;
   if (!buf_create(1, 0, 0, &ctx->cookie, ctx->mem))
      goto failed;
   else
      ctx->cleanup |= cf_ria_http_cookie;
#ifdef USE_RIA_ASYNC_CALLS
   ria_async_create(&ctx->async);
   ctx->cleanup |= cf_ria_http_async;
#endif   

   return true;

failed:
   ria_http_destroy(ctx);
   return false;


}

/*****************************************************************************/
bool 
   ria_http_destroy(     
      ria_http_t*   IN OUT   ctx)
/*
 * Destroys execution context
 *
 */
{

   bool ret = true;

   assert(ctx != NULL);

   if (ctx->request != NULL) {
      ret = ria_papi_http_close_request(ctx) && ret;
      ctx->request = NULL;
   }      
   if (ctx->connect != NULL) {
      ret = ria_papi_http_disconnect(ctx) && ret;
      ctx->connect = NULL;
   }      
   
   if (ctx->cleanup & cf_ria_http_pool)
      ret = buf_destroy(&ctx->pool) && ret;
   if (ctx->cleanup & cf_ria_http_resp)
      ret = buf_destroy(&ctx->resp) && ret;
   if (ctx->cleanup & cf_ria_http_hdrs)
      ret = buf_destroy(&ctx->hdrs) && ret;
   if (ctx->cleanup & cf_ria_http_temp)
      ret = buf_destroy(&ctx->temp) && ret;
   if (ctx->cleanup & cf_ria_http_cookie)
      ret = buf_destroy(&ctx->cookie) && ret;
#ifdef USE_RIA_ASYNC_CALLS
   if (ctx->cleanup & cf_ria_http_async)
      ria_async_destroy(&ctx->async);
#endif   

   ctx->cleanup = 0;
   return ret;

}

/*****************************************************************************/
bool 
   ria_http_get_string(     
      char const**        OUT   str,
      unumber             IN    id,
      const ria_http_t*   IN    ctx)
/*
 * Fetches HTTP string value
 *
 */
{

   usize c;
   byte* p;

   assert(str != NULL);
   assert(ctx != NULL);

   /*
    * Sanity checks
    *
    */
   if (id >= ria_str_max)
      ERR_SET(err_bad_param);

   /*
    * Get string
    *
    */
   *str = NULL;
   c = buf_get_length(&ctx->pool);
   p = buf_get_ptr_bytes(&ctx->pool);
   if (ctx->strs[id] >= c)
      /* Not set yet */
      return true;
   *str = (char*)(p + ctx->strs[id]);
   return true;

}

/*****************************************************************************/
bool 
   ria_http_set_string(     
      const char*   IN    pstr,
      usize         IN    cstr,
      unumber       IN    id,
      ria_http_t*   IN    ctx)
/*
 * Sets HTTP string value
 *
 */
{

   usize c1;
   usize c2;
   unumber i;
   ioffset shift;
   byte* p1;
   byte* p2;
   byte* p3;

   assert(pstr != NULL);
   assert(ctx  != NULL);

   /*
    * Get current string value
    *
    */
   if (!ria_http_get_string((char const**)&p2, id, ctx))
      return false;

   /*
    * Calculate shift
    *
    */
   c1 = (p2 == NULL) ? 0 : StrLen((char*)p2) + 1;
   c2 = cstr + 1;
   shift = c2 - c1;

   /*
    * Allocate for shift
    *
    */
   c1 = buf_get_length(&ctx->pool);
   if (shift > 0) {
      if (!buf_expand(c1+shift, &ctx->pool))
         return false;
   }

   /*
    * Find origin
    *
    */
   p1 = buf_get_ptr_bytes(&ctx->pool);
   if (!ria_http_get_string((char const**)&p2, id, ctx))
      return false;
   if (p2 == NULL) {
      for (i=id; i>0; i--)
         if (ctx->strs[i-1] < c1) {
            p2  = p1 + ctx->strs[i-1];
            p2 += StrLen((char*)p2) + 1;
            break;
         }
      if (i == 0)
         p2 = p1;
   }

   /*
    * Find next pos
    *
    */
   p3 = NULL;
   for (i=id+1; i<ria_str_max; i++)
      if (ctx->strs[i] < c1) {
        p3  = p1 + ctx->strs[i];
        break;
      }
   
   /*
    * Shift if needed
    *
    */
   if (shift != 0) {
      if (!buf_set_length(c1+shift, &ctx->pool))
         return false;
      if (p3 != NULL)
         MemMove(p3+shift, p3, c1-(p3-p1));   
      for (i=id+1; i<ria_str_max; i++)
         if (ctx->strs[i] < c1)
            ctx->strs[i] += shift;
   }

   /* 
    * Set string
    *
    */
   MemMove(p2, pstr, cstr);
   p2[cstr] = 0x00;
   ctx->strs[id] = p2 - p1;
   return true;

}

/*****************************************************************************/
bool 
   ria_http_init(     
      ria_http_t*   IN    ctx)
/*
 * Initializes HTTP protocol stack
 *
 */
{

   static const char _agent[] = "iPhone/001/861 (compatible; MSIE)";//testagent";

   usize i;
   byte* p;
   const char* agent;

   assert(ctx != NULL);

   /*
    * Check whether already initialized
    *
    */
   if (ctx->proto != NULL)
      return true;

   /*
    * Check agent name 
    *
    */
   i = buf_get_length(&ctx->pool);
   p = buf_get_ptr_bytes(&ctx->pool);
   if (!ria_http_get_string(&agent, ria_str_user_agent, ctx))
      return false;
   if (agent == NULL)
      agent = _agent;

   /* 
    * Call platform 
    *
    */
   return ria_papi_http_init(agent, ctx);

}

/*****************************************************************************/
bool 
   ria_http_connect(     
      bool*         OUT   pending,
      const char*   IN    psite,
      usize         IN    csite,
      ria_http_t*   IN    ctx)
/*
 * Connects to site
 *
 */
{

   const char* p = NULL;

   assert(psite   != NULL);
   assert(ctx     != NULL);
   assert(pending != NULL);

   /*
    * Check for connection reuse
    *
    */
   if (ctx->connect != NULL) {
      if (!ria_http_get_string(&p, ria_str_site_address, ctx))
         return false;
      if (p != NULL)    
         if (StrLen(p) == csite) 
            if (StrNICmp(p, psite, csite) == 0) {
               /*
                * If already connected to requested site, exit
                *
                */
               *pending = false; 
               return true;
            }   
      if (!ria_papi_http_disconnect(ctx))
         return false;
   }

   /*
    * Connect and save
    *
    */
   if (!ria_http_set_string(psite, csite, ria_str_site_address, ctx))
      return false;
   if (!ria_http_get_string(&p, ria_str_site_address, ctx))
      return false;
#ifdef RIA_ASYNC_CONNECT   
   ria_async_set_param(&ctx->async, &ctx->connect);
#endif   
   return ria_papi_http_connect(pending, p, ctx);
 
}

/*****************************************************************************/
bool 
   ria_http_send(     
      bool*         OUT   pending,
      const char*   IN    purl,
      usize         IN    curl,
      const char*   IN    pval,
      usize         IN    cval,
      ria_http_t*   IN    ctx)
/*
 * Sends GET/POST request
 *
 */
{

   assert(purl    != NULL);
   assert(ctx     != NULL);
   assert(pending != NULL);

   /*
    * Save last URL
    *
    */
   if (!ria_http_set_string(purl, curl, ria_str_last_url, ctx))
      return false;
   if (!ria_http_get_string(&purl, ria_str_last_url, ctx))
      return false;

   /*
    * Redirect to platform
    *
    */
#ifdef RIA_ASYNC_CONNECT   
   ria_async_set_param(&ctx->async, &ctx->request);
#endif   
   return ria_papi_http_send(pending, purl, pval, cval, ctx);

}

/*****************************************************************************/
bool 
   ria_http_receive(     
      bool*         OUT   pending,
      const char*   IN    tofile,
      const char*   IN    todump,
      bool          IN    normalize,
      bool          IN    reentry,
      ria_http_t*   IN    ctx)
/*
 * Receives response
 *
 */
{

   static const char _script[] = "script>";

   usize c, i, j; 
   byte* p;
   byte* q;
   void* file = NULL;
   void* dump = NULL;
   bool  fileout = (tofile == NULL) ? false : true;
   bool  dumpout = (todump == NULL) ? false : true;
   bool  ret;

   enum {
      state_in_tag              = 0x0001,
      state_space               = 0x0002,
      state_lf                  = 0x0004,
      state_cr                  = 0x0008,
      state_detect_script_start = 0x0010,
      state_detect_script_stop  = 0x0020,
      state_tag_begin           = 0x0040,
      state_script              = 0x0080,
      state_pre_script          = 0x0100,
      state_quoted              = 0x0200,
      state_possibly_script_end = 0x0400,
      state_after_tag           = 0x0800
   };
   
   enum {
      cleanup_file = 0x01,
      cleanup_dump = 0x02
   } cleanup = 0x00;

   assert(pending != NULL);
   assert(ctx     != NULL);

   /*
    * Cleanup for first entry
    *
    */
   if (!reentry) { 
      if (!buf_destroy(&ctx->resp))
         return false;
      if (!buf_destroy(&ctx->hdrs))
         return false;
      ctx->http_code      = 0;
      ctx->html.state     = 0;
      ctx->html.detector  = 0;
      ctx->html.tag_depth = 0;
      ctx->html.quote     = 0;
   }         

   /*
    * Check status code
    *
    */
   if (ctx->http_code == 0) {
      if (!ria_papi_http_get_status(&ctx->http_code, ctx))
         return false;
   }      
   
   ret = false;

   /*
    * Setup output
    *
    */
#define CHUNK 1024
   if (fileout) {
      i = buf_get_length(&ctx->config->tempdir);
      j = StrLen(tofile);
      if (!buf_expand(i+j+1, &ctx->config->tempdir))
         goto exit;
      p = buf_get_ptr_bytes(&ctx->config->tempdir) + i;   
      MemCpy(p, tofile, j+1);   
      if (reentry) {
         if (!ria_papi_fopen(&file, (char*)p-i, "a+b"))
            goto exit;
      }
      else {
         if (!ria_papi_fopen(&file, (char*)p-i, "wb"))
            goto exit;
      }      
      cleanup |= cleanup_file;   
      c = 0;   
   }
   else {
      if (reentry) {
         c = buf_get_length(&ctx->resp);
         if (normalize) {
            /*
             * Remove previously posted zero terminator
             *
             */
            if (c == 0)
               ERR_SET(err_internal);
            if (buf_get_ptr_bytes(&ctx->resp)[c-1] != 0x00)   
               ERR_SET(err_internal);
            c--;   
         }
      }         
      else {   
         if (!buf_expand(1, &ctx->resp))
            goto exit;
         if (!buf_set_length(1, &ctx->resp))
            goto exit;
         buf_get_ptr_bytes(&ctx->resp)[0] = (byte)ria_string;
         c = 1;   
      }  
   }          
   if (dumpout) {
      i = buf_get_length(&ctx->config->tempdir);
      j = StrLen(todump);
      if (!buf_expand(i+j+1, &ctx->config->tempdir))
         goto exit;
      p = buf_get_ptr_bytes(&ctx->config->tempdir) + i;   
      MemCpy(p, todump, j+1);   
      if (!ria_papi_fopen(&dump, (char*)p-i, "a+b"))
         goto exit;
      else
         cleanup |= cleanup_dump;
   }         
   
   for (;;) {
   
      /*
       * Read response
       *
       */
      if (!buf_expand(c+CHUNK, &ctx->resp))
         goto exit;
      p = buf_get_ptr_bytes(&ctx->resp) + c;
      if ((ctx->html.state & state_in_tag) && 
          (ctx->html.state & state_space)) {
         if (!ria_papi_http_receive(pending, &i, p+1, CHUNK-1, ctx))
            goto exit;
         if (i == 0)
            break;
         p[0] = ' ';
         i++;   
         ctx->html.state &= ~state_space;
      }
      else {  
         if (!ria_papi_http_receive(pending, &i, p, CHUNK, ctx))
            goto exit;
         if (i == 0)
            break;
      }      

      /*
       * Normalize data if requested
       *
       */   
      if (normalize) {

         for (j=0, q=p; j<i; j++) {

            /*
             * Check quoted text
             *
             */
            if (ctx->html.state & state_quoted) {
               if (p[j] == ctx->html.quote)
                  ctx->html.state &= ~state_quoted;
               *(q++) = p[j];   
               continue;
            }
         
            /*
             * Eliminate whitespaces
             *
             */
            switch (p[j]) {
            case '\n':
            case '\r':
               if (!(ctx->html.state & state_after_tag)) 
                  p[j] = ' ';
               else {
                  if (p[j] == '\n')                  
                     ctx->html.state &= ~state_after_tag;
                  if (ctx->html.state & state_lf)
                     continue;
                  break;
               }                  
            case ' ':
               ctx->html.state &= ~state_after_tag;
               if (ctx->html.state & 
                  (state_space|state_cr|state_lf|state_tag_begin))
                  continue;
               break;   
            case '\t':
               ctx->html.state &= ~state_after_tag;
               continue;
            default:
               ctx->html.state &= ~state_after_tag;
            }
               
            /*
             * State control
             *
             */
            switch (p[j]) {
            case ' ':
               ctx->html.state |= state_space;
               ctx->html.state &= ~(state_lf|state_cr);
               break;   
            case '\r':
               ctx->html.state |= state_cr;
               ctx->html.state &= ~(state_space);
               break;   
            case '\n':
               ctx->html.state |= state_lf;
               ctx->html.state &= ~(state_space|state_cr);
               break;   
            case '<':
               if (ctx->html.state & state_in_tag)
                  ctx->html.tag_depth++;
               if (ctx->html.state & state_script)
                  ctx->html.state |= state_possibly_script_end;
               ctx->html.state |= state_in_tag|state_tag_begin;
               ctx->html.state &= ~(state_space|state_cr|state_lf);
               break;               
            case '/':
               if (!(ctx->html.state & state_in_tag))
                  break;
               if (ctx->html.state & state_possibly_script_end)
                  ctx->html.state |= state_detect_script_stop;   
            case '>':
               if (ctx->html.state & state_space)
                  --q;
               ctx->html.state &= ~(state_space|state_tag_begin);
               if (ctx->html.state & 
                  (state_detect_script_start|state_detect_script_stop))
                  break;  
               if (!(ctx->html.state & (state_pre_script|state_in_tag)))
                  if (ctx->html.state & state_script)                   
                     break;
               if (p[j] == '>') {
                  if (ctx->html.tag_depth > 0) {
                     ctx->html.tag_depth--;
                     break;
                  }
                  ctx->html.state &= ~(state_in_tag|state_pre_script);
                  ctx->html.state |= state_after_tag;
               }   
               break;       
            case '\'':
               if (!(ctx->html.state & state_script))
                  break;
            case '"':
               ctx->html.state |= state_quoted;
               ctx->html.state &= 
                  ~(state_space|state_cr|state_lf|state_tag_begin);        
               ctx->html.quote = p[j];
               break;
            }
            
            /*
             * Script detector
             *
             */
            switch (p[j]) {
            case '\r':
            case '\n':
            case '<':
            case '/':
            case '"':
               break;   
            case ' ':
            case '>':
               if (!(ctx->html.state & 
                    (state_detect_script_start|state_detect_script_stop)))
                  break;  
               if (_script[ctx->html.detector] == '>') {
                  if (ctx->html.state & state_detect_script_start) {
                     ctx->html.state &= ~state_detect_script_start;
                     ctx->html.state |= state_script;                     
                  }
                  else {
                     ctx->html.state &= ~state_detect_script_stop;
                     ctx->html.state &= ~state_script;                     
                  }
               }
               else
                  ctx->html.state &= ~
                     (state_detect_script_start|state_detect_script_stop);
               ctx->html.detector = 0;
               if (p[j] == '>') {
                  if (ctx->html.tag_depth > 0) {
                     ctx->html.tag_depth--;
                     break;
                  }
                  ctx->html.state &= ~state_in_tag;
                  ctx->html.state |= state_after_tag;
               }   
               else
                  ctx->html.state |= state_pre_script;   
               break;
            case '\'':
               if (ctx->html.state & state_script)
                  break;
            default:   
               if (!(ctx->html.state & state_script))
                  if (ctx->html.state & state_in_tag) 
                     if (ctx->html.state & state_tag_begin)
                        ctx->html.state |= state_detect_script_start;
               if ((ctx->html.state & 
                   (state_detect_script_start|state_detect_script_stop))) {
                  char ch = ((p[j] >= 'A') && (p[j] <= 'Z')) ? 
                     p[j]-'A'+'a' : p[j];
                  if (ch != _script[ctx->html.detector]) {
                     ctx->html.detector = 0;
                     ctx->html.state &= ~state_detect_script_start;
                     ctx->html.state &= ~state_detect_script_stop;
                     if (ctx->html.state & state_possibly_script_end) 
                        ctx->html.state &= 
                           ~(state_possibly_script_end|state_in_tag);
                  }
                  else 
                     ctx->html.detector++;
               }      
               else 
                  if (ctx->html.state & state_possibly_script_end) 
                     ctx->html.state &= 
                        ~(state_possibly_script_end|state_in_tag);
               ctx->html.state &= 
                  ~(state_space|state_cr|state_lf|state_tag_begin);        
            } 
            
            *(q++) = p[j];
            
         }
         
         i = q - p;
         
      } 
      if (ctx->html.state & state_in_tag)
         if (ctx->html.state & state_space)
            i--;
      
      /*
       * Dump
       *
       */
      if (dumpout) 
         if (!ria_papi_fwrite(p, i, dump))
            goto exit;
            
      /*
       * Remove all linebreaks
       *
       */
      for (j=0, q=p; j<i; j++) {
         switch (p[j]) {
         case '\r':
         case '\n':
            continue;
         }
         *(q++) = p[j];
      }
      i = q - p;
       
      /*
       * Write to output
       *
       */
      if (fileout) {
         if (!ria_papi_fwrite(p, i, file))
            goto exit;
      }
      else {
         c += i;   
         if (!buf_set_length(c, &ctx->resp))
            goto exit;
      }            
      
      /*
       * Check pending state
       *
       */
      if (*pending)
         break;
         
   }
   
   /*
    * Finalize output
    *
    */
   if (fileout) {
      cleanup &= ~cleanup_file;
      if (!ria_papi_fclose(file))
         goto exit;
      if (!*pending) {
         /*
          * Finalize response buffer
          *
          */   
         if (!ria_prealloc_datatype_in_buf(&p, ria_string, 1, &ctx->resp))
            goto exit;
         *p = 0x00;   
      }         
   }
   else
      if (normalize) {
         c++;
         if (!buf_expand(c, &ctx->resp))
            goto exit;
         if (!buf_set_length(c, &ctx->resp))
            goto exit;
         buf_get_ptr_bytes(&ctx->resp)[c-1] = 0x00;   
      }         
   if (dumpout) {
      cleanup &= ~cleanup_dump;
      if (!ria_papi_fclose(dump))
         goto exit;
   }         
#undef CHUNK

   ret = true;
exit:   
   if (cleanup & cleanup_file)
      ret = ria_papi_fclose(file) && ret;
   if (cleanup & cleanup_dump)
      ret = ria_papi_fclose(dump) && ret;
   return ret;

}

/*****************************************************************************/
bool 
   ria_http_get_header(     
      buf_t*              IN OUT   dst,
      const char*         OUT      phdr,
      usize               IN       chdr,
      const ria_http_t*   IN       ctx)
/*
 * Fetches HTTP header value
 *
 */
{

   byte* p;
   byte* q;
   usize c;
   usize l = StrLen(_multiheader);
   
   /*
    * Prepare destination
    *
    */
   buf_set_empty(dst);
   
   /*
    * Scan existing headers
    *
    */
   c = buf_get_length(&ctx->hdrs);
   p = buf_get_ptr_bytes(&ctx->hdrs);
   for (; c>chdr; p=q+2, c-=2) {
   
      bool found;
   
      /*
       * Check for match
       *
       */
      found = (!StrNICmp((const char*)p, phdr, chdr)) ? true : false;
      if (found) {
         if (p[chdr] == ':') {
            for (p++, c--; c>0; p++, c--)
               if (p[chdr] != ' ')
                  break;
         }   
         else
            found = false;
      }            
         
      /*
       * Scan header boundary
       *
       */
      for (p+=chdr, q=p, c-=chdr; c>0; q++, c--)
         if (q[0] == '\r')
            break;
      if (c == 0)
         ERR_SET(err_internal);
      if (q[1] != '\n')
         ERR_SET(err_internal);
      
      /*
       * Exit or continue
       *
       */
      if (found) {
         if (buf_get_length(dst) != 0) {
            if (!buf_append(_multiheader, l, dst))
               return false;
         }      
         if (!buf_append(p, q-p, dst))
            return false;
      }      

   }

   /*
    * Not found 
    *
    */
   return buf_fill(0x00, buf_get_length(dst), 1, dst);

}

/*****************************************************************************/
bool 
   ria_http_set_header(     
      const char*   OUT      phdr,
      usize         IN       chdr,
      const char*   OUT      pval,
      usize         IN       cval,
      ria_http_t*   IN       ctx)
/*
 * Sets HTTP header value
 *
 */
{

   usize c;
   byte* p;

   assert(phdr != NULL);
   assert(pval != NULL);
   assert(ctx  != NULL);

   /*
    * Cleanup received headers
    *
    */
   if (ctx->hdrs_recv) {
      if (!buf_destroy(&ctx->hdrs))
         return false;
      ctx->hdrs_recv = false;   
   } 
   
   /*
    * Append header
    *
    */
   c = buf_get_length(&ctx->hdrs);
   if (!buf_expand(c+cval+chdr+4, &ctx->hdrs))
      return false;
   p = buf_get_ptr_bytes(&ctx->hdrs) + c;
   MemCpy(p, phdr, chdr), p += chdr;
   *(p++) = ':';   
   *(p++) = ' ';   
   MemCpy(p, pval, cval), p += cval;
   *(p++) = '\r';   
   *(p++) = '\n';   
   return buf_set_length(c+cval+chdr+4, &ctx->hdrs);   

}

/*****************************************************************************/
bool 
   ria_http_parse_cookie(     
      bool*           OUT      ok,
      ria_cookie_t*   IN OUT   dst,
      const char*     IN       prec,
      usize           IN       crec)
/*
 * Parses HTTP cookie record
 *
 */
{

   /*
    * Cookie record parameters
    *
    */
    
   typedef enum par_type_e {
      par_expires,
      par_path,
      par_domain,
      par_cookie
   } par_type_t; 
    
   typedef struct par_s {
      const char* name;
      usize       len;
      par_type_t  type;
   } par_t;
   
#define PAR(_p) { #_p, sizeof(#_p)-1, par_##_p }
   static const par_t _pars[] = 
   {
      PAR(expires),
      PAR(path),
      PAR(domain)
   };
#undef PAR   

   static const char _http_only[] = "httponly";
   
   const char* p;
   const char* pend;
   usize i;
   par_type_t type;

   assert(ok   != NULL);
   assert(dst  != NULL);
   assert(prec != NULL);
   
   /*
    * Initialize cookie info
    *
    */
   *ok = false;  
   MemSet(dst, 0x00, sizeof(*dst));   
   dst->pname = NULL;
   dst->ppath = NULL;
   dst->pdomain = NULL;
    
   /*
    * Format is:
    * <par>=<val>; space...<par>=<val>
    *
    */
    
   /*
    * Parse sequence
    *
    */
   for (pend=prec+crec; crec>0; ) {
      
      /*
       * Find parameter name
       *
       */
      for (p=prec; p<pend; p++)
         if (*p == '=')
            break;
      if (p == pend) {
         /*
          * Check for known flags
          *
          */
         if (p-prec == sizeof(_http_only)-1)
            if (!StrNICmp(prec, _http_only, p-prec))
               break;
         return true;
      }                  

      /*
       * Check parameter name
       *
       */
      for (i=0; i<sizeof(_pars)/sizeof(_pars[0]); i++) {
         if (_pars[i].len != (unsigned)(p-prec))
            continue;     
         if (!StrNICmp(prec, _pars[i].name, _pars[i].len))
            break;
      }  
      if (i == sizeof(_pars)/sizeof(_pars[0])) {
         /*
          * Store cookie name if not set yet
          *
          */
         if (dst->pname != NULL)  
            return true;
         dst->pname = prec;
         dst->cname = p - prec;
         type = par_cookie;
      }             
      else 
         type = _pars[i].type;
       
      /*
       * Find parameter value
       *
       */
      p++; 
      for (crec-=p-prec, prec=p; p<pend; p++)
         if (*p == ';')
            break;

      /*
       * Set parameter
       *
       */
      switch (type) {
      case par_cookie:
         dst->pvalue = prec;
         dst->cvalue = p - prec;
         break;
      case par_expires:
         if (dst->expire != 0)
            return true;
         ria_http_parse_time(ok, &dst->expire, prec, p-prec);
         if (!*ok)
            return true;
         *ok = false;
         break;   
      case par_path:
         if (dst->ppath != NULL)
            return true;
         dst->ppath = prec;
         dst->cpath = p - prec;
         break;
      case par_domain:
         if (dst->pdomain != NULL)
            return true;
         dst->pdomain = prec;
         dst->cdomain = p - prec;
         break;
      default:
         ERR_SET(err_internal);
      }  
      
      if (p != pend) {
         p++;
         if (*p != ' ')
            return true;
         p++;
      }      
      crec -= p - prec;
      prec  = p;
   
   }    
   
   *ok = true;
   return true;

}

/*****************************************************************************/
bool 
   ria_http_get_cookie(    
      ria_cookie_t*   IN OUT   dst,
      const buf_t*    IN       src,
      const char*     IN       pname,
      usize           IN       cname)
/*
 * Gets HTTP cookie value
 *
 */
{

   const byte* pos;
   usize len;
   time_t t;
   
   assert(src   != NULL);
   assert(dst   != NULL);
   assert(pname != NULL);

   /* 
    * Initialize result structure
    *
    */
   MemSet(dst, 0x00, sizeof(*dst)); 
   dst->pname   = pname;
   dst->cname   = cname;
   dst->pdomain = NULL;
   dst->ppath   = NULL;
   dst->pvalue  = NULL;
    
   /* 
    * Find cookie start position
    * LL [<cookie>=<value>; EEEE]
    *
    */
   if (!ria_http_find_cookie_pos(&pos, &len, src, pname, cname))
      return false;    
   if (pos == NULL) 
      return true;
   if (len < 10)
      ERR_SET(err_internal);   

   /* 
    * Check expiration
    *
    */
   Time(&t);               
   dst->expire = (pos[len-4] << 24) | (pos[len-3] << 16) | 
                 (pos[len-2] <<  8) |  pos[len-1];
   {
      struct tm* tt = localtime(&dst->expire);
      tt->tm_year += 0;
   }
   if (dst->expire <= t)
      return true;
    
   /* 
    * Extract value:
    *
    */
   dst->pvalue = (char*)pos + cname + 3; 
   dst->cvalue = len - cname - 9;
   return true;
    
}

/*****************************************************************************/
bool 
   ria_http_pack_cookies(    
      buf_t*   IN OUT   src)
/*
 * Packs HTTP cookies into single header format
 *
 */
{

   byte* p;
   usize i, j, c;
   
   assert(src != NULL);

   /* 
    * Scan cookies and pack to header format
    * LL [<cookie>=<value>; EEEE] -> [<cookie>=<value>; ]
    *
    */
   p = buf_get_ptr_bytes(src); 
   c = buf_get_length(src);
   for (i=0; i<c; ) {
      if (c-i < 10)
         ERR_SET(err_internal);
      j = (p[0] << 8) | p[1];
      if (j+2 > c-i)
         ERR_SET(err_internal);
      i += j + 2; 
      MemMove(p, p+2, j-4);
      MemMove(p+j-4, p+j+2, c-i);
      p += j - 4;
      c -= 6;
   }         
      
   /* 
    * Remove trailing '; '
    *
    */
   if (c > 0) {
      p = buf_get_ptr_bytes(src); 
      if ((c < 2) || (p[c-1] != ' ') || (p[c-2] != ';'))
         ERR_SET(err_internal);
      c -= 2;
   } 
   return buf_set_length(c, src);
    
}

/*****************************************************************************/
bool 
   ria_http_set_cookie(    
      buf_t*                IN OUT   dst, 
      const ria_cookie_t*   IN       src)
/*
 * Sets cookie value
 *
 */
{

   byte* p;
   usize c, l, i, j;

   assert(dst != NULL);
   assert(src != NULL);
   
   /* 
    * Find cookie start position
    * LL [<cookie>=<value>; EEEE]
    *
    */
   if (!ria_http_find_cookie_pos(&p, &l, dst, src->pname, src->cname))
      return false;    
   if (p == NULL) {
      if (l != 0)
         ERR_SET(err_internal);
      c = buf_get_length(dst);         
   }
   else {      
      if (l < 10)
         ERR_SET(err_internal);   
      c = buf_get_length(dst) - (buf_get_ptr_bytes(dst) - p);
   }      
      
   /*
    * Check whether move is required
    *
    */
   j = src->cname + src->cvalue + 9; 
   if (l != j) {
      i = buf_get_length(dst);
      if (l < j) {
         if (!buf_expand(i+j-l, dst))
            return false;
         p = buf_get_ptr_bytes(dst) + i - c;   
      }     
      if (!buf_set_length(i+j-l, dst))
         return false;   
      if (c > 0) {
         if (c < l)
            ERR_SET(err_internal);
         MemMove(p, p+l, c-l);
      }         
      p = buf_get_ptr_bytes(dst) + buf_get_length(dst) - j;
   }    

   /*
    * Write
    *
    */
   j -= 2; 
   *(p++) = (byte)(j >> 8);
   *(p++) = (byte)(j >> 0);
   MemCpy(p, src->pname, src->cname);
   p += src->cname;
   *(p++) = '=';
   MemCpy(p, src->pvalue, src->cvalue);
   p += src->cvalue;
   *(p++) = ';';
   *(p++) = ' ';
   *(p++) = (byte)(src->expire >> 24);
   *(p++) = (byte)(src->expire >> 16);
   *(p++) = (byte)(src->expire >>  8);
   *(p++) = (byte)(src->expire >>  0);
   return true;   

}


