#include "ria_pars.h"
#include "ria_papi.h"
#include "ria_exec.h"

/*
 * #define EXTRACT_STRING_FROM_FILE_TRACE
 *
 */

/******************************************************************************
 *  String extraction
 */

/*****************************************************************************/
static bool 
   _search_regexp(
      char const**            OUT      p,
      char const**            OUT      q,
      const char*             IN       psrc,
      const char*             IN       pexp,
      usize                   IN       cexp,
      ria_extract_params_t*   IN OUT   params,
      bool                    IN       top)
/*
 * Search for regexp in string
 *
 */
{

   const char* p1;
   const char* p2;
   const char* pe;
   usize csrc, i, j, k;
   bool exact = false;

   assert(p      != NULL);
   assert(q      != NULL);
   assert(psrc   != NULL);
   assert(pexp   != NULL);
   assert(params != NULL);
   
   if (params->cexcludes & 0x01)
      ERR_SET(err_bad_param);

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
   if (top) {
      for (p1=pexp; p1!=pe; p1++)
         if (*p1 != '*')
            break;
      if (p1 == pexp)
         exact = true;
      else
      if (p1 == pe) { 
         if (*p == NULL)
            *p = psrc;
         *q = psrc + csrc;
         return true;
      }             
      cexp -= p1 - pexp;
      pexp  = p1;
   }
   else
      p1 = pexp;
   for (; p1!=pe; p1++)
      if (*p1 == '*')
         break; 
   if (p1 == pexp)
      ERR_SET(err_bad_param);      
       
   /*
    * Search loop
    *
    */ 
   for (; csrc>0; ) {
   
      usize ct;
   
      /*
       * Skip unproper chars (process implicit/explicit '*')
       * Keep in mind exclude character pairs
       *
       */
      if (exact) 
         p2 = (*psrc == *pexp) ? psrc : NULL;
      else 
         if (params->cexcludes == 0) 
            p2 = StrChr(psrc, *pexp);
         else {
            for (i=csrc, p2=psrc; i>0; i--, p2++) {
               k = buf_get_length(params->tmp);
               if (k > 0) {
                  if (*p2 == buf_get_ptr_bytes(params->tmp)[k-1])
                     if (!buf_set_length(k-1, params->tmp))
                        return false;
                  continue;      
               }         
               if (*p2 == *pexp)
                  break;
               for (j=0; j<params->cexcludes; j+=2) 
                  if (*p2 == params->pexcludes[j]) {
                     if (!buf_expand(k+1, params->tmp))
                        return false;
                     if (!buf_set_length(k+1, params->tmp))
                        return false;
                     buf_get_ptr_bytes(params->tmp)[k] = 
                        params->pexcludes[j+1];
                     break;      
                  }
            }  
            if (i == 0)
               p2 = NULL;
         }  
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
         if (p1 == pe) 
            *q = psrc;
         else {   
            for (; p1!=pe; p1++)
               if (*p1 != '*')
                  break;
            if (pe == p1)
               *q = psrc + csrc;
            else         
               if (!_search_regexp(p, q, psrc, p1, pe-p1, params, false))
                  return false;
         }      
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
      char const**            OUT      p,
      char const**            OUT      q,
      char const**            OUT      r,
      const char*             IN       src,
      ria_extract_params_t*   IN OUT   params)
/*
 * String extraction
 *
 */
{

   usize c;

   assert(p      != NULL);
   assert(q      != NULL);
   assert(src    != NULL);
   assert(params != NULL);
   
   *q = NULL;

   /*
    * Find begin marker
    *
    */
   *p = src; 
   c  = StrLen(src);
   if (params->cbegin != 0) 
      if (!_search_regexp(
              r, p, src, params->pbegin, params->cbegin, params, true))
         return false;
   if (*p == NULL)
      return true;
      
   /*
    * Check begin-only mode
    *
    */
   switch (params->mode) {
   case ria_string_extract_begin_only_pos_only:
      *q = *p;
      *r = *p;
      return true;
   case ria_string_extract_begin_end_pos_only:
   case ria_string_extract_begin_end:
      break;
   default:
      ERR_SET(err_bad_param);   
   } 

   /*
    * Find end marker
    *
    */
   *q = *p;
   if (params->cend != 0)
      if (!_search_regexp(
              q, r, *p, params->pend, params->cend, params, true))
         return false;
   return true;

}
 
/*****************************************************************************/
bool 
   ria_extract_string_from_file(
      bool*                      OUT      ok,
      buf_t*                     IN OUT   dst,
      const char*                IN       pfilename,
      usize                      IN       cfilename,
      ria_extract_params_t*      IN OUT   params,      
      struct ria_exec_state_s*   IN OUT   ctx)
/*
 * Extracts string from file
 *
 */
{

#define CHUNK 1025

   const char* p;
   const char* q;
   const char* r;
   byte* pt;
   usize c, i;
   void* file;
   bool  ret;
   
   enum {
      cleanup_file = 0x01
   } cleanup = 0x00;

   assert(ok        != NULL);
   assert(dst       != NULL);
   assert(pfilename != NULL);
   assert(params    != NULL);

   /*
    * Sanity checks
    *
    */
   if (params->cbegin >= CHUNK-1)
      ERR_SET(err_internal);
   if (params->cend >= CHUNK-1)
      ERR_SET(err_internal);

#ifdef EXTRACT_STRING_FROM_FILE_TRACE
   RIA_TRACE_START;
   RIA_TRACE_MSG("Entering extract_string_from_file()\nbegin=");
   RIA_TRACE_STR(params->pbegin, params->cbegin);
   RIA_TRACE_MSG("\nend=");
   RIA_TRACE_STR(params->pend, params->cend);
   RIA_TRACE_MSG("\npos=");
   RIA_TRACE_INT(*(params->pos));
   switch (params->mode) {
   case ria_string_extract_begin_only_pos_only:
      RIA_TRACE_MSG("\nmode BEGIN-ONLY-POS-ONLY");
      break;
   case ria_string_extract_begin_end_pos_only:
      RIA_TRACE_MSG("\nmode BEGIN-END-POS-ONLY");
      break;
   case ria_string_extract_begin_end:
      RIA_TRACE_MSG("\nmode BEGIN-END");
      break;
   default:
      RIA_TRACE_MSG("\nmode UNKNOWN");
   }
   RIA_TRACE_MSG("\n");
   RIA_TRACE_STOP;
#endif   
      
   /* 
    * Open file and read from proper pos
    *      
    */
   ret = false; 
   *ok = false;
   c = buf_get_length(&ctx->http.config->tempdir);
   if (!buf_expand(c+cfilename+1, &ctx->http.config->tempdir))
      return false;
   pt = buf_get_ptr_bytes(&ctx->http.config->tempdir); 
   MemCpy(pt+c, pfilename, cfilename);
   pt[c+cfilename] = 0x00;
   if (!ria_papi_fopen(&file, (char*)pt, "rb"))
      return false;
   else
      cleanup |= cleanup_file;   
   if (!ria_papi_fseek(*(params->pos), ria_file_seek_begin, file))
      goto exit;
   if (!buf_expand(CHUNK, dst))
      goto exit;
   pt = buf_get_ptr_bytes(dst);   
   
   /* 
    * Search loop
    *      
    */
   buf_set_empty(params->tmp); 
   for (i=0; ; *(params->pos)+=c-i) {
      if (!ria_papi_fread(&c, pt+i, CHUNK-i-1, file))
         goto exit;
      if (c == 0) {
         *(params->pos) += i;
         goto not_found;      
      }   
      c += i;   
      pt[c] = 0x00;   
      if (!_extract_string(&p, &q, &r, (char*)pt, params))
         goto exit;
      if (p == NULL) {
         i = params->cbegin;
         MemMove(pt, pt+c-i, i);
         continue;   
      }
      if (q == NULL) {
         i = params->cbegin + (((char*)pt+c) - p);
         if (i == CHUNK-1) 
            i = ((char*)pt+c) - p;
         MemMove(pt, pt+c-i, i);
         continue;   
      }
      if (r == NULL)
         ERR_SET(err_internal);
      *(params->pos) += r - (char*)pt;
      break;
   } 

   /*
    * Save result
    *
    */
   switch (params->mode) {
   case ria_string_extract_begin_end_pos_only:
   case ria_string_extract_begin_only_pos_only:
      buf_set_empty(dst);
      break;
   case ria_string_extract_begin_end:
      MemMove(pt, p, q-p);
      pt[q-p] = 0x00;
      if (!buf_set_length(q-p+1, dst))
         goto exit;
      break;   
   }   
   ret = true;
   *ok = true;
#ifdef EXTRACT_STRING_FROM_FILE_TRACE
   RIA_TRACE_START;
   RIA_TRACE_MSG("Result=");
   switch (params->mode) {
   case ria_string_extract_begin_end_pos_only:
   case ria_string_extract_begin_only_pos_only:
      RIA_TRACE_MSG("success");
      break;
   case ria_string_extract_begin_end:
      RIA_TRACE_STR(pt, q-p);
      break;   
   }   
   RIA_TRACE_MSG("\n");
   RIA_TRACE_STOP;
#endif
   goto update_pos;
   
not_found:
   /*
    * Mark result as empty if not found
    *
    */   
   switch (params->mode) {
   case ria_string_extract_begin_end_pos_only:
   case ria_string_extract_begin_only_pos_only:
      buf_set_empty(dst);
      break;
   case ria_string_extract_begin_end:
      if (!buf_fill(0x00, 0, 1, dst))
         goto exit;
      if (!buf_set_length(1, dst))
         goto exit;
      break;   
   }   
#ifdef EXTRACT_STRING_FROM_FILE_TRACE
   RIA_TRACE_START;
   RIA_TRACE_MSG("Nothing is found\n");
   RIA_TRACE_STOP;
#endif   
   ret = true;
   
update_pos:    
#ifdef EXTRACT_STRING_FROM_FILE_TRACE
   RIA_TRACE_START;
   RIA_TRACE_MSG("Updated pos=");
   RIA_TRACE_INT(*(params->pos));
   RIA_TRACE_MSG("\n");
   RIA_TRACE_STOP;
#endif   
   
exit:
   if (cleanup & cleanup_file)
      ret = ria_papi_fclose(file) && ret;
   return ret; 
   
#undef CHUNK
    
}


/******************************************************************************
 *  Parser
 */

enum {
   cf_ria_parser_src = 0x01,
   cf_ria_parser_tmp = 0x02
};

#define LIST_TO_RULE(_p)                                                      \
       (                                                                      \
         (ria_parser_rule_t*)((byte*)(_p) -                                   \
         OFFSETOF(ria_parser_rule_t, linkage))                                \
       )

/*****************************************************************************/
bool 
   ria_parser_create(     
      ria_parser_t*   IN OUT   ctx,
      heap_ctx_t*     IN OUT   mem)
/*
 * Creates parser context
 *
 */
{

   assert(ctx != NULL);
   assert(mem != NULL);

   MemSet(ctx, 0x00, sizeof(*ctx));  
   list_init_head(&ctx->rules);
   ctx->type = ria_parser_unknown;
   ctx->mem  = mem;
   
   if (!buf_create(1, 0, 0, &ctx->src, ctx->mem))
      goto failed;
   else
      ctx->cleanup |= cf_ria_parser_src;   
   if (!buf_create(1, 0, 0, &ctx->tmp, ctx->mem))
      goto failed;
   else
      ctx->cleanup |= cf_ria_parser_tmp;   
   return true;
   
failed:
   ria_parser_destroy(ctx);
   return false;    

}

/*****************************************************************************/
static bool 
   ria_parser_cleanup(     
      ria_parser_t*   IN OUT   ctx)
/*
 * Cleans parser context
 *
 */
{

   bool ret = true;

   assert(ctx != NULL);

   while (!list_is_empty(&ctx->rules)) {
      ria_parser_rule_t* rule = LIST_TO_RULE(list_next(&ctx->rules));
      list_remove_entry_simple(&rule->linkage);
      ret = heap_free(rule, ctx->mem) && ret;
   }
   
   ctx->type = ria_parser_unknown;
   return ret;

}

/*****************************************************************************/
bool 
   ria_parser_destroy(     
      ria_parser_t*   IN OUT   ctx)
/*
 * Destroys parser context
 *
 */
{

   bool ret = true;
   
   assert(ctx != NULL);
   
   ret = ria_parser_cleanup(ctx);
   if (ctx->cleanup & cf_ria_parser_src) 
      ret = buf_destroy(&ctx->src) && ret;
   if (ctx->cleanup & cf_ria_parser_tmp) 
      ret = buf_destroy(&ctx->tmp) && ret;
      
   ctx->cleanup = 0;      
   return ret;   
   
}

/*****************************************************************************/
bool 
   ria_parser_reset(     
      ria_parser_t*   IN OUT   ctx,
      const char*     IN       psrc,
      usize           IN       csrc,
      bool            IN       is_src_file,
      const char*     IN       ptype,
      usize           IN       ctype)
/*
 * Resets parser context
 *
 */
{

   typedef struct typ_s {
      const char*         pname;
      usize               cname;
      ria_parser_type_t   typ;
   } typ_t;

   usize i;

#define TYP(_n, _t) { _n, sizeof(_n)-1, _t }
   static const typ_t _types[] = 
   {
      TYP("json", ria_parser_json)
   };
#undef TYP   

   assert(ctx   != NULL);
   assert(ptype != NULL);
   assert(psrc  != NULL);

   if (!ria_parser_cleanup(ctx))
      return false;

   /*
    * Find proper parser type
    *
    */
   for (i=0; i<sizeof(_types)/sizeof(_types[0]); i++) {
      if (ctype != _types[i].cname)
         continue;
      if (!StrNICmp(_types[i].pname, ptype, ctype))
         break;
   }
   if (i == sizeof(_types)/sizeof(_types[0]))
      ERR_SET(err_bad_param);
      
   /*
    * Save source
    *
    */
   if (is_src_file) {
      if (!buf_detach(&ctx->src))
         return false;
      if (!buf_load(psrc, 0, csrc, &ctx->src))
         return false;
   }
   else 
      if (!buf_attach((char*)psrc, csrc, csrc, true, &ctx->src))
         return false;

   ctx->type = _types[i].typ;
   return true;

}

/*****************************************************************************/
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
      usize           IN       chint)
/*
 * Adds parsing rule
 *
 */
{

   ria_parser_rule_t* rule;
   byte* p;

   assert(pname  != NULL);
   assert(pbegin != NULL);
   assert(pend   != NULL);
   assert(phint  != NULL);

   /*
    * Check for duplicated rule
    *
    */
   if (!ria_parser_find_rule(&rule, ctx, pname, cname))
      return false;
   if (rule != NULL)
      ERR_SET(err_bad_param);    
   
   /*
    * Allocate new rule
    *
    */
   if (!heap_alloc(
           (void**)&p, 
           sizeof(ria_parser_rule_t)+cname+cbegin+cend+chint, 
           ctx->mem))
      return false;
   rule = (ria_parser_rule_t*)p;
   list_insert_tail(&ctx->rules, &rule->linkage);   
   rule->flags = 0;
   rule->pos   = 0;
   
   /*
    * Fill the rule
    *
    */
   p += sizeof(*rule);
   rule->pname = (char*)p;
   rule->cname = cname;
   MemCpy(p, pname, cname);
   p += cname;      
   rule->pbegin = (char*)p;
   rule->cbegin = cbegin;
   MemCpy(p, pbegin, cbegin);
   p += cbegin;      
   rule->pend = (char*)p;
   rule->cend = cend;
   MemCpy(p, pend, cend);
   p += cend;
   rule->phint = (char*)p;
   rule->chint = chint;
   MemCpy(p, phint, chint);
   return true;

}

/*****************************************************************************/
bool 
   ria_parser_find_rule(     
      ria_parser_rule_t**   OUT      rule,
      ria_parser_t*         IN OUT   ctx,
      const char*           IN       pname,
      usize                 IN       cname)
/*
 * Finds parsing rule
 *
 */
{

   list_entry_t* pl;
   
   assert(rule  != NULL);
   assert(ctx   != NULL);
   assert(pname != NULL);
   
   *rule = NULL;

   /*
    * Find rule by name
    *
    */
   list_for_each(&ctx->rules, &pl) {
      *rule = LIST_TO_RULE(pl);
      if ((*rule)->cname == cname)
         if (!StrNICmp((*rule)->pname, pname, cname))
            break;
      *rule = NULL;      
   }
   
   return true;

}




