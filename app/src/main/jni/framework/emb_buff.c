#include "emb_buff.h"

/******************************************************************************
 *   Memory block API
 */

/*****************************************************************************/
bool
   mem_blk_get_token(
      mem_blk_t*   IN OUT   dst, 
      mem_blk_t*   IN       src, 
      usize        IN       offset, 
      byte         IN       separator,
      bool         IN       skip_before,
      bool         IN       remove)
/* 
 * Gets token from memory block
 *
 */
{

   usize c;

   assert(dst != NULL);
   assert(src != NULL);

   /*
    * Sanity checks
    *
    */ 
   if (src->c <= offset)
      ERR_SET(err_bad_param);
   if ((offset != 0) && remove)
      ERR_SET(err_bad_param);
      
   dst->p = src->p + offset;
   dst->c = src->c - offset;

   /*
    * Skip whitespaces before if needed
    *
    */
   if (skip_before) 
      for (; dst->c>0; dst->c--, dst->p++) {
         switch (dst->p[0]) {
         case ' ' :
         case '\r':
         case '\n':
         case '\t':
            continue;
         }
         break;
      }  
    
   /*
    * Find end-of-token
    *
    */
   for (c=0; c<dst->c; c++) {
      if (separator == ' ') {
         switch (dst->p[c]) {
         case ' ' :
         case '\r':
         case '\n':
         case '\t':
            break;
         default:
            continue;
         }
      }  
      else 
         if (dst->p[c] != separator)
            continue;
      break;
   }
   dst->c = c;
   
   /*
    * Remove the token if reqired
    *
    */
   if (remove) { 
      if (dst->c == src->c) {
         src->p += dst->c;
         src->c  = 0;
      }
      else {
         if (dst->c > src->c)
            ERR_SET(err_internal);
         src->p += dst->c + 1;
         src->c -= dst->c + 1;
      }
   }
   return true;

}


/******************************************************************************
 *   Buffer API
 */

/*****************************************************************************/
#ifdef USE_DEBUG
bool
   buf_create_helper(
      usize         IN       catom,
      usize         IN       cbuf,
      umask         IN       flags,
      const char*   IN       file, 
      unumber       IN       line,
      buf_t*        IN OUT   buf,
      heap_ctx_t*   IN OUT   mem)
#else
bool
   buf_create_helper(
      usize         IN       catom,
      usize         IN       cbuf,
      umask         IN       flags,
      buf_t*        IN OUT   buf,
      heap_ctx_t*   IN OUT   mem)
#endif
/*
 * Creates buffer
 *
 */
{

   assert(buf != NULL);

   MemSet(buf, 0x00, sizeof(*buf));

   if ((mem == NULL) && (cbuf != 0))
      ERR_SET(err_internal);
   if (catom > buf_atom_mask)
      ERR_SET(err_bad_param);
   if (flags & (buf_atom_mask|buf_alloc_mask))
      ERR_SET(err_bad_param);

   buf->cdata = 0;
   buf->flags = flags | catom;
   buf->mem   = mem;

   if (cbuf != 0) {
#ifdef USE_DEBUG
      if (!heap_alloc_helper(
              &(buf->uptr.pbuf), 
              cbuf*catom, 
              file, 
              line,  
              mem))
#else
      if (!heap_alloc_helper(
              &(buf->uptr.pbuf), 
              cbuf*catom, 
              mem))
#endif
         return false;
      buf->flags |= cbuf << 8;
      return true;
   }
   else {
      buf->uptr.pbuf = NULL;
      return true;
   }

}

/*****************************************************************************/
bool
   buf_destroy(
      buf_t*   IN OUT   buf)
/*
 * Destroys buffer
 *
 */
{

   bool r;
   usize catom, cbuf;

   assert(buf != NULL);

   if (buf->uptr.pbuf != NULL) {

      catom = buf_get_atom_size(buf);
      cbuf  = buf_get_allocated_size(buf);

      if ((buf->flags & buf_secured) 
             && 
         ((buf->flags & buf_shared) == 0x00))
         MemSet(buf->uptr.pbuf, 0x00, cbuf*catom);

      if (((buf->flags & buf_attached) == 0x00) 
             && 
          (cbuf > 0)
             &&
          (buf->uptr.pbuf != NULL))
         r = heap_free(
                buf->uptr.pbuf, 
                buf->mem);
      else
         r = true;

   }
   else
      r = true;

   buf->uptr.pbuf = NULL;
   buf->cdata     = 0;
   buf->flags    &= ~(buf_alloc_mask|buf_attached|buf_shared);
   return r;

}

/*****************************************************************************/
bool 
   buf_set_length(                                            
      usize    IN       len,                            
      buf_t*   IN OUT   buf)                          
/*
 * Sets buffer length (in range 0...cbuf)
 *
 */
{

   usize cbuf;

   if (buf == NULL)
      ERR_SET(err_bad_param);
   if (len > buf_get_allocated_size(buf)) 
      ERR_SET(err_out_of_bounds);
   cbuf = buf_get_allocated_size(buf);

   buf->flags &= ~buf_alloc_mask;
   buf->flags |= (cbuf-len) << 8;
   buf->cdata  = len;
   return true;

}

/*****************************************************************************/
#ifdef USE_DEBUG
bool
   buf_load_helper(
      const void*   IN       pdata, 
      usize         IN       offset, 
      usize         IN       cdata,
      const char*   IN       file, 
      unumber       IN       line, 
      buf_t*        IN OUT   buf)
#else
bool
   buf_load_helper(
      const void*   IN       pdata, 
      usize         IN       offset, 
      usize         IN       cdata,
      buf_t*        IN OUT   buf)
#endif
/*
 * Loads atoms from raw buffer 
 *
 */
{

   usize c, catom, cbuf;

   assert((pdata != NULL) || (cdata == 0));
   assert(buf != NULL);

   c     = offset + cdata;
   catom = buf_get_atom_size(buf);
   cbuf  = buf_get_allocated_size(buf);

   if (c > cbuf) {
      if (((byte*)pdata >= buf->uptr.pbytes) 
            && 
         ((byte*)pdata < buf->uptr.pbytes+cbuf*catom))
         ERR_SET(err_invalid_pointer); 
#ifdef USE_DEBUG
      if (!buf_expand_helper(c, file, line, buf))
         return false;
#else
      if (!buf_expand_helper(c, buf))
         return false;
#endif
   }         
  
   MemMove(
      buf->uptr.pbytes+catom*offset, 
      pdata, 
      catom*cdata);
   return buf_set_length(c, buf);

}

/*****************************************************************************/
#ifdef USE_DEBUG
bool 
   buf_expand_helper(
      usize         IN       len, 
      const char*   IN       file,
      unumber       IN       line,
      buf_t*        IN OUT   buf)
#else
bool 
   buf_expand_helper(
      usize         IN       len, 
      buf_t*        IN OUT   buf)
#endif
/*
 * Expands buffer 
 *
 */
{

   usize catom, cbuf;
 
   assert(buf != NULL);

   catom = buf_get_atom_size(buf);
   cbuf  = buf_get_allocated_size(buf);

   if (len > cbuf) {

      usize cnewbuf, cval;
      byte* p;

      assert(buf->mem != NULL);

      if ((buf->flags & buf_attached) != 0)
         ERR_SET(err_unexpected_call);

      cnewbuf = (cbuf != 0) ? (cbuf << 1) : heap_get_unit(buf->mem);
      cval    = buf->cdata;

      if (buf->flags & buf_no_growth)
         cnewbuf = len;
      else
         for (; len>cnewbuf; cnewbuf<<=1)
            if (cnewbuf==0)
               ERR_SET(err_internal);
   
#ifdef USE_DEBUG
      if (!heap_alloc_helper((void**)&p, catom*cnewbuf, file, line, buf->mem))
         return false;
#else
      if (!heap_alloc_helper((void**)&p, catom*cnewbuf, buf->mem))
         return false;
#endif

      MemMove(p, buf->uptr.pbuf, buf->cdata*catom); 
      if (!buf_destroy(buf))
         return false;
            
      buf->uptr.pbuf = p; 
      buf->cdata     = cval;
      buf->flags    &= ~buf_alloc_mask; 
      buf->flags    |= (cnewbuf - cval) << 8;

   }

   return true;

}

/*****************************************************************************/
bool
   buf_fill(
      unumber   IN       sample, 
      usize     IN       offset, 
      usize     IN       cfill,
      buf_t*    IN OUT   buf)
/* 
 * Fills buffer with sample atom
 *
 */
{
   
   usize c;

   assert(buf != NULL);

   c = offset + cfill;
   if (!buf_expand(c, buf))
      return false;

   switch (buf_get_atom_size(buf)) {
   case sizeof(byte):
      for (; offset<c; offset++)
         buf->uptr.pbytes[offset]   = (byte)sample;
      break;
   case sizeof(uint16):
      for (; offset<c; offset++)
         buf->uptr.puints16[offset] = (uint16)sample;
      break;
   case sizeof(uint32):
      for (; offset<c; offset++)
         buf->uptr.puints32[offset] = (uint32)sample;
      break;
#ifdef PLATFORM_64BIT
   case sizeof(digit):
      for (; offset<c; offset++)
         buf->uptr.pdigits[offset] = (digit)sample;
      break;
#endif
   default:
      ERR_SET(err_unexpected_call);
   }

   if (c > buf->cdata)
      return buf_set_length(c, buf);
   else
      return true;

}

/*****************************************************************************/
bool
   buf_attach(
      void*    IN       pdata,
      usize    IN       cval,
      usize    IN       cbuf,
      bool     IN       shared,      
      buf_t*   IN OUT   buf)
/* 
 * Attaches raw buffer to buffer object
 *
 */
{

   assert(buf != NULL);

   if (cval > cbuf)
      ERR_SET(err_bad_param);    
   if (!buf_destroy(buf))
      return false;

   buf->uptr.pbuf = pdata; 
   buf->cdata     = cval; 
   buf->flags    &= ~buf_alloc_mask;
   buf->flags    |= ((cbuf - cval) << 8) | buf_attached; 
   
   if (shared)
      buf->flags |= buf_shared;
   else
      buf->flags &= ~buf_shared;
   return true;

}

/*****************************************************************************/
bool 
   buf_detach(                                                                
      buf_t*   IN OUT   buf)
/* 
 * Detaches raw buffer (if any) from buffer object
 *
 */
{

   assert(buf != NULL);

   if (!buf_destroy(buf))
      return false;
   buf->flags &= ~buf_attached;
   buf->flags &= ~buf_shared;
   return true;

}


/******************************************************************************
 *   Memory chunk list API
 */

#ifdef USE_MCL

/*
 * Memory chunk flags
 *
 */
enum {
   mem_chunk_attached = 0x01
};

/*
 * Memory chunk
 *
 */
typedef struct mem_chunk_s {
   list_entry_t   linkage;                       /* Linkage to list          */
   usize          cused;                         /* Used size, bytes         */
   usize          cfree;                         /* Free size, bytes         */
} mem_chunk_t;

/*
 * Casts list_entry_t to mem_chunk_t
 *
 */
#define CHUNK_FROM_LIST(_p)                                                   \
           ( (mem_chunk_t*)((byte*)(_p) - OFFSETOF(mem_chunk_t, linkage)) )

/*****************************************************************************/
static bool
   mem_chunk_destroy(
      mem_chunk_t*   IN OUT   chunk,
      heap_ctx_t*    IN OUT   mem)
/*
 * Destroys memory chunk
 *
 */
{
   assert(chunk != NULL);
   list_remove_entry_simple(&chunk->linkage);
   return heap_free(chunk, mem); 
}

/*****************************************************************************/
static bool
   mem_chunk_create(
      mem_chunk_t**   OUT      chunk,
      usize           IN       cbuf,
      heap_ctx_t*     IN OUT   mem)
/*
 * Creates memory chunk
 *
 */
{

   usize calloc;
   usize cunit;

   assert(chunk != NULL);

   /*
    * Get appropriate allocation size 
    *
    */
   calloc = cbuf + sizeof(mem_chunk_t);
   cunit  = heap_get_unit(mem);
   calloc = ((calloc + cunit - 1) / cunit) * cunit;
   if (!heap_alloc_in_range(
           (void*)chunk, 
           &calloc, 
           sizeof(mem_chunk_t)+1, 
           calloc, 
           mem))
      return false;

   list_init_entry(&((*chunk)->linkage));
   (*chunk)->cused = 0;
   (*chunk)->cfree = calloc - sizeof(mem_chunk_t);
   return true;

}

/*****************************************************************************/
bool
   mem_chunk_list_create(
      mem_chunk_list_t*   IN OUT   lst,
      heap_ctx_t*         IN OUT   mem)
/*
 * Creates empty list of memory chunks
 *
 */
{

   assert(lst != NULL);

   MemSet(lst, 0x00, sizeof(*lst));
   list_init_head(&lst->items);
   lst->mem   = mem;
   lst->flags = 0;
   return true;

}

/*****************************************************************************/
bool
   mem_chunk_list_destroy(
      mem_chunk_list_t*   IN OUT   lst)
/*
 * Destroys list of memory chunks 
 *
 */
{

   bool ret;

   assert(lst != NULL);

   ret = true;
   if ((lst->flags & mem_chunk_attached) == 0x00) 
      while (!list_is_empty(&lst->items)) {
         mem_chunk_t* p = CHUNK_FROM_LIST(list_next(&lst->items));
         ret = mem_chunk_destroy(p, lst->mem) && ret;
      }
   else {
      list_init_entry(&lst->items);
      lst->flags &= ~mem_chunk_attached;
   }

   return ret;

}

/*****************************************************************************/
bool
   mem_chunk_list_attach(
      mem_chunk_list_t*   IN OUT   lst,
      byte*               IN       pdata,
      usize               IN       cdata)
/*
 * Attaches given data to chunk list
 *
 */
{

   assert(lst   != NULL);
   assert(pdata != NULL);
   
   /*
    * Remove existing chunk list
    *
    */
   if (!mem_chunk_list_destroy(lst))
      return false;
   
   /*
    * Attach 
    *
    */
   lst->flags |= mem_chunk_attached;
   lst->items.next = (list_entry_t*)pdata;
   lst->items.prev = (list_entry_t*)cdata;
   return true;

}

#endif /* USE_MCL */

/*****************************************************************************/
bool
   mem_chunk_list_move(
      mem_chunk_list_t*   IN OUT   dst,
      mem_chunk_list_t*   IN OUT   src,
      bool                IN       to_head,
      usize               IN       bytes)
/*
 * Moves data from one chunk list to another 
 *
 */
{

#ifdef USE_MCL 

   usize i;

   assert(dst != NULL);
   assert(src != NULL);

   /*
    * Check for attached chunks
    *
    */
   if (dst->flags & mem_chunk_attached) 
      ERR_SET(err_bad_param);
   if (src->flags & mem_chunk_attached) {
      i = (bytes == 0) ? (usize)src->items.prev : bytes;
      if (!mem_chunk_list_push(
              dst, 
              (byte*)src->items.next, 
              i,
              to_head))
         return false;
      if (bytes == 0)
         return mem_chunk_list_destroy(src);
      else {
         bool ok;
         if (!mem_chunk_list_skip(&ok, bytes, src, true))
            return false;
         if (!ok)
            ERR_SET(err_internal);
         return true;
      }             
   }

   /*
    * Check for empty source
    *
    */
   if (list_is_empty(&src->items))
      return true;

   /*
    * Insert source list into destination
    *
    */
   if (!mem_chunk_list_get_size(&i, src))
      return false;
   if ((bytes == 0) || (i == bytes)) {
      if (to_head) {
         list_join_head(&dst->items, &src->items);
      }
      else {
         list_join_tail(&dst->items, &src->items);
      }
   }
   else {
      list_entry_t* p;
      for (p=&dst->items; bytes>0;) {
         mem_chunk_t* chunk = CHUNK_FROM_LIST(list_next(&src->items));
         list_remove_entry_simple(&chunk->linkage);
         if (to_head) {
            list_insert_after(p, &chunk->linkage);
            p = &chunk->linkage;
         }
         else
            list_insert_tail(&dst->items, &chunk->linkage);
         if (chunk->cused > bytes) {
            if (!mem_chunk_list_push(
                    src, 
                    (byte*)(chunk + 1) + bytes,
                    chunk->cused-bytes, 
                    true))
               return false;
            chunk->cfree += chunk->cused - bytes;
            chunk->cused  = bytes;
            break;
         }
         bytes -= chunk->cused;
      }
   }

   return true;

#else

   assert(dst != NULL);
   assert(src != NULL);

   if (dst->flags & buf_attached) 
      ERR_SET(err_bad_param);
   bytes = (bytes == 0) ? buf_get_length(src) : bytes;
   if (!buf_expand(buf_get_length(dst)+bytes, dst))
      return false;
   if (to_head) {
      MemMove(
         buf_get_ptr_bytes(dst)+bytes,
         buf_get_ptr_bytes(dst),
         buf_get_length(dst));
      MemMove(
         buf_get_ptr_bytes(dst),
         buf_get_ptr_bytes(src),
         bytes);
   }
   else 
      MemMove(
         buf_get_ptr_bytes(dst)+buf_get_length(dst),
         buf_get_ptr_bytes(src),
         bytes);
   if (!buf_set_length(buf_get_length(dst)+bytes, dst))
      return false;
   if (bytes != buf_get_length(src)) {
      MemMove(
         buf_get_ptr_bytes(src),
         buf_get_ptr_bytes(src)+bytes,
         buf_get_length(src)-bytes);
      return buf_set_length(buf_get_length(src)-bytes, src);
   }
   else
      return buf_destroy(src);

#endif

}

/*****************************************************************************/
bool
   mem_chunk_list_reuse(
      mem_chunk_list_t*   IN OUT   lst,
      usize               IN       cbuf)
/*
 * Reuses old list (destroys & pre-allocates required number of bytes)
 *
 */
{

#ifdef USE_MCL

   mem_chunk_t*  chunk;
   list_entry_t* p;

   assert(lst != NULL);

   /*
    * If attached, destroy old contents and create new chunks
    *
    */
   if (lst->flags & mem_chunk_attached) {
      if (!mem_chunk_list_destroy(lst))
         return false;
      return mem_chunk_list_pre_alloc(lst, cbuf, false);
   }         

   /*
    * Reuse existing chunks
    *
    */
   for (p=list_next(&lst->items); (cbuf>0)&&(p!=&lst->items); p=list_next(p)) {
      chunk = CHUNK_FROM_LIST(p);
      chunk->cfree += chunk->cused;
      chunk->cused  = 0;
      cbuf = (cbuf > chunk->cfree) ? cbuf-chunk->cfree : 0; 
   }
   if (cbuf == 0) 
      for (; p!=&lst->items; ) {
         chunk = CHUNK_FROM_LIST(p);
         p = list_next(p);
         if (!mem_chunk_destroy(chunk, lst->mem))
            return false;
      }

   /*
    * Allocate if required
    *
    */
   while (cbuf > 0) {
      if (!mem_chunk_create(&chunk, cbuf, lst->mem)) 
         return false;
      list_insert_tail(&lst->items, &chunk->linkage);
      cbuf = (chunk->cfree > cbuf) ? 0 : cbuf-chunk->cfree;
   }
   return true;

#else
   buf_set_empty(lst);
   return mem_chunk_list_pre_alloc(lst, cbuf, false);
#endif

}

/*****************************************************************************/
bool
   mem_chunk_list_pre_alloc(
      mem_chunk_list_t*   IN OUT   lst,
      usize               IN       cbuf,
      bool                IN       to_head)
/*
 * Pre-allocates required number of bytes in list
 *
 */
{

#ifdef USE_MCL

   mem_chunk_t*  chunk;
   list_entry_t* p;

   assert(lst != NULL);

   /*
    * If attached, destroy old contents
    *
    */
   if (lst->flags & mem_chunk_attached)
      if (!mem_chunk_list_destroy(lst))
         return false;

   /*
    * Adjust from existing chunk
    *
    */
   if (to_head) 
      for (p=list_next(&lst->items); p!=&lst->items; p=list_next(p)) {
         chunk = CHUNK_FROM_LIST(p);
         if (chunk->cused != 0)
            break;
         cbuf = (cbuf > chunk->cfree) ? cbuf-chunk->cfree : 0;
      }
   else 
      for (p=list_prev(&lst->items); p!=&lst->items; p=list_prev(p)) {
         chunk = CHUNK_FROM_LIST(p);
         if (chunk->cused != 0)
            break;
         cbuf = (cbuf > chunk->cfree) ? cbuf-chunk->cfree : 0;
      }
   if (cbuf == 0)
      return true;

   /*
    * Allocation loop
    *
    */
   while (cbuf > 0) {
      if (!mem_chunk_create(&chunk, cbuf, lst->mem)) 
         return false;
      if (to_head) {
         list_insert_head(&lst->items, &chunk->linkage);
      }
      else {
         list_insert_tail(&lst->items, &chunk->linkage);
      }
      cbuf = (chunk->cfree > cbuf) ? 0 : cbuf-chunk->cfree;
   }
   return true;

#else
   UNUSED(to_head);
   UNUSED(cbuf);
   UNUSED(lst);
   return true;
#endif

}

/*****************************************************************************/
bool
   mem_chunk_list_acquire_block(
      mem_blk_t*          OUT      blk,
      mem_chunk_list_t*   IN OUT   lst,
      usize               IN       cbuf,
      bool                IN       to_head)
/*
 * Acquires new block of memory from list
 *
 */
{

#ifdef USE_MCL

   mem_chunk_t*  chunk;
   list_entry_t* p;

   assert(lst != NULL);
   assert(blk != NULL);

   /*
    * If attached, destroy old contents
    *
    */
   if (lst->flags & mem_chunk_attached)
      if (!mem_chunk_list_destroy(lst))
         return false;

   /*
    * Try to use existing chunks
    *
    */
   if (to_head) 
      for (p=list_next(&lst->items); p!=&lst->items; p=list_next(p)) {
         chunk = CHUNK_FROM_LIST(p);
         if (chunk->cused != 0) {
            p = list_prev(p);
            break;
         }   
      }
   else 
      for (p=list_prev(&lst->items); p!=&lst->items; p=list_prev(p)) {
         chunk = CHUNK_FROM_LIST(p);
         if (chunk->cused != 0) {
            p = list_next(p);
            break;
         }   
      }
   if (p != &lst->items) {
      chunk = CHUNK_FROM_LIST(p);
      goto exit;
   }   

   /*
    * Allocation 
    *
    */
   if (!mem_chunk_create(&chunk, cbuf, lst->mem)) 
      return false;
   if (to_head) {
      list_insert_head(&lst->items, &chunk->linkage);
   }
   else {
      list_insert_tail(&lst->items, &chunk->linkage);
   }

exit:
   blk->p = (byte*)(chunk + 1);
   blk->c = chunk->cfree;
   return true;

#else

   assert(lst != NULL);
   assert(blk != NULL);
   
   if (!buf_expand(cbuf+buf_get_length(lst), lst))
      return false;
   if (to_head) {
      blk->p = buf_get_ptr_bytes(lst);   
      blk->c = cbuf;
      if (!buf_load(blk->p+blk->c, 0, buf_get_length(lst), lst))
         return false;
   }
   else {
      blk->p = buf_get_ptr_bytes(lst) + buf_get_length(lst);
      blk->c = cbuf;
   }
   return true;
   
#endif

}

/*****************************************************************************/
bool
   mem_chunk_list_release_block(
      mem_chunk_list_t*   IN OUT   lst,
      const mem_blk_t*    IN       blk,
      usize               IN       cdata)
/*      
 * Releases previously acquired block of memory 
 *
 */
{

#ifdef USE_MCL

   mem_chunk_t*  chunk;
   list_entry_t* p;

   assert(lst != NULL);
   assert(blk != NULL);

   /*
    * Find proper chunk
    *
    */
   for (p=list_next(&lst->items); p!=&lst->items; p=list_next(p)) {
      chunk = CHUNK_FROM_LIST(p);
      if ((byte*)(chunk+1) != blk->p) 
         continue;
      if (chunk->cused+chunk->cfree != blk->c)
         ERR_SET(err_bad_param);
      if (blk->c < cdata)
         ERR_SET(err_bad_param);
      if (cdata == 0)
         return mem_chunk_destroy(chunk, lst->mem);
      chunk->cfree -= cdata - chunk->cused;
      chunk->cused  = cdata;      
      break;
   }   
   
   if (p == &lst->items) 
      ERR_SET(err_bad_param);
   return true;

#else

   assert(lst != NULL);
   assert(blk != NULL);
   
   if (blk->c < cdata)
      ERR_SET(err_bad_param);
      
   if (blk->p == buf_get_ptr_bytes(lst)+buf_get_length(lst)) {
      if (blk->c+buf_get_length(lst) > buf_get_allocated_size(lst))
         ERR_SET(err_bad_param);
      if (!buf_set_length(cdata+buf_get_length(lst), lst))
         return false;
   }
   else {
      if (blk->p != buf_get_ptr_bytes(lst)) 
         ERR_SET(err_bad_param);
      if ((cdata > buf_get_length(lst)) || (blk->c > buf_get_length(lst)))
         ERR_SET(err_bad_param);
      if (cdata < blk->c) {
         MemMove(
            buf_get_ptr_bytes(lst)+cdata,
            buf_get_ptr_bytes(lst)+blk->c,
            buf_get_length(lst)-blk->c);
         if (!buf_set_length(buf_get_length(lst)-blk->c+cdata, lst))
            return false;   
      }            
   }
   return true;
   
#endif

}

/*****************************************************************************/
bool
   mem_chunk_list_push(
      mem_chunk_list_t*   IN OUT   lst,
      const byte*         IN       pdata,
      usize               IN       cdata,
      bool                IN       to_head)
/*
 * Pushes given data into list 
 *
 */
{

#ifdef USE_MCL

   byte* pbuf;
   usize cbuf;
   list_entry_t* p;
   list_entry_t* q;
   mem_chunk_t*  chunk;

   assert(lst != NULL);

   /*
    * If attached, destroy old contents
    *
    */
   if (lst->flags & mem_chunk_attached)
      if (!mem_chunk_list_destroy(lst))
         return false;

   /*
    * Push loop
    *
    */
   for (;;) {

      /*
       * Find first free chunk 
       *
       */
      p = &lst->items;
      if (to_head) 
         for (q=list_next(&lst->items); q!=&lst->items; q=list_next(q)) {
            chunk = CHUNK_FROM_LIST(q);
            if (chunk->cused != 0) 
               break;
            p = q;
         }
      else
         for (q=list_prev(&lst->items); q!=&lst->items; q=list_prev(q)) {
            chunk = CHUNK_FROM_LIST(q);
            if (chunk->cfree == 0) 
               break;
            p = q;
            if (chunk->cused != 0) 
               break;
         }

      /*
       * If there are preallocated space, use it
       *
       */
      if (to_head) 
         for (; p!=&lst->items; p=list_prev(p)) {
            chunk = CHUNK_FROM_LIST(p);
            if (cdata == 0)
               break;
            if (chunk->cused != 0)
               ERR_SET(err_internal);
            pbuf = (byte*)(chunk + 1) + chunk->cused;
            cbuf = (chunk->cfree > cdata) ? cdata : chunk->cfree;
            MemCpy(pbuf, pdata+cdata-cbuf, cbuf);
            chunk->cused += cbuf;
            chunk->cfree -= cbuf;
            cdata -= cbuf;
         }
      else
         for (; p!=&lst->items; p=list_next(p)) {
            chunk = CHUNK_FROM_LIST(p);
            if (cdata == 0)
               break;
            pbuf = (byte*)(chunk + 1) + chunk->cused;
            cbuf = (chunk->cfree > cdata) ? cdata : chunk->cfree;
            MemCpy(pbuf, pdata, cbuf);
            chunk->cused += cbuf;
            chunk->cfree -= cbuf;
            pdata += cbuf;
            cdata -= cbuf;
         }

      /*
       * If there are remaining data to push, preallocate and repeat
       *
       */
      if (cdata == 0)
         break;
      if (!mem_chunk_list_pre_alloc(lst, cdata, to_head))
         return false;

   }
  
   return true;

#else

   assert(lst != NULL);
   
   if (!buf_expand(buf_get_length(lst)+cdata, lst))
      return false;
   if (to_head) {
      MemMove(
         buf_get_ptr_bytes(lst)+cdata,
         buf_get_ptr_bytes(lst),
         buf_get_length(lst));
      MemMove(
         buf_get_ptr_bytes(lst),
         pdata,
         cdata);
   }
   else
      MemMove(
         buf_get_ptr_bytes(lst)+buf_get_length(lst),
         pdata,
         cdata);
   return buf_set_length(buf_get_length(lst)+cdata, lst);

#endif

}

/*****************************************************************************/
bool
   mem_chunk_list_get(
      bool*               OUT      ok,
      byte*               IN OUT   pbuf,
      usize               IN       cbuf,
      mem_chunk_list_t*   IN OUT   lst,
      bool                IN       from_head,
      bool                IN       remove)
/*
 * Gets requested number of bytes from list 
 *
 */
{

#ifdef USE_MCL

   byte* pdata;
   usize cdata;
   mem_chunk_t*  chunk;
   list_entry_t* p;

   assert(lst != NULL);
   assert(ok  != NULL);

   *ok = (cbuf == 0) ? true : false;

   /*
    * Check available data
    *
    */
   if (cbuf == 0) 
      return true;
   if (!mem_chunk_list_get_size(&cdata, lst))
      return false;
   if (cbuf > cdata)
      return true;

   /*
    * If attached, copy data
    *
    */
   if (lst->flags & mem_chunk_attached) {
      *ok = true;
      if (pbuf != NULL) {
         if (from_head)
            MemCpy(pbuf, lst->items.next, cbuf);
         else
            MemCpy(
               pbuf, 
               (byte*)lst->items.next+(usize)lst->items.prev-cbuf, 
               cbuf);
      }
      if (remove) {
         lst->items.prev = (list_entry_t*)(cdata - cbuf);
         if ((usize)lst->items.prev == 0)
            return mem_chunk_list_destroy(lst);
         if (from_head)
            lst->items.next = (list_entry_t*)((byte*)lst->items.next + cbuf);
      }
      return true;
   }

   /*
    * Pop loop
    *
    */
   for (p=(from_head)?list_next(&lst->items):list_prev(&lst->items); 
        cbuf>0; ) {

      /*
       * Take current chunk 
       *
       */
      if (p == &lst->items)
         return true;      
      chunk = CHUNK_FROM_LIST(p);
      cdata = (chunk->cused > cbuf) ? cbuf : chunk->cused;
      pdata = (byte*)(chunk + 1);
      if (pbuf != NULL) {
         if (from_head) {
            MemCpy(pbuf, pdata, cdata);
            pbuf += cdata;
         }
         else
            MemCpy(
               pbuf+cbuf-cdata, 
               pdata+chunk->cused-cdata, 
               cdata);
      }
      cbuf -= cdata;

      /*
       * Remove if chunk exhausted
       *
       */
      p = (from_head) ? list_next(p) : list_prev(p);
      if (!remove) 
         continue;
      if (cdata == chunk->cused) {
         if (!mem_chunk_destroy(chunk, lst->mem))
            return false;
      }
      else {
         if (from_head) 
            MemMove(pdata, pdata+cdata, chunk->cused-cdata);
         chunk->cused -= cdata;
         chunk->cfree += cdata;
      }

   }

   *ok = true;
   return true;

#else

   assert(lst != NULL);
   assert(ok  != NULL);

   *ok = false;
   if (buf_get_length(lst) < cbuf)
      return true;
      
   if (pbuf != NULL) {
      if (from_head)
         MemMove(pbuf, buf_get_ptr_bytes(lst), cbuf);
      else
         MemMove(
            pbuf, 
            buf_get_ptr_bytes(lst)+buf_get_length(lst)-cbuf, 
            cbuf);
   }
   if (remove) {
      if (from_head)
         MemMove(
            buf_get_ptr_bytes(lst), 
            buf_get_ptr_bytes(lst)+cbuf, 
            buf_get_length(lst)-cbuf);
      *ok = true;
      if (buf_get_length(lst) != cbuf)
         return buf_set_length(buf_get_length(lst)-cbuf, lst);
      else
         return buf_destroy(lst);
   }

   *ok = true;
   return true;
  
#endif

}

/*****************************************************************************/
#ifdef USE_MCL
bool
   mem_chunk_list_get_size(
      usize*                    OUT   cdata,
      const mem_chunk_list_t*   OUT   lst)
/*
 * Returns size of chunk list data 
 *
 */
{

   list_entry_t* p;

   assert(cdata != NULL);
   assert(lst   != NULL);

   /*
    * If attached, fetch the size
    *
    */
   if (lst->flags & mem_chunk_attached) {
      *cdata = (usize)lst->items.prev;
      return true;
   }

   /*
    * Calc size by list
    *
    */
   *cdata = 0;
   list_for_each(&lst->items, &p) {
      *cdata += CHUNK_FROM_LIST(p)->cused;
   }
   return true;

}
#endif

/*****************************************************************************/
bool
   mem_chunk_list_iterate(
      bool*                     OUT      ok, 
      mem_blk_t*                OUT      blk,
      handle*                   IN OUT   iterator,
      const mem_chunk_list_t*   IN OUT   lst)
/*
 * Iterates through chunks
 *
 */
{

#ifdef USE_MCL

   mem_chunk_t*  chunk;
   list_entry_t* p;

   assert(blk      != NULL);
   assert(lst      != NULL);
   assert(iterator != NULL);
   assert(ok       != NULL);

   *ok = false;

   /*
    * If attached, fetch the size
    *
    */
   if (lst->flags & mem_chunk_attached) {
      if (*iterator != NULL)
         return true;
      *iterator = (handle)&lst->items;
      blk->p    = (byte*)lst->items.next;
      blk->c    = (usize)lst->items.prev;
      *ok       = true;
      return true;
   }

   /*
    * If empty return null
    *
    */
   if (list_is_empty(&lst->items)) 
      return true;

   /*
    * Check iterator
    *
    */
   if (*iterator == NULL) 
      p = list_next(&lst->items);
   else {
      if (*iterator == &lst->items)
         return true;
      list_for_each(&lst->items, &p) {
         if (p == *iterator)
            break;
      }
      if (p == &lst->items)
         ERR_SET(err_bad_param);
   }       

   /*
    * Fetch data
    *
    */
   *ok    = true;
   chunk  = CHUNK_FROM_LIST(p);
   blk->p = (byte*)(chunk + 1);
   blk->c = chunk->cused;

   /*
    * Advance iterator
    *
    */
   *iterator = list_next(p);
   return true;

#else

   assert(blk      != NULL);
   assert(lst      != NULL);
   assert(iterator != NULL);
   assert(ok       != NULL);
   
   *ok = false;
   if (*iterator != NULL)
      return true;
   if (buf_get_length(lst) == 0)
      return true;

   blk->p    = buf_get_ptr_bytes(lst);
   blk->c    = buf_get_length(lst);
   *iterator = (handle)lst;
   *ok       = true;
   return true;

#endif

}

/*****************************************************************************/
bool
   mem_chunk_list_find_byte(
      usize*                    OUT   found, 
      usize                     IN    offset,
      byte                      IN    sample,
      const mem_chunk_list_t*   IN    lst)
/*
 * Searches for first entry of specified byte in chunk list
 *
 */
{

   bool  ok;
   usize i;
   mem_blk_t data;
   handle iterator;
   
   assert(found != NULL);

   /*
    * Scan chunk by chunk
    *
    */
   for (iterator=NULL, *found=0;; *found+=data.c) {
      if (!mem_chunk_list_iterate(&ok, &data, &iterator, lst))
         return false;
      if (!ok)
         break;
      if (offset != 0)  
         if (offset >= data.c) {
            offset -= data.c;
            continue;
         }
      for (i=offset, offset=0; i<data.c; i++)
         if (data.p[i] == sample)
            break;
      if (i < data.c) {
         *found += i;
         return true;
      }      
   }

   /*
    * Not found
    *
    */
   *found = NOT_FOUND_MARKER; 
   return true;

}

/*****************************************************************************/
#ifdef USE_DEBUG
bool
   mem_chunk_list_dump(
      mem_chunk_list_t*   IN OUT   lst)
/*
 * Dumps memory chunk list
 *
 */
{

   bool      ok;
   mem_blk_t data;
   handle    iterator;

   for (iterator=NULL;;) {
      if (!mem_chunk_list_iterate(&ok, &data, &iterator, lst))
         return false;
      if (!ok)
         break;
      DUMP_FILE(Stdout, data.p, data.c, 4); 
   }

   return true;

}
#endif

/*****************************************************************************/
bool
   buf_to_mem_chunk_list(
      mem_chunk_list_t*   IN OUT   dst,
      buf_t*              IN OUT   src,
      bool                IN       append,
      bool                IN       copy)
/*
 * Converts buffer to memory chunk list
 *
 */
{

#ifdef USE_MCL

   assert(dst != NULL);
   assert(src != NULL);

   if (!append)
      if (!mem_chunk_list_destroy(dst))
         return false;
   if (!mem_chunk_list_push(
           dst, 
           src->uptr.pbytes,
           src->cdata,
           false))
      return false;

   if (!copy)
      if (!buf_destroy(src))
         return false;
   return true;

#else

   assert(dst != NULL);
   assert(src != NULL);

   if (append || copy) 
      return mem_chunk_list_push(
                dst, 
                buf_get_ptr_bytes(src), 
                buf_get_length(src),
                false);
   if (!buf_destroy(dst))
      return false;

   dst->cdata       = src->cdata;
   dst->flags       = src->flags;
   dst->uptr        = src->uptr;     
   dst->mem         = src->mem;
   src->cdata       = 0;
   src->flags      &= ~(buf_alloc_mask|buf_attached|buf_shared);
   src->uptr.pbytes = NULL;
   return true;

#endif

}

/*****************************************************************************/
bool
   buf_from_mem_chunk_list(
      buf_t*              IN OUT   dst,
      mem_chunk_list_t*   IN OUT   src,
      bool                IN       append,
      bool                IN       copy)
/*
 * Converts memory chunk list to buffer
 *
 */
{

#ifdef USE_MCL

   usize count;
   byte* p;
   usize c;
   bool  ok;

   if (!mem_chunk_list_get_size(&count, src))
      return false;
   c = (append) ? dst->cdata+count : count;
   if (!buf_expand(c, dst))
      return false;
   p = (append) ? dst->uptr.pbytes+dst->cdata : dst->uptr.pbytes;
   if (!mem_chunk_list_get(&ok, p, count, src, true, !copy))
      return false;
   if (!ok)
      ERR_SET(err_internal);

   return buf_set_length(c, dst);

#else
   return buf_to_mem_chunk_list(dst, src, append, copy);
#endif

}


