#include "ria_func.h"
#include "ria_papi.h"
#include "emb_codr.h"
#include "ria_core.h"


#ifdef USE_RIA_TRACE
#define EXTRACT_STRING_TRACE
#define EXTRACT_STRING_FROM_FILE_TRACE
#endif


/******************************************************************************
 *   Utils
 */

/*
 * Parameters passing
 *
 */
enum {
   ria_param_by_val,
   ria_param_by_ref,
   ria_param_immediate
};

/*
 * Parameter entity
 *
 */
typedef struct ria_param_s {
   bool   is_buf;
   buf_t* buf;
   usize  len;
   union _up {
      const char* str;
      const byte* ptr;
   } uptr;
} ria_param_t;

#define UNPACK_STRING(_v, _p, _c)                                             \
           if (!ria_unpack_func_param(&(_v), ria_string, &(_p), &(_c)))       \
              return false;
#define UNPACK_INT(_v, _p, _c)                                                \
           if (!ria_unpack_func_param(&(_v), ria_int, &(_p), &(_c)))          \
              return false;

/*
 * Multi-header delimiter
 *
 */
const char* _multiheader = "; && ";

#define MAX_COOKIE_SIZE 255

/*****************************************************************************/
static bool 
   _search_regexp(
      char const**   OUT   p,
      char const**   OUT   q,
      const char*    IN    psrc,
      const char*    IN    pexp,
      usize          IN    cexp,
      bool           IN    top)
/*
 * Search for regexp in string
 *
 */
{

   const char* p1;
   const char* p2;
   const char* pe;
   usize csrc;

   assert(p    != NULL);
   assert(q    != NULL);
   assert(psrc != NULL);
   assert(pexp != NULL);

   /*
    * Regexp is <symbols>{1} ( '*'{1} <symbols>{0,1} ){0,1}
    * Regexp assumes implicit '*' in the beginning
    *
    */ 

   /*
    * Initialize
    *
    */ 
   csrc = StrLen(psrc); 
   if (top) {
      *p = NULL;
      *q = NULL;
   }    
 
   /*
    * Detect first symbolic part of regexp
    *
    */
   pe = pexp + cexp;
   for (p1=pexp; p1!=pe; p1++)
      if (*p1 == '*')
         break; 
   if (p1 == pexp)
      ERR_SET(err_internal);      
       
   /*
    * Search loop
    *
    */ 
   for (; csrc>0; ) {
   
      usize ct;
   
      /*
       * Skip unproper chars (process implicit/explicit '*')
       *
       */
      p2 = StrChr(psrc, *pexp);
      if (p2 == NULL) {
         *p = NULL;
         return true;
      }
      csrc -= p2 - psrc;
      psrc  = p2;
      ct    = csrc;
       
      /*
       * Check match
       *
       */
      if ((p1 == pexp+1) || (!StrNICmp(pexp+1, psrc+1, p1-pexp-1))) {
         if (*p == NULL)
            *p = psrc;
         psrc += p1 - pexp;
         csrc -= p1 - pexp;
         for (; p1!=pe; p1++)
            if (*p1 != '*')
               break;
         if (pe == p1)
            *q = psrc;
         else         
            if (!_search_regexp(p, q, psrc, p1, pe-p1, false))
               return false;
         if (*p != NULL) {   
            if (*q == NULL)
               ERR_SET(err_internal);
            return true;         
         }               
      } 
      
      /*
       * Match failed, skip to next pos in source
       *
       */
      psrc = p2 + 1;
      csrc = ct - 1;
  
   } 
   
   /*
    * Check tail
    *
    */ 
   *p = NULL;
   return true;

}

/*****************************************************************************/
static bool 
   _extract_string(
      char const**          OUT   p,
      char const**          OUT   q,
      char const**          OUT   r,
      const char*           IN    src,
      const ria_param_t*    IN    begin,
      const ria_param_t*    IN    end)
/*
 * String extraction
 *
 */
{

   usize c;

   assert(p     != NULL);
   assert(q     != NULL);
   assert(src   != NULL);
   assert(begin != NULL);
   assert(end  != NULL);
   
   *q = NULL;

   /*
    * Find begin marker
    *
    */
   *p = src; 
   c  = StrLen(src);
   if (begin->len != 0) 
      if (!_search_regexp(r, p, src, begin->uptr.str, begin->len, true))
         return false;
   if (*p == NULL)
      return true;

   /*
    * Find end marker
    *
    */
   *q = src + c;
   if (end->len != 0)
      if (!_search_regexp(q, r, *p, end->uptr.str, end->len, true))
         return false;
   return true;

}

/*****************************************************************************/
static bool 
   _get_post(
      buf_t*              IN OUT   dst,
      const char*         IN       tofile,
      const char*         IN       todump,
      const char*         IN       purl,
      const char*         IN       pval, /* == NULL if GET */
      bool                IN       normalize,
      ria_exec_state_t*   IN OUT   ctx)
/*
 * HTTP GET/POST 
 *
 */
{

   static const char _http[]   = "http://";
   static const char _defurl[] = "/";
   static const char _cookie[] = "Cookie";

   const char* psite;
   usize csite;
   usize curl;
   byte* p;
   bool  reentry = false;
   bool  pending;

   assert(purl  != NULL);
   assert(ctx   != NULL);
   assert(dst   != NULL);
   
#ifdef USE_RIA_ASYNC_CALLS
   /*
    * Async receive support
    *
    */
   if (ctx->status == ria_exec_proceed) 
      switch (ctx->http.async.state) {
#ifdef RIA_ASYNC_CONNECT      
      case ria_http_async_connect:
         /*
          * Restore last URL 
          *
          */
         RIA_TRACE_MSG("_get_post(): call after ria_http_async_connect\n");
         if (!ria_http_get_string(&purl, ria_str_last_url, &ctx->http))
            return false;
         curl = StrLen(purl);
         goto send;
#endif         
#ifdef RIA_ASYNC_RECEIVE
      case ria_http_async_receive:
         RIA_TRACE_MSG("_get_post(): call after ria_http_async_receive\n");
         goto receive;
      case ria_http_async_rcvmore:
         RIA_TRACE_MSG("_get_post(): call after ria_http_async_rcvmore\n");
         reentry = true;
         goto receive;
#endif         
      default:
         ERR_SET(err_internal);
      }      
#endif      

   RIA_TRACE_START;
   RIA_TRACE_MSG("_get_post(): normal call\n");
   RIA_TRACE_STOP;
   
   ctx->http.redirects = 0;
again:
   curl = StrLen(purl);
   
   /*
    * Extract site address
    *
    */
   if ((curl > sizeof(_http)-1) && 
       (StrNICmp(_http, purl, sizeof(_http)-1) == 0)) {
      purl += sizeof(_http) - 1;
      curl -= sizeof(_http) - 1;
      for (csite=0, psite=purl; csite<curl; csite++)
         if (purl[csite] == '/')
            break;
      if (csite >= curl) {
         purl = _defurl;
         curl = sizeof(_defurl)-1;
      }
      else {
         curl -= csite;
         purl += csite;
      }
   }
   else {
      /*
       * Use previous one
       *
       */
      if (!ria_http_get_string(&psite, ria_str_site_address, &ctx->http))
         return false;
      if (psite == NULL)
         ERR_SET(err_bad_param);   
      csite = StrLen(psite);         
   }

   RIA_TRACE_START;
   RIA_TRACE_MSG("site=");
   RIA_TRACE_STR(psite, csite);
   RIA_TRACE_MSG(", url=");
   RIA_TRACE_STR(purl, curl);
   RIA_TRACE_MSG("\n");
   RIA_TRACE_STOP;
   
   /*
    * Set cookies
    *
    */
   if (buf_get_length(&ctx->http.cookie) > 0) { 
      if (!ria_http_pack_cookies(&ctx->http.cookie))
         return false;
      if (!ria_http_set_header(
              _cookie,
              sizeof(_cookie)-1,
              (char*)buf_get_ptr_bytes(&ctx->http.cookie),
              buf_get_length(&ctx->http.cookie),
              &ctx->http))
         return false;       
      buf_set_empty(&ctx->http.cookie);       
   }      
    
   /*
    * Connect
    *
    */
#ifdef RIA_ASYNC_CONNECT
   ria_async_set_state(
      &ctx->http.async, 
      ria_http_async_connect,
      ctx);
#endif   
   if (!ria_http_init(&ctx->http))
      return false;
   if (!ria_http_connect(&pending, psite, csite, &ctx->http))
      return false;
   if (pending) {      
#ifdef RIA_ASYNC_CONNECT
      if (ria_async_set_status(&ctx->http.async, ria_exec_pending) 
             == ria_exec_pending) {
         /*
          * Save last URL to restore later
          *
          */
         if (!ria_http_set_string(purl, curl, ria_str_last_url, &ctx->http))
            return false;
         return true;
      }      
#else
      ERR_SET(err_internal);
#endif
   }

#ifdef RIA_HTTP_ATOMIC    

   /*
    * Setup receive parameters for atomic callback
    *
    */
   ctx->http.tofile    = tofile;
   ctx->http.todump    = todump;
   ctx->http.normalize = normalize;   
   ctx->http.reentry   = false;

   /*
    * Perform HTTP request as atomic operation
    *
    */
   if (!ria_http_send(
           &pending,
           purl, 
           curl, 
           pval, 
           (pval==NULL)?0:StrLen(pval), 
           &ctx->http))
      return false;
   if (pending)
      ERR_SET(err_internal);   
#else /* RIA_HTTP_ATOMIC */

   /*
    * Send
    *
    */
#ifdef RIA_ASYNC_RECEIVE
send:
   ria_async_set_state(
      &ctx->http.async, 
      ria_http_async_receive,
      ctx);
#endif   
   if (!ria_http_send(
           &pending, 
           purl, 
           curl, 
           pval, 
           (pval==NULL)?0:StrLen(pval), 
           &ctx->http))
      return false;
   if (pending) {      
#ifdef RIA_ASYNC_RECEIVE
      if (ria_async_set_status(&ctx->http.async, ria_exec_pending) 
            == ria_exec_pending) 
         return true;
#else
      ERR_SET(err_internal);         
#endif
   }
      
   /*
    * Receive
    *
    */
#ifdef RIA_ASYNC_RECEIVE
receive:
   ria_async_set_state(
      &ctx->http.async, 
      ria_http_async_rcvmore,
      ctx);
#endif   
   if (!ria_http_receive(
           &pending, 
           tofile, 
           todump, 
           normalize, 
           reentry, 
           &ctx->http))
      return false;
   if (pending) {
#ifdef RIA_ASYNC_RECEIVE
      if (ria_async_set_status(&ctx->http.async, ria_exec_pending) 
             == ria_exec_pending) 
         return true;
      reentry = true;   
      goto receive;   
#else      
      ERR_SET(err_internal);
#endif
   }   
   
#endif /* !RIA_HTTP_ATOMIC */

   /* 
    * Process redirects
    *
    */
   if (ctx->http.http_code == 301) {
      static const char _location[] = "location";   
      if (!ria_http_get_header(
              &ctx->http.temp, 
              _location, 
              sizeof(_location)-1, 
              &ctx->http))
         return false;    
      purl = (char*)buf_get_ptr_bytes(&ctx->http.temp);
      if (++ctx->http.redirects < 5)
         goto again;                
   }     
        
   /* 
    * Save status code
    *
    */
   if (!ria_prealloc_datatype_in_buf(&p, ria_int, 4, dst))
      return false;                                            
   U_OS_B_COUNT(p, 4, ctx->http.http_code);
   return true;

}

/*****************************************************************************/
static bool 
   _open_cookie_file(
      bool*           OUT      ok,
      void**          OUT      file,
      const char*     IN       psite,
      usize           IN       csite,
      bool            IN       read,
      ria_config_t*   IN OUT   config)
/*
 * Open cookie file  
 *
 */
{

   usize i, c;
   byte* p;
   
   assert(ok     != NULL);
   assert(file   != NULL);
   assert(psite  != NULL);
   assert(config != NULL);
   
   *ok = false;

   /*
    * Generate cookie file name 
    *
    */
   c = buf_get_length(&config->tempdir); 
   if (!buf_expand(c+csite+1, &config->tempdir))
      return false;    
   p = buf_get_ptr_bytes(&config->tempdir) + c;   
   for (i=0; i<csite; i++) 
      p[i] = (psite[i] == '.') ? '_' : psite[i];
   p[csite] = 0x00;      

   /*
    * Open cookie file
    *
    */
   if (!ria_papi_fopen(file, (char*)p-c, (read)?"rb":"r+b")) {
      /*
       * Assume that there's no cookie file
       *
       */
      if (read) 
         return true;
      /*
       * For write mode, try to create file
       *
       */
      if (!ria_papi_fopen(file, (char*)p-c, "w+b")) 
         return false;
   }      
   *ok = true;
   return true;
   
}         


/******************************************************************************
 *   API
 */

/*****************************************************************************/
bool 
   ria_pack_func_param(
      byte**            IN OUT   pdst,
      usize*            IN OUT   cdst,
      ria_data_kind_t   IN       kind,
      void*             IN       ppar)
/*
 * Packs function parameter 
 *
 */
{

   ria_type_t type;
   byte* ptr;
   usize len;

   assert(pdst != NULL);
   assert(cdst != NULL);
   
   if (ppar == NULL)
      ERR_SET(err_internal);

   switch (kind) {
   case ria_data_var:
      /*
       * Vars are passed by ref
       *
       */
      if (*cdst < sizeof(void*)+1)
         ERR_SET(err_internal);
      (*pdst)[0] = (byte)ria_param_by_ref;
      (*pdst)++;
      (*cdst)--;
      break;
   case ria_data_tmp:
      /*
       * Temps are passed by val
       *
       */
      if (*cdst < sizeof(void*)+4)
         ERR_SET(err_internal);
      (*pdst)[0] = (byte)ria_param_by_val;
      if (!ria_get_datainfo_from_buf(&type, &ptr, &len, (buf_t*)ppar))
         return false;
      if (len > 0xFFFF)
         ERR_SET(err_internal);
      (*pdst)[1] = (byte)type;
      (*pdst)[2] = (byte)(len >> 8);
      (*pdst)[3] = (byte)(len >> 0);
      (*pdst) += 4;
      (*cdst) -= 4;
      PTR_OS((*pdst), ptr);
      (*cdst) -= sizeof(void*);
      return true;
   case ria_data_str:
   case ria_data_par:
      /*
       * Strings are passed by val
       *
       */
      if (*cdst < sizeof(void*)+4)
         ERR_SET(err_internal);
      (*pdst)[0] = (byte)ria_param_by_val;
      len = StrLen((char*)ppar) + 1;
      (*pdst)[1] = (byte)ria_string;
      (*pdst)[2] = (byte)(len >> 8);
      (*pdst)[3] = (byte)(len >> 0);
      (*pdst) += 4;
      (*cdst) -= 4;
      break;
   case ria_data_int:
      /*
       * Integers are passed as immediate value
       *
       */
      if (*cdst < 6)
         ERR_SET(err_internal);
      len = (*((byte*)ppar) & 0x03) + 1;
      ptr = (byte*)ppar + 1;
      (*pdst)[0] = (byte)ria_param_immediate;
      (*pdst)[1] = (byte)ria_int;
      (*pdst)[2] = (len>3) ? *(ptr++) : 0;
      (*pdst)[3] = (len>2) ? *(ptr++) : 0;
      (*pdst)[4] = (len>1) ? *(ptr++) : 0;
      (*pdst)[5] = *ptr;
      (*pdst) += 6;
      (*cdst) -= 6;
      return true;   
   default:
      ERR_SET(err_internal);
   }

   PTR_OS((*pdst), ppar);
   (*cdst) -= sizeof(void*);
   return true;

}

/*****************************************************************************/
bool 
   ria_unpack_func_param(
      ria_param_t*   OUT      par,
      ria_type_t     IN       type,
      byte**         IN OUT   psrc,
      usize*         IN OUT   csrc)
/*
 * Unpacks function parameter 
 *
 */
{

   void* pt;

   assert(par  != NULL);
   assert(psrc != NULL);
   assert(csrc != NULL);

   /*
    * Check by ref or by val
    *
    */
   if (*csrc < 1)
      ERR_SET(err_internal);
   switch ((*psrc)[0]) {
   case ria_param_by_ref:
      par->is_buf = true;
      par->len = 0;
      (*psrc)++;
      (*csrc)--;
      break;
   case ria_param_by_val:
      if (*csrc < 4)
         ERR_SET(err_internal);
      if ((byte)type != (*psrc)[1])
         ERR_SET(err_internal);
      par->is_buf = false;
      par->len = ((*psrc)[2] << 8) | (*psrc)[3];
      (*psrc) += 4;
      (*csrc) -= 4;
      if (par->len == 0)
         ERR_SET(err_internal);
      break;
   case ria_param_immediate:
      if (*csrc < 2)
         ERR_SET(err_internal);
      if ((byte)type != (*psrc)[1])
         ERR_SET(err_internal);
      if (type == ria_int) {
         if (*csrc < 6)
            ERR_SET(err_internal);
         par->is_buf = false;
         par->len = 4;
         par->uptr.ptr = (*psrc) + 2;
         (*psrc) += 6;
         (*csrc) -= 6;
         return true;
      }
   default:
      ERR_SET(err_internal);
   }

   /*
    * Get pointer
    *
    */
   OS_PTR(pt, (*psrc));
   par->uptr.ptr = (byte*)pt;
   (*csrc) -= sizeof(void*);

   /*
    * Check type if buffer
    *
    */
   if (par->is_buf) {
      ria_type_t type2;
      par->buf = (buf_t*)par->uptr.ptr;
      if (!ria_get_datainfo_from_buf(
              &type2, 
              (byte**)&par->uptr.ptr, 
              &par->len, 
              par->buf))
         return false;
      if (type2 != type)
         ERR_SET(err_internal);
   }

   if (type == ria_string)
      par->len--;
   return true;

}

/*****************************************************************************/
bool 
   ria_implement_add_parsing_rule(
      buf_t*              IN OUT   dst,
      umask*              OUT      flags,
      byte*               IN       ppar,
      usize               IN       cpar,
      ria_exec_state_t*   IN OUT   ctx)
/*
 * create_parser_for_file implementation
 *
 */
{

   /*
    * par1 - rule name <string>
    * par2 - rule begin <string>
    * par3 - rule end <string>
    * par4 - hint <string>
    *
    */
    
   ria_param_t name;
   ria_param_t begin;
   ria_param_t end;
   ria_param_t hint;
   ctx, dst;

   assert(dst   != NULL);
   assert(ppar  != NULL);
   assert(ctx   != NULL);
   assert(flags != NULL);

   UNPACK_STRING(name, ppar, cpar);
   UNPACK_STRING(begin, ppar, cpar);
   UNPACK_STRING(end, ppar, cpar);
   UNPACK_STRING(hint, ppar, cpar);
   if (cpar != 0)
      ERR_SET(err_internal);
   buf_set_empty(dst);
   *flags = 0;   
   
   /*
    * Add parser rule
    *
    */
   if (!ria_parser_add_rule(
           &ctx->parser, 
           name.uptr.str,
           name.len,
           begin.uptr.str,
           begin.len,
           end.uptr.str,
           end.len,
           hint.uptr.str,
           hint.len))
      return false;           
   return true;
    
}

/*****************************************************************************/
bool 
   ria_implement_create_parser_for_file(
      buf_t*              IN OUT   dst,
      umask*              OUT      flags,
      byte*               IN       ppar,
      usize               IN       cpar,
      ria_exec_state_t*   IN OUT   ctx)
/*
 * create_parser_for_file implementation
 *
 */
{

   /*
    * par1 - file name <string>
    * par2 - parser type <string>
    *
    */
    
   ria_param_t filename;
   ria_param_t type;
   ctx, dst;

   assert(dst   != NULL);
   assert(ppar  != NULL);
   assert(ctx   != NULL);
   assert(flags != NULL);

   UNPACK_STRING(filename, ppar, cpar);
   UNPACK_STRING(type, ppar, cpar);
   if (cpar != 0)
      ERR_SET(err_internal);
   buf_set_empty(dst);
   *flags = 0;   
   
   /*
    * Signal parser creation
    *
    */
   if (!ria_parser_reset(
           &ctx->parser, 
           filename.uptr.str,
           filename.len,
           true,
           type.uptr.str, 
           type.len))
      return false; 
   ctx->flags |= ria_ef_parser_ready;
   return true;
    
}

/*****************************************************************************/
bool 
   ria_implement_dehtml(
      buf_t*              IN OUT   dst,
      umask*              OUT      flags,
      byte*               IN       ppar,
      usize               IN       cpar,
      ria_exec_state_t*   IN OUT   ctx)
/*
 * de-HTML implementation
 *
 */
{

   /*
    * par1 - source string <string>
    *
    */
    
   typedef struct de_s {
      const char* pname;
      usize       cname;
      unicode     replace;
   } de_t;

#define DE(_t, _r) { _t, sizeof(_t)-1, _r }
   static const de_t _tags[] = 
   {
      DE("<br/>", ' ')
   };
   static const de_t _codes[] = 
   {
      DE("nbsp", 160),
      DE("quot", 34)
   };
#undef DE   
    
   usize i, j, k, c;
   unicode code;
   byte* p;
   
   ria_param_t src;
   ctx, dst;

   assert(dst   != NULL);
   assert(ppar  != NULL);
   assert(ctx   != NULL);
   assert(flags != NULL);

   UNPACK_STRING(src, ppar, cpar);
   if (cpar != 0)
      ERR_SET(err_internal);
   *flags = 0;   
   
   /*
    * Scan source string (UTF-8)
    *
    */
   c = src.len; 
   if (!buf_expand(c+1, dst))
      return false;
   p = buf_get_ptr_bytes(dst);   
   for (i=0; (i<src.len)&&(c>0); i++) {
   
      /*
       * Detect &-encoded char
       *
       */
      if (src.uptr.str[i] == '&') {
         if (src.len-i < 2)
            goto copy_char;
         j = i + 1;   
         if (src.uptr.str[j] == '#') {
            for (j++, code=0; j<src.len; j++) {
               if ((src.uptr.str[j] < '0') || (src.uptr.str[j] > '9'))
                  break;
               code = code*10 + src.uptr.str[j] - '0';   
            }   
         }
         else {
            for (k=0; k<sizeof(_codes)/sizeof(_codes[0]); k++) {
               if (src.len-j < _codes[k].cname)
                  continue;
               if (!StrNICmp(src.uptr.str+j, _codes[k].pname, _codes[k].cname))
                  break;
            }      
            if (k == sizeof(_codes)/sizeof(_codes[0]))
               goto copy_char;
            j += _codes[k].cname;
            code = _codes[k].replace;   
         }   
         if ((j == src.len) || (src.uptr.str[j] != ';'))
            goto copy_char;
         k = c;   
         if (!utf8_encode_u16_char(p, &k, code))
            return false;
         p += k;
         c -= k;   
         i  = j;   
         continue;
      }
      
      /*
       * Detect formatting tags
       *
       */
      if (src.uptr.str[i] == '<') {
         for (j=0; j<sizeof(_tags)/sizeof(_tags[0]); j++) {
            if (src.len-i < _tags[j].cname)
               continue;
            if (!StrNICmp(src.uptr.str+i, _tags[j].pname, _tags[j].cname))
               break;
         }      
         if (j == sizeof(_tags)/sizeof(_tags[0]))
            goto copy_char;
         if (_tags[j].replace != 0x00) {
            k = c;   
            if (!utf8_encode_u16_char(p, &k, _tags[j].replace))
               return false;
            p += k;
            c -= k;   
         }
         i += _tags[j].cname - 1;
         continue;             
      } 
       
      /*
       * Skip UTF-8 encoded
       *
       */
      if ((src.uptr.str[i] & 0x80) == 0x00)
         goto copy_char;
      if ((src.uptr.str[i] & 0xE0) == 0xC0) {
         *(p++) = src.uptr.str[i++], c--;      
         goto copy_char;
      }   
      if ((src.uptr.str[i] & 0xF0) == 0xE0) {
         *(p++) = src.uptr.str[i++], c--;      
         *(p++) = src.uptr.str[i++], c--;      
         goto copy_char;
      }   
      ERR_SET(err_not_supported);      
      
copy_char:
      *(p++) = src.uptr.str[i];      
      c--;
   
   } 

   if (i != src.len)
      ERR_SET(err_internal);
   *p = 0x00;
   c  = src.len - c;
   return buf_set_length(c+1, dst);
    
}

/*****************************************************************************/
bool 
   ria_implement_extract_string(
      buf_t*              IN OUT   dst,
      umask*              OUT      flags,                     
      byte*               IN       ppar,
      usize               IN       cpar,
      ria_exec_state_t*   IN OUT   ctx)
/*
 * extract_string implementation
 *
 */
{

   /*
    * par1 - source string <string>
    * par2 - position <int>
    * par3 - begin marker <string>
    * par4 - end marker <string>
    *
    */

   const char* p;
   const char* q;
   const char* r;
   byte* pt;
   usize u;

   ria_param_t src;
   ria_param_t pos;
   ria_param_t begin;
   ria_param_t end;
   ctx, dst;

   assert(dst   != NULL);
   assert(ppar  != NULL);
   assert(ctx   != NULL);
   assert(flags != NULL);

   UNPACK_STRING(src, ppar, cpar);
   UNPACK_INT(pos, ppar, cpar);
   UNPACK_STRING(begin,  ppar, cpar);
   UNPACK_STRING(end,  ppar, cpar);
   if (cpar != 0)
      ERR_SET(err_internal);
   if (!buf_fill(0x00, 0, 1, dst))
      return false;
   if (!buf_set_length(1, dst))
      return false;
   *flags = 0;   

   /*
    * Apply position
    *
    */
   pt = (byte*)pos.uptr.ptr; 
   OS_B_U_COUNT(u, pt, pos.len);

#ifdef EXTRACT_STRING_TRACE
   RIA_TRACE_START;
   RIA_TRACE_MSG("Entering extract_string()\nbegin=");
   RIA_TRACE_STR(begin.uptr.str, begin.len);
   RIA_TRACE_MSG("\nend=");
   RIA_TRACE_STR(end.uptr.str, end.len);
   RIA_TRACE_MSG("\npos=");
   RIA_TRACE_INT(u);
   RIA_TRACE_MSG("\nsrc.len=");
   RIA_TRACE_INT(src.len);
   RIA_TRACE_MSG("\n");
   RIA_TRACE_STOP;
#endif   
    
   if (u >= src.len) 
      return true;    

   /*
    * Extract
    *
    */
   if (!_extract_string(&p, &q, &r, src.uptr.str+u, &begin, &end))
      return false; 
   if ((p == NULL) || (q == NULL)) {
      u = src.len;
#ifdef EXTRACT_STRING_TRACE
      RIA_TRACE_START;
      RIA_TRACE_MSG("Nothing is found\n");
      RIA_TRACE_STOP;
#endif   
      goto update_pos;
   }
   if (r == NULL)
      ERR_SET(err_internal);        

   /*
    * Save result
    *
    */
   if (!buf_expand(q-p+1, dst))
      return false;
   MemCpy(buf_get_ptr_bytes(dst), p, q-p);
   buf_get_ptr_bytes(dst)[q-p] = 0x00;
   if (!buf_set_length(q-p+1, dst))
      return false;
   u = r - src.uptr.str;
#ifdef EXTRACT_STRING_TRACE
   RIA_TRACE_START;
   RIA_TRACE_MSG("Result=");
   RIA_TRACE_STR(p, q-p);
   RIA_TRACE_MSG("\n");
   RIA_TRACE_STOP;
#endif
   
   /*
    * Return updated pos if possible
    *
    */
update_pos:
   if (pos.is_buf) {
      if (!ria_prealloc_datatype_in_buf(&pt, ria_int, 4, pos.buf))
         return false;                                            
      U_OS_B_COUNT(pt, 4, u);
   }
#ifdef EXTRACT_STRING_TRACE
   RIA_TRACE_START;
   RIA_TRACE_MSG("Updated pos=");
   RIA_TRACE_INT(u);
   RIA_TRACE_MSG("\n");
   RIA_TRACE_STOP;
#endif   
   return true; 
    
}

/*****************************************************************************/
bool 
   ria_implement_extract_string_from_file(
      buf_t*              IN OUT   dst,
      umask*              OUT      flags,                     
      byte*               IN       ppar,
      usize               IN       cpar,
      ria_exec_state_t*   IN OUT   ctx)
/*
 * extract_string implementation
 *
 */
{

#define CHUNK 1025

   /*
    * par1 - source file <string>
    * par2 - position <int>
    * par3 - begin marker <string>
    * par4 - end marker <string>
    *
    */
    
   const char* p;
   const char* q;
   const char* r;
   byte* pt;
   usize c, u, i;
   void* file;
   bool  ret;
   
   enum {
      cleanup_file = 0x01
   } cleanup = 0x00;

   ria_param_t filename;
   ria_param_t pos;
   ria_param_t begin;
   ria_param_t end;
   ctx, dst;

   assert(dst   != NULL);
   assert(ppar  != NULL);
   assert(ctx   != NULL);
   assert(flags != NULL);

   UNPACK_STRING(filename, ppar, cpar);
   UNPACK_INT(pos, ppar, cpar);
   UNPACK_STRING(begin,  ppar, cpar);
   UNPACK_STRING(end,  ppar, cpar);
   if (cpar != 0)
      ERR_SET(err_internal);
   if (begin.len >= CHUNK-1)
      ERR_SET(err_internal);
   if (end.len >= CHUNK-1)
      ERR_SET(err_internal);
   *flags = 0;   

   /*
    * Apply position
    *
    */
   pt = (byte*)pos.uptr.ptr; 
   OS_B_U_COUNT(u, pt, pos.len);

#ifdef EXTRACT_STRING_FROM_FILE_TRACE
   RIA_TRACE_START;
   RIA_TRACE_MSG("Entering extract_string_from_file()\nbegin=");
   RIA_TRACE_STR(begin.uptr.str, begin.len);
   RIA_TRACE_MSG("\nend=");
   RIA_TRACE_STR(end.uptr.str, end.len);
   RIA_TRACE_MSG("\npos=");
   RIA_TRACE_INT(u);
   RIA_TRACE_MSG("\n");
   RIA_TRACE_STOP;
#endif   
      
   /* 
    * Open file and read from proper pos
    *      
    */
   ret = false; 
   c = buf_get_length(&ctx->http.config->tempdir);
   if (!buf_expand(c+filename.len+1, &ctx->http.config->tempdir))
      return false;
   pt = buf_get_ptr_bytes(&ctx->http.config->tempdir); 
   MemCpy(pt+c, filename.uptr.str, filename.len+1);
   if (!ria_papi_fopen(&file, (char*)pt, "rb"))
      return false;
   else
      cleanup |= cleanup_file;   
   if (!ria_papi_fseek(u, ria_file_seek_begin, file))
      goto exit;
   if (!buf_expand(CHUNK, dst))
      goto exit;
   pt = buf_get_ptr_bytes(dst);   
   
   /* 
    * Search loop
    *      
    */
   for (i=0; ; u+=c-i) {
      if (!ria_papi_fread(&c, pt+i, CHUNK-i-1, file))
         goto exit;
      if (c == 0) 
         goto not_found;      
      c += i;   
      pt[c] = 0x00;   
      if (!_extract_string(&p, &q, &r, (char*)pt, &begin, &end))
         goto exit;
      if (p == NULL) {
         i = begin.len;
         MemMove(pt, pt+c-i, i);
         continue;   
      }
      if (q == NULL) {
         i = begin.len + (((char*)pt+c) - p);
         if (i == CHUNK-1) 
            i = ((char*)pt+c) - p;
         MemMove(pt, pt+c-i, i);
         continue;   
      }
      if (r == NULL)
         ERR_SET(err_internal);
      u += r - (char*)pt;
      break;
   } 

   /*
    * Save result
    *
    */
   MemMove(pt, p, q-p);
   pt[q-p] = 0x00;
   if (!buf_set_length(q-p+1, dst))
      goto exit;
   ret = true;
#ifdef EXTRACT_STRING_FROM_FILE_TRACE
   RIA_TRACE_START;
   RIA_TRACE_MSG("Result=");
   RIA_TRACE_STR(pt, q-p);
   RIA_TRACE_MSG("\n");
   RIA_TRACE_STOP;
#endif
   goto update_pos;
   
not_found:
   /*
    * Mark result as empty if not found
    *
    */   
   if (!buf_fill(0x00, 0, 1, dst))
      goto exit;
   if (!buf_set_length(1, dst))
      goto exit;
#ifdef EXTRACT_STRING_FROM_FILE_TRACE
   RIA_TRACE_START;
   RIA_TRACE_MSG("Nothing is found\n");
   RIA_TRACE_STOP;
#endif   
   ret = true;
   
   /*
    * Return updated pos if possible
    *
    */
update_pos:    
   if (pos.is_buf) {
      if (!ria_prealloc_datatype_in_buf(&pt, ria_int, 4, pos.buf))
         goto exit;                                            
      U_OS_B_COUNT(pt, 4, u);
   }
#ifdef EXTRACT_STRING_FROM_FILE_TRACE
   RIA_TRACE_START;
   RIA_TRACE_MSG("Updated pos=");
   RIA_TRACE_INT(u);
   RIA_TRACE_MSG("\n");
   RIA_TRACE_STOP;
#endif   
   
exit:
   if (cleanup & cleanup_file)
      ret = ria_papi_fclose(file) && ret;
   return ret; 
   
#undef CHUNK
    
}

/*****************************************************************************/
bool 
   ria_implement_get_binary_to_file(
      buf_t*              IN OUT   dst,
      umask*              OUT      flags,                     
      byte*               IN       ppar,
      usize               IN       cpar,
      ria_exec_state_t*   IN OUT   ctx)
/*
 * HTTP GET implementation, binary case, stores result if nile
 *
 */
{

   /*
    * par1 - file name <string>
    * par2 - url <string>
    *
    */

   ria_param_t file;
   ria_param_t url;
   dst;

   assert(dst   != NULL);
   assert(ppar  != NULL);
   assert(ctx   != NULL);
   assert(flags != NULL);

   UNPACK_STRING(file, ppar, cpar);
   UNPACK_STRING(url,  ppar, cpar);
   if (cpar != 0)
      ERR_SET(err_internal);
   *flags = ria_func_dst_ready;   

   /*
    * Call helper
    *
    */
   return _get_post(dst, file.uptr.str, NULL, url.uptr.str, NULL, false, ctx);

}

/*****************************************************************************/
bool 
   ria_implement_get_header(
      buf_t*              IN OUT   dst,
      umask*              OUT      flags,                     
      byte*               IN       ppar,
      usize               IN       cpar,
      ria_exec_state_t*   IN OUT   ctx)
/*
 * HTTP header access implementation
 *
 */
{

   /*
    * par1 - header name <string>
    *
    */

   ria_param_t header;

   assert(dst   != NULL);
   assert(ppar  != NULL);
   assert(ctx   != NULL);
   assert(flags != NULL);

   UNPACK_STRING(header, ppar, cpar);
   if (cpar != 0)
      ERR_SET(err_internal);
   if (!buf_fill(0x00, 0, 1, dst))
      return false;
   *flags = 0;   

   /*
    * Get header value
    *
    */
   return ria_http_get_header(
             dst, 
             header.uptr.str, 
             header.len, 
             &ctx->http);

}

/*****************************************************************************/
bool 
   ria_implement_get_html(
      buf_t*              IN OUT   dst,
      umask*              OUT      flags,                     
      byte*               IN       ppar,
      usize               IN       cpar,
      ria_exec_state_t*   IN OUT   ctx)
/*
 * HTTP GET implementation, HTML case
 *
 */
{

   /*
    * par1 - url <string>
    *
    */

   ria_param_t url;

   assert(dst   != NULL);
   assert(ppar  != NULL);
   assert(ctx   != NULL);
   assert(flags != NULL);

   UNPACK_STRING(url, ppar, cpar);
   if (cpar != 0)
      ERR_SET(err_internal);
   *flags = ria_func_dst_ready;   

   /*
    * Call helper
    *
    */
   return _get_post(dst, NULL, NULL, url.uptr.str, NULL, true, ctx);
   
}

/*****************************************************************************/
bool 
   ria_implement_get_html_with_dump(
      buf_t*              IN OUT   dst,
      umask*              OUT      flags,                     
      byte*               IN       ppar,
      usize               IN       cpar,
      ria_exec_state_t*   IN OUT   ctx)
/*
 * HTTP GET implementation, HTML case, making dump 
 *
 */
{

   /*
    * par1 - url <string>
    * par2 - dump filename <string>
    *
    */

   ria_param_t url;
   ria_param_t dump;

   assert(dst   != NULL);
   assert(ppar  != NULL);
   assert(ctx   != NULL);
   assert(flags != NULL);

   UNPACK_STRING(url, ppar, cpar);
   UNPACK_STRING(dump, ppar, cpar);
   if (cpar != 0)
      ERR_SET(err_internal);
   *flags = ria_func_dst_ready;   

   /*
    * Call helper
    *
    */
   return _get_post(dst, NULL, dump.uptr.str, url.uptr.str, NULL, true, ctx);
   
}

/*****************************************************************************/
bool 
   ria_implement_get_html_to_file(
      buf_t*              IN OUT   dst,
      umask*              OUT      flags,                     
      byte*               IN       ppar,
      usize               IN       cpar,
      ria_exec_state_t*   IN OUT   ctx)
/*
 * HTTP GET implementation, HTML case, stores result if file
 *
 */
{

   /*
    * par1 - file name <string>
    * par2 - url <string>
    *
    */

   ria_param_t file;
   ria_param_t url;

   assert(dst   != NULL);
   assert(ppar  != NULL);
   assert(ctx   != NULL);
   assert(flags != NULL);

   UNPACK_STRING(file, ppar, cpar);
   UNPACK_STRING(url,  ppar, cpar);
   if (cpar != 0)
      ERR_SET(err_internal);
   *flags = ria_func_dst_ready;   

   /*
    * Call helper
    *
    */
   return _get_post(dst, file.uptr.str, NULL, url.uptr.str, NULL, true, ctx);

}

/*****************************************************************************/
bool 
   ria_implement_get_html_to_file_with_dump(
      buf_t*              IN OUT   dst,
      umask*              OUT      flags,                     
      byte*               IN       ppar,
      usize               IN       cpar,
      ria_exec_state_t*   IN OUT   ctx)
/*
 * HTTP GET implementation, HTML case, making dump, stores result if file
 *
 */
{

   /*
    * par1 - file name <string>
    * par2 - url <string>
    * par3 - dump file name <string>
    *
    */

   ria_param_t file;
   ria_param_t url;
   ria_param_t dump;

   assert(dst   != NULL);
   assert(ppar  != NULL);
   assert(ctx   != NULL);
   assert(flags != NULL);

   UNPACK_STRING(file, ppar, cpar);
   UNPACK_STRING(url,  ppar, cpar);
   UNPACK_STRING(dump, ppar, cpar);
   if (cpar != 0)
      ERR_SET(err_internal);
   *flags = ria_func_dst_ready;   

   /*
    * Call helper
    *
    */
   return _get_post(
              dst, 
              file.uptr.str, 
              dump.uptr.str, 
              url.uptr.str, 
              NULL,
              true, 
              ctx);

}

/*****************************************************************************/
bool 
   ria_implement_int_to_string(
      buf_t*              IN OUT   dst,
      umask*              OUT      flags,                     
      byte*               IN       ppar,
      usize               IN       cpar,
      ria_exec_state_t*   IN OUT   ctx)
/*
 * int_to_string implementation
 *
 */
{

   /*
    * par1 - src <int>
    *
    */
    
   ria_param_t src;
   byte  buf[12];
   usize u;
   byte* pt;
   ctx;
   
   assert(dst   != NULL);
   assert(ppar  != NULL);
   assert(ctx   != NULL);
   assert(flags != NULL);

   UNPACK_INT(src, ppar, cpar);
   if (cpar != 0)
      ERR_SET(err_internal);
   *flags = ria_func_dst_ready;

   /*
    * Convert 
    *
    */
   pt = (byte*)src.uptr.ptr; 
   OS_B_U_COUNT(u, pt, src.len);
   pt = buf + sizeof(buf) - 1;
   for (; u>0; u/=10) {
      if (pt == buf-1)
         ERR_SET(err_internal);
      *(pt--) = u%10 + '0';
   }
   u = sizeof(buf) - (pt-buf) - 1;
   if (!ria_prealloc_datatype_in_buf(&pt, ria_string, u+1, dst))
      return false;                                         
   MemCpy(pt, buf+sizeof(buf)-u, u);
   pt[u] = 0x00;
   return true;

}

/*****************************************************************************/
bool 
   ria_implement_last_response(
      buf_t*              IN OUT   dst,
      umask*              OUT      flags,                     
      byte*               IN       ppar,
      usize               IN       cpar,
      ria_exec_state_t*   IN OUT   ctx)
/*
 * last_response implementation
 *
 */
{

   /*
    * No params
    *
    */

   assert(dst   != NULL);
   assert(ctx   != NULL);
   assert(flags != NULL);
   ppar;

   if (cpar != 0)
      ERR_SET(err_internal);
   *flags = ria_func_dst_ready;
   
   return buf_attach(
             buf_get_ptr_bytes(&ctx->http.resp),
             buf_get_length(&ctx->http.resp),
             buf_get_length(&ctx->http.resp),
             true,
             dst);

}

/*****************************************************************************/
bool 
   ria_implement_length(
      buf_t*              IN OUT   dst,
      umask*              OUT      flags,                     
      byte*               IN       ppar,
      usize               IN       cpar,
      ria_exec_state_t*   IN OUT   ctx)
/*
 * length (of string) implementation
 *
 */
{

   /*
    * par1 - string <string>
    *
    */
    
   ria_param_t str;
   byte* pt;
   ctx;
   
   assert(dst   != NULL);
   assert(ppar  != NULL);
   assert(ctx   != NULL);
   assert(flags != NULL);

   UNPACK_STRING(str, ppar, cpar);
   if (cpar != 0)
      ERR_SET(err_internal);
   *flags = ria_func_dst_ready;      

   /*
    * Save length
    *
    */
   if (!ria_prealloc_datatype_in_buf(&pt, ria_int, 4, dst))
      return false;                                            
   U_OS_B_COUNT(pt, 4, str.len);
   return true;  

}

/*****************************************************************************/
bool 
   ria_implement_load_cookie(
      buf_t*              IN OUT   dst,
      umask*              OUT      flags,                     
      byte*               IN       ppar,
      usize               IN       cpar,
      ria_exec_state_t*   IN OUT   ctx)
/*
 * load_cookie implementation
 *
 */
{

   /*
    * par1 - site <string>
    * par2 - user <string>
    * par3 - key <string>
    *
    */
    
   ria_param_t site;
   ria_param_t user;
   ria_param_t key;    
   
   void* file;
   buf_t tmp;
   bool  ret;
   usize c;
   
   ria_cookie_t cookie;

   enum {
      cleanup_file = 0x01,
      cleanup_tmp  = 0x02
   } cleanup = 0x00;

   assert(dst   != NULL);
   assert(ppar  != NULL);
   assert(ctx   != NULL);
   assert(flags != NULL);

   UNPACK_STRING(site, ppar, cpar);
   UNPACK_STRING(user, ppar, cpar);
   UNPACK_STRING(key, ppar, cpar);
   if (cpar != 0)
      ERR_SET(err_internal);
   if (!buf_fill(0x00, 0, 1, dst))
      return false;
   *flags = 0;      

   /*
    * Open cookie file
    *
    */
   if (!_open_cookie_file(
           &ret, 
           &file, 
           site.uptr.str,
           site.len,
           true,
           ctx->http.config))
      return false;
   if (!ret) 
      return true;
   ret = false;
   cleanup |= cleanup_file;   

   /*
    * Load all cookie values
    *
    */
   if (!ria_papi_fseek(0, ria_file_seek_end, file))
      goto exit;   
   if (!ria_papi_ftell(&c, file))
      goto exit;   
   if (c == 0) {
      ret = true;
      goto exit;
   }   
   if (!ria_papi_fseek(0, ria_file_seek_begin, file))
      goto exit;   
   if (!buf_create(1, c, 0, &tmp, ctx->mem))
      goto exit;
   else
      cleanup |= cleanup_tmp;   
   if (!ria_papi_fread(
           &c, 
           buf_get_ptr_bytes(&tmp),
           c,
           file))
      goto exit;     
   if (!buf_set_length(c, &tmp))
      goto exit;      
   
   /*
    * Select proper and apply
    *
    */
   if (!ria_http_get_cookie(&cookie, &tmp, key.uptr.str, key.len))
      goto exit;
   if (cookie.pvalue != NULL) {   
      if (!ria_http_set_cookie(&ctx->http.cookie, &cookie))
         goto exit;
      if (!buf_load(cookie.pvalue, 0, cookie.cvalue, dst))
         goto exit;
      if (!buf_fill(0x00, cookie.cvalue, 1, dst))
         goto exit;      
   }      
   
   ret = true;

exit:    
   if (cleanup & cleanup_file)
      ret = ria_papi_fclose(file) && ret;
   if (cleanup & cleanup_tmp)
      ret = buf_destroy(&tmp) && ret;
   return ret;

}

/*****************************************************************************/
bool 
   ria_implement_load_from_file(
      buf_t*              IN OUT   dst,
      umask*              OUT      flags,                     
      byte*               IN       ppar,
      usize               IN       cpar,
      ria_exec_state_t*   IN OUT   ctx)
/*
 * load_from_file implementation
 *
 */
{

   /*
    * par1 - file <string>
    *
    */
    
   ria_param_t file;
   
   void* pfile;
   bool  ret;
   usize c;
   
   enum {
      cleanup_file = 0x01
   } cleanup = 0x00;

   assert(dst   != NULL);
   assert(ppar  != NULL);
   assert(ctx   != NULL);
   assert(flags != NULL);

   UNPACK_STRING(file, ppar, cpar);
   if (cpar != 0)
      ERR_SET(err_internal);
   if (!buf_fill(0x00, 0, 1, dst))
      return false;
   *flags = 0;      

   ret = false;

   /*
    * Open file
    *
    */
   if (!ria_papi_fopen(&pfile, file.uptr.str, "a+t"))
      goto exit;
   cleanup |= cleanup_file;   

   /*
    * Load data
    *
    */
   if (!ria_papi_fseek(0, ria_file_seek_end, pfile))
      goto exit;   
   if (!ria_papi_ftell(&c, pfile))
      goto exit;   
   if (c == 0) {
      ret = true;
      goto exit;
   }   
   if (!ria_papi_fseek(0, ria_file_seek_begin, pfile))
      goto exit;   
   if (!buf_expand(c+1, dst))
      goto exit;
   if (!ria_papi_fread(
           &c, 
           buf_get_ptr_bytes(dst),
           c,
           pfile))
      goto exit;     
   if (!buf_set_length(c+1, dst))
      goto exit;         
   buf_get_ptr_bytes(dst)[c] = 0x00;
   
   ret = true;

exit:    
   if (cleanup & cleanup_file)
      ret = ria_papi_fclose(pfile) && ret;
   return ret;

}

/*****************************************************************************/
bool 
   ria_implement_post(
      buf_t*              IN OUT   dst,
      umask*              OUT      flags,                     
      byte*               IN       ppar,
      usize               IN       cpar,
      ria_exec_state_t*   IN OUT   ctx)
/*
 * HTTP POST implementation
 *
 */
{

   /*
    * par1 - url <string>
    * par2 - value <string>
    *
    */

   ria_param_t url;
   ria_param_t val;

   assert(dst   != NULL);
   assert(ppar  != NULL);
   assert(ctx   != NULL);
   assert(flags != NULL);

   UNPACK_STRING(url, ppar, cpar);
   UNPACK_STRING(val, ppar, cpar);
   if (cpar != 0)
      ERR_SET(err_internal);
   *flags = ria_func_dst_ready;   

   /*
    * Call helper
    *
    */
   return _get_post(dst, NULL, NULL, url.uptr.str, val.uptr.str, false, ctx);
   
}

/*****************************************************************************/
bool 
   ria_implement_post_with_dump(
      buf_t*              IN OUT   dst,
      umask*              OUT      flags,                     
      byte*               IN       ppar,
      usize               IN       cpar,
      ria_exec_state_t*   IN OUT   ctx)
/*
 * HTTP POST implementation, making dump 
 *
 */
{

   /*
    * par1 - url <string>
    * par2 - value <string>
    * par3 - dump filename <string>
    *
    */

   ria_param_t url;
   ria_param_t val;
   ria_param_t dump;

   assert(dst   != NULL);
   assert(ppar  != NULL);
   assert(ctx   != NULL);
   assert(flags != NULL);

   UNPACK_STRING(url, ppar, cpar);
   UNPACK_STRING(val, ppar, cpar);
   UNPACK_STRING(dump, ppar, cpar);
   if (cpar != 0)
      ERR_SET(err_internal);
   *flags = ria_func_dst_ready;   

   /*
    * Call helper
    *
    */
   return _get_post(
             dst, 
             NULL, 
             dump.uptr.str, 
             url.uptr.str, 
             val.uptr.str, 
             false,
             ctx);
   
}

/*****************************************************************************/
bool 
   ria_implement_post_to_file(
      buf_t*              IN OUT   dst,
      umask*              OUT      flags,                      
      byte*               IN       ppar,
      usize               IN       cpar,
      ria_exec_state_t*   IN OUT   ctx)
/*
 * HTTP POST implementation, stores result if file
 *
 */
{

   /*
    * par1 - file name <string>
    * par2 - url <string>
    * par3 - value <string>
    *
    */

   ria_param_t file;
   ria_param_t url;
   ria_param_t val;

   assert(dst   != NULL);
   assert(ppar  != NULL);
   assert(ctx   != NULL);
   assert(flags != NULL);

   UNPACK_STRING(file, ppar, cpar);
   UNPACK_STRING(url,  ppar, cpar);
   UNPACK_STRING(val,  ppar, cpar);
   if (cpar != 0)
      ERR_SET(err_internal);
   *flags = ria_func_dst_ready;   

   /*
    * Call helper
    *
    */
   return _get_post(
             dst, 
             file.uptr.str, 
             NULL, 
             url.uptr.str, 
             val.uptr.str, 
             false,
             ctx);

}

/*****************************************************************************/
bool 
   ria_implement_post_to_file_with_dump(
      buf_t*              IN OUT   dst,
      umask*              OUT      flags,                     
      byte*               IN       ppar,
      usize               IN       cpar,
      ria_exec_state_t*   IN OUT   ctx)
/*
 * HTTP POST implementation, making dump, stores result if file
 *
 */
{

   /*
    * par1 - file name <string>
    * par2 - url <string>
    * par3 - value <string>
    * par4 - dump file name <string>
    *
    */

   ria_param_t file;
   ria_param_t url;
   ria_param_t val;
   ria_param_t dump;

   assert(dst   != NULL);
   assert(ppar  != NULL);
   assert(ctx   != NULL);
   assert(flags != NULL);

   UNPACK_STRING(file, ppar, cpar);
   UNPACK_STRING(url,  ppar, cpar);
   UNPACK_STRING(val,  ppar, cpar);
   UNPACK_STRING(dump, ppar, cpar);
   if (cpar != 0)
      ERR_SET(err_internal);
   *flags = ria_func_dst_ready;   

   /*
    * Call helper
    *
    */
   return _get_post(
             dst, 
             file.uptr.str, 
             dump.uptr.str, 
             url.uptr.str, 
             val.uptr.str, 
             false,
             ctx);

}

/*****************************************************************************/
bool 
   ria_implement_save_cookie(
      buf_t*              IN OUT   dst,
      umask*              OUT      flags,                     
      byte*               IN       ppar,
      usize               IN       cpar,
      ria_exec_state_t*   IN OUT   ctx)
/*
 * save_cookie implementation
 *
 */
{

   static const char _cookie[] = "set-cookie";
   static const char _any[] = "*";

   /*
    * par1 - site <string>
    * par2 - user <string>
    * par3 - key <string>
    *
    */
    
   ria_param_t site;
   ria_param_t user;
   ria_param_t key;    
   
   void*  file = NULL;
   buf_t  tmp;
   bool   ret;
   byte*  p;
   byte*  p2;
   usize  c, l;
   bool   ok;
   bool   update;

   enum {
      cleanup_tmp  = 0x01,
      cleanup_file = 0x02
   } cleanup = 0x00;

   assert(dst   != NULL);
   assert(ppar  != NULL);
   assert(ctx   != NULL);
   assert(flags != NULL);

   UNPACK_STRING(site, ppar, cpar);
   UNPACK_STRING(user, ppar, cpar);
   UNPACK_STRING(key, ppar, cpar);
   if (cpar != 0)
      ERR_SET(err_internal);
   buf_set_empty(dst);
   *flags = 0;    
   
   /*
    * Open cookie file
    *
    */
   if (!_open_cookie_file(
           &ret, 
           &file, 
           site.uptr.str,
           site.len,
           false,
           ctx->http.config))
      return false;
   if (!ret) 
      return true;
   ret = false;
   cleanup |= cleanup_file;   

   /*
    * Load all saved cookie values
    *
    */
   if (!ria_papi_fseek(0, ria_file_seek_end, file))
      goto exit;   
   if (!ria_papi_ftell(&c, file))
      goto exit;   
   if (c != 0) {
      if (!ria_papi_fseek(0, ria_file_seek_begin, file))
         goto exit;   
      if (!buf_expand(c, &ctx->http.cookie))
         goto exit;
      if (!ria_papi_fread(
              &c, 
              buf_get_ptr_bytes(&ctx->http.cookie),
              c,
              file))
         goto exit;
      if (!buf_set_length(c, &ctx->http.cookie))
         goto exit;           
   }         
  
   /*
    * Get received cookies 
    *
    */
   if (!buf_create(1, 0, 0, &tmp, ctx->mem))
      goto exit;
   else
      cleanup |= cleanup_tmp;   
   if (!ria_http_get_header(&tmp, _cookie, sizeof(_cookie)-1, &ctx->http))
      goto exit;
      
   /*
    * Process one by one
    *
    */
   p = buf_get_ptr_bytes(&tmp);
   c = buf_get_length(&tmp) - 1;
   l = StrLen(_multiheader);
   for (update=false; c!=0; ) {
   
      ria_cookie_t cookie;
      
      /*
       * Parse
       *
       */
      for (p2=p; p2!=NULL;) {
         p2 = (byte*)StrChr((char*)p2, _multiheader[0]);
         if ((p2 != NULL) && (c-(p2-p) >= l)) {
            if (!StrNICmp((char*)p2, _multiheader, l))
               break;
            p2++;   
         }      
      }
      if (!ria_http_parse_cookie(&ok, &cookie, (char*)p, (p2==NULL)?c:p2-p))
         goto exit;
      if (!ok) {
         ERR_SET_NO_RET(err_internal);
         goto exit;   
      }
      
      if (p2 == NULL) 
         c = 0;
      else {   
         c -= p2 - p + l;
         p  = p2 + l;
      }
         
      /*
       * Check whether it is interesting
       *
       */
      if (cookie.cdomain != 0)   
         if (cookie.cdomain < site.len) {
            if (StrNICmp(
                   cookie.pdomain, 
                   site.uptr.str+site.len-cookie.cdomain, 
                   cookie.cdomain))
               continue;      
         }
         else {
            if (StrNICmp(
                   cookie.pdomain+cookie.cdomain-site.len, 
                   site.uptr.str, 
                   site.len))
               continue;      
         }
      if ((sizeof(_any)-1 != key.len) || 
          StrNICmp(_any, key.uptr.str, key.len)) {
         if (cookie.cname != key.len)
            continue;
         if (StrNICmp(cookie.pname, key.uptr.str, key.len))
            continue;      
      }          

      /*
       * Save cookie value
       *
       */
      if (!ria_http_set_cookie(&ctx->http.cookie, &cookie))
         goto exit;
      update = true;   
     
   }

   /*
    * Update cookie in file if required
    *
    */
   if (update) {
      if (!ria_papi_fseek(0, ria_file_seek_begin, file))
         goto exit;   
      if (!ria_papi_fwrite(
              buf_get_ptr_bytes(&ctx->http.cookie), 
              buf_get_length(&ctx->http.cookie), 
              file))
         goto exit;    
   }         
   buf_set_empty(&ctx->http.cookie);          

   ret = true;

exit:
   if (cleanup & cleanup_file)
      ret = ria_papi_fclose(file) && ret;
   if (cleanup & cleanup_tmp)
      ret = buf_destroy(&tmp) && ret;
   return ret;

}

/*****************************************************************************/
bool 
   ria_implement_save_to_file(
      buf_t*              IN OUT   dst,
      umask*              OUT      flags,                     
      byte*               IN       ppar,
      usize               IN       cpar,
      ria_exec_state_t*   IN OUT   ctx)
/*
 * save_to_file implementation
 *
 */
{

   /*
    * par1 - file <string>
    * par2 - data <string>
    *
    */
    
   ria_param_t file;
   ria_param_t data;
   
   void*  pfile = NULL;
   bool   ret;

   enum {
      cleanup_file = 0x01
   } cleanup = 0x00;

   assert(dst   != NULL);
   assert(ppar  != NULL);
   assert(ctx   != NULL);
   assert(flags != NULL);

   UNPACK_STRING(file, ppar, cpar);
   UNPACK_STRING(data, ppar, cpar);
   if (cpar != 0)
      ERR_SET(err_internal);
   buf_set_empty(dst);
   *flags = 0;    

   ret = false;
   
   /*
    * Open file
    *
    */
   if (!ria_papi_fopen(&pfile, file.uptr.str, "wt"))
      goto exit;   
   cleanup |= cleanup_file;   

   /*
    * Save string data to file
    *
    */
   if (!ria_papi_fwrite(data.uptr.ptr, data.len, pfile))
      goto exit;   
   ret = true;

exit:
   if (cleanup & cleanup_file)
      ret = ria_papi_fclose(pfile) && ret;
   return ret;

}

/*****************************************************************************/
bool 
   ria_implement_set_header(
      buf_t*              IN OUT   dst,
      umask*              OUT      flags,                     
      byte*               IN       ppar,
      usize               IN       cpar,
      ria_exec_state_t*   IN OUT   ctx)
/*
 * HTTP header setting implementation
 *
 */
{

   /*
    * par1 - header name <string>
    * par2 - value <string>
    *
    */

   ria_param_t header;
   ria_param_t value;

   assert(dst   != NULL);
   assert(ppar  != NULL);
   assert(ctx   != NULL);
   assert(flags != NULL);

   UNPACK_STRING(header, ppar, cpar);
   UNPACK_STRING(value, ppar, cpar);
   if (cpar != 0)
      ERR_SET(err_internal);
   buf_set_empty(dst);
   *flags = 0;   

   /*
    * Set header value
    *
    */
   return ria_http_set_header(
             header.uptr.str, 
             header.len, 
             value.uptr.str, 
             value.len, 
             &ctx->http);

}

/*****************************************************************************/
bool 
   ria_implement_string_to_int(
      buf_t*              IN OUT   dst,
      umask*              OUT      flags,                     
      byte*               IN       ppar,
      usize               IN       cpar,
      ria_exec_state_t*   IN OUT   ctx)
/*
 * string_to_int implementation
 *
 */
{

   /*
    * par1 - src <string>
    *
    */
    
   ria_param_t src;
   usize i, u;
   byte* pt;
   ctx;
   
   assert(dst   != NULL);
   assert(ppar  != NULL);
   assert(ctx   != NULL);
   assert(flags != NULL);

   UNPACK_STRING(src, ppar, cpar);
   if (cpar != 0)
      ERR_SET(err_internal);
   *flags = ria_func_dst_ready;

   /*
    * Convert 
    *
    */
   u = 0;
   for (i=0; i<src.len; i++) {
      if ((src.uptr.str[i] < '0') || (src.uptr.str[i] > '9')) {
         u = (usize)-1;
         break;
      }
      u = u*10 + src.uptr.str[i] - '0';
   }
   i = (u > 0xFFFFFF) ? 4 : (u > 0xFFFF) ? 3 : (u > 0xFF) ? 2 : 1;
   if (!ria_prealloc_datatype_in_buf(&pt, ria_int, i, dst))
      return false;                                         
   U_OS_B_COUNT(pt, i, u);
   return true;

}

/*****************************************************************************/
bool 
   ria_implement_substring(
      buf_t*              IN OUT   dst,
      umask*              OUT      flags,                     
      byte*               IN       ppar,
      usize               IN       cpar,
      ria_exec_state_t*   IN OUT   ctx)
/*
 * substring implementation
 *
 */
{

   /*
    * par1 - string <string>
    * par2 - pos <int>
    * par3 - len <int>
    *
    */
    
   ria_param_t str;
   ria_param_t pos;
   ria_param_t len;    
   usize l, u;
   byte* pt;
   ctx;
   
   assert(dst   != NULL);
   assert(ppar  != NULL);
   assert(ctx   != NULL);
   assert(flags != NULL);

   UNPACK_STRING(str, ppar, cpar);
   UNPACK_INT(pos, ppar, cpar);
   UNPACK_INT(len, ppar, cpar);
   if (cpar != 0)
      ERR_SET(err_internal);
   if (!buf_fill(0x00, 0, 1, dst))
      return false;
   *flags = 0;      

   /*
    * Get substring
    *
    */
   pt = (byte*)pos.uptr.ptr; 
   OS_B_U_COUNT(u, pt, pos.len);
   if (u >= str.len)
      return true;
   pt = (byte*)len.uptr.ptr; 
   OS_B_U_COUNT(l, pt, len.len);
   if (u+l > str.len)
      l = str.len - u;
   if (!buf_expand(l+1, dst))
      return true;
   MemCpy(buf_get_ptr_bytes(dst), str.uptr.str, l);
   buf_get_ptr_bytes(dst)[l] = 0x00;
   return buf_set_length(l+1, dst);      

}

