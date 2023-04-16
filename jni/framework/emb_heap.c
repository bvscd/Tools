#include "emb_heap.h"
#include "emb_buff.h"

#ifdef USE_POOL

/******************************************************************************
 *   Pool support
 */

/*
 * Pool context
 *
 */
typedef struct pool_ctx_s {                 
   list_entry_t   linkage;                  /* Linkage to pool list          */
   uint32*        mask;                     /* Allocation mask               */
   usize          chunk;                    /* Chunk size, bytes             */
   usize          qty;                      /* Pool size. chunks             */
   void*          pool;                     /* Pool itself                   */
} pool_ctx_t;

/*
 * Casts list_entry_t to pool_ctx_t
 *
 */
#define POOL_FROM_LIST(_p)                                                    \
           ( (pool_ctx_t*)((byte*)(_p) - OFFSETOF(pool_ctx_t, linkage)) )

#ifdef USE_DEBUG
static bool
   heap_alloc_helper_internal(
      void**        OUT      ppbuf,
      usize         IN       cbuf,
      const char*   IN       file,
      unumber       IN       line,
      bool          IN       lock,
      void*         IN       psrc,
      usize         IN       csrc,
      heap_ctx_t*   IN OUT   ctx);
#else
static bool
   heap_alloc_helper_internal(
      void**        OUT      ppbuf,
      usize         IN       cbuf,
      bool          IN       lock,
      void*         IN       psrc,
      usize         IN       csrc,
      heap_ctx_t*   IN OUT   ctx);
#endif

/*****************************************************************************/
static void
   pool_estimate_size(
      usize*   OUT      mask_size,
      usize*   OUT      pool_size,
      usize    IN       chunk,
      usize*   IN OUT   qty)
/*
 * Estimates size of memory for pool
 *
 */
{

   usize i;

   assert(mask_size != NULL);
   assert(pool_size != NULL);
   assert(qty       != NULL);

   /*
    * Calculate proper numbers
    *
    */
   i          = (*qty + 31) / 32;
   *qty       = i * 32;
   *mask_size = i * sizeof(uint32);
   *pool_size = *qty * chunk;

}

/*****************************************************************************/
static bool
   pool_is_empty(
      const pool_ctx_t*   IN   pool)
/*
 * Checks pool for emptiness
 *
 */
{

   uint32* p;

   assert(pool != NULL);

   /*
    * Check the mask
    *
    */
   for (p=pool->mask; p<(uint32*)pool->pool; p++)
      if (*p != (uint32)-1)
         break;
   return (p < (uint32*)pool->pool) ? false : true;

}

/*****************************************************************************/
static bool
   pool_alloc(
      bool*         OUT   ok,
      void**        OUT   pp,
      pool_ctx_t*   IN    pool)
/*
 * Allocates free element
 *
 */
{

   uint32* p;
   uint32  u;
   usize   i;   

   assert(pool != NULL);
   assert(pp   != NULL);
   assert(ok   != NULL);

   *ok = false;

   /*
    * Find cell with vacant bit(s)
    *
    */
   for (p=pool->mask; p<(uint32*)pool->pool; p++)
      if (*p != 0x00)
         break;
   if (p > (uint32*)pool->pool)
      ERR_SET(err_internal);
   if (p == (uint32*)pool->pool) 
      return true;

   /*
    * Find vacant bit
    *
    */
   u = *p;
   i = 0;
   if ((u & 0xFFFF) == 0x0000)
      u >>= 16, 
      i  += 16;
   if ((u & 0x00FF) == 0x0000)
      u >>= 8, 
      i  += 8;
   if ((u & 0x000F) == 0x0000)
      u >>= 4, 
      i  += 4;
   if ((u & 0x0003) == 0x0000)
      u >>= 2, 
      i  += 2;
   if ((u & 0x0001) == 0x0000)
      i++;

   /*
    * Acquire bit
    *
    */
   *p &= ~((uint32)1 << i);
   i  += (p - pool->mask)*32;

   /*
    * Return chunk
    *
    */
   *pp = (byte*)pool->pool + i*pool->chunk;
   *ok = true;
   return true;

}

/*****************************************************************************/
static bool
   pool_free(
      void*         IN   p, 
      pool_ctx_t*   IN   pool)
/*
 * Releases chunk
 *
 */
{

   usize i, j;

   assert(pool != NULL);

   /*
    * Sanity check
    *
    */
   if (p < pool->pool)
      ERR_SET(err_bad_param);
   if ((byte*)p >= (byte*)pool->pool+pool->qty*pool->chunk)
      ERR_SET(err_bad_param);

   /*
    * Check bit vacancy
    *
    */
   i = (byte*)p - (byte*)pool->pool;
   if ((i%pool->chunk) != 0)
      ERR_SET(err_bad_param);
   i /= pool->chunk; 
   j  = i >> 5; /* / 32 */
   i &= 0x1F;   /* % 32 */
   if ((pool->mask[j] & ((uint32)1 << i)) != 0x00)
      ERR_SET(err_bad_param);

   /*
    * Ok, thus release the bit 
    *
    */
   pool->mask[j] |= ((uint32)1 << i);
   return true;

}

/*****************************************************************************/
static bool
   heap_find_pool(
      pool_ctx_t**   OUT      pool,
      usize          IN       chunk,
      usize          IN       qty,
      heap_ctx_t*    IN OUT   mem)
/*
 * Searches for proper pool and creates it if not found
 *
 */
{

   usize ms, ps;
   list_entry_t* p;

   assert(pool != NULL);
   assert(mem  != NULL);

   /*
    * Try to find needed pool
    *
    */
   list_for_each(&mem->pools, &p) {
      pool_ctx_t* pp = POOL_FROM_LIST(p);
      if (pp->chunk == chunk) {
         *pool = pp;
         return true;
      }         
   }

   /*
    * Try to create new pool
    *
    */
   pool_estimate_size(&ms, &ps, chunk, &qty);

#ifdef USE_DEBUG
   if (!heap_alloc_helper_internal(
            (void**)pool, 
            sizeof(pool_ctx_t)+ms+ps,
            __FILE__, 
            __LINE__, 
            false,
            mem->pbuf,
            mem->cbuf,
            mem)) {
#else
   if (!heap_alloc_helper_internal(
            (void**)pool,
            sizeof(pool_ctx_t)+ms+ps, 
            false, 
            mem->pbuf,
            mem->cbuf,
            mem)) {
#endif
      *pool = NULL;
      return true;
   }
   
   MemSet(*pool, 0x00, sizeof(pool_ctx_t));

   /*
    * Initialize pool
    *
    */
   list_init_entry(&(*pool)->linkage);
   (*pool)->mask  = (uint32*)(*pool + 1);
   (*pool)->pool  = (byte*)((*pool)->mask) + ms;
   (*pool)->chunk = chunk;
   (*pool)->qty   = qty;

   /*
    * Initialize the mask
    *
    */
   MemSet((*pool)->mask, 0xFF, ms);

   /*
    * Pool created ok, attach it
    *
    */
   list_insert_tail(&mem->pools, &(*pool)->linkage);
   return true;

}

#endif /* USE_POOL */


/******************************************************************************
 *   Heap API
 */

/*
 * Heap block tag, describes continuous block of heap memory
 *
 */
typedef struct heap_tag_s {                 /* sizeof() == unit              */
   usize         cprev;                     /* Size of previous block, units */
   usize         cnext;                     /* Size of next block, units     */
#ifdef USE_DEBUG                            /* Allocation origin:            */
   const char*   file;                      /* File                          */
   unumber       line;                      /* Line                          */
#endif
} heap_tag_t;

/*
 * Heap block tag related constants
 *
 */
#define HT_MASK_BUSY ( HBIT_USIZE)
#define HT_MASK_OFFS (~HBIT_USIZE)
#define HT_MASK_LINE (~HBIT_USIZE)

#ifdef USE_MALLOC

/*
 * Allocation extension, used for malloc-ed memory blocks
 *
 */
typedef struct heap_ext_s {                 
   list_entry_t   linkage;                  /* List linkage                  */
   usize          cbuf;                     /* Attached buffer size, units   */
   void*          pbuf;                     /* Attached buffer               */
} heap_ext_t;

/*
 * Casts list_entry_t to heap_ext_t
 *
 */
#define EXT_FROM_LIST(_p)                                                   \
           ( (heap_ext_t*)((byte*)(_p) - OFFSETOF(heap_ext_t, linkage)) )

#endif

/*
 * Heap cleanup flags
 *
 */
enum {
   cf_mutex = 0x01
};

/*****************************************************************************/
static bool
   _heap_check_pointer(
      const void*   IN   ptr,
      const void*   IN   psrc,
      usize         IN   csrc)
/*
 * Checks pointer validity
 *
 */
{

   heap_tag_t* p;
   heap_tag_t* q;

   assert(psrc != NULL);

   p = (heap_tag_t*)psrc;
   if (((byte*)ptr-(byte*)psrc) % sizeof(heap_tag_t))
      ERR_SET(err_invalid_pointer);
   if (((heap_tag_t*)ptr < p+1) || ((heap_tag_t*)ptr >= p+csrc))
      ERR_SET(err_invalid_pointer);

#ifdef USE_DEBUG

   /*
    * Check heap & pointer
    *
    */

   for (q=p; 
        q<p+csrc; 
        q+=(q->cnext & HT_MASK_OFFS)+1)
      if (q+1 == ptr)
         break;
   if (q == p+csrc) 
      ERR_SET(err_heap_corrupted);
   if (q > p+csrc) 
      ERR_SET(err_heap_corrupted);

#endif

   q = (heap_tag_t*)ptr - 1;
   if ((q->cnext & HT_MASK_BUSY) == 0x00) 
      ERR_SET(err_invalid_pointer);
   return true;

}

/*****************************************************************************/
#ifdef USE_POOL
static bool
   _heap_check_block(
      pool_ctx_t**   OUT      pool,
      void*          IN       ptr,
      heap_ctx_t*    IN OUT   ctx)
#else
static bool
   _heap_check_block(
      void*          IN       ptr,
      heap_ctx_t*    IN OUT   ctx)
#endif
/*
 * Checks block pointer validity
 *
 */
{

#ifdef USE_POOL
   list_entry_t* pl;
#endif

   assert(ptr != NULL);
   assert(ctx != NULL);

#ifdef USE_POOL

   assert(pool != NULL);
   *pool = NULL;

   /*
    * May be from pool
    *
    */
   list_for_each(&ctx->pools, &pl) {
      pool_ctx_t* pp = POOL_FROM_LIST(pl);
      if (pp->pool > ptr) 
         continue;
      if ((byte*)pp->pool+pp->chunk*pp->qty <= (byte*)ptr) 
         continue;
      *pool = pp;
      return true;
   }

#endif

   if (!_heap_check_pointer(ptr, ctx->pbuf, ctx->cbuf)) {
#ifdef USE_MALLOC
      list_for_each(&ctx->exts, &pl) {
         heap_ext_t* ext = EXT_FROM_LIST(pl);
         if (_heap_check_pointer(ptr, ext->pbuf, ext->cbuf)) {
            ERR_SET_NO_RET(err_none);
            return true;
         }
      }
#endif
      return false;
   }
      
   return true;

}

#ifdef USE_POOL
#define heap_check_block(a, b, c) _heap_check_block(a, b, c)
#else
#define heap_check_block(a, b, c) _heap_check_block(b, c)
#endif

/*****************************************************************************/
static usize
   heap_get_max_free_block(
      heap_ctx_t* ctx)
/*
 * Returns size of maximum free block, internal use ONLY
 *
 */
{

   usize i, j, k;
   heap_tag_t* p;

   assert(ctx != NULL);

   p = (heap_tag_t*)ctx->pbuf;
   k = 0;

   for (i=ctx->cbuf;
        i>0;
        j&=HT_MASK_OFFS, i-=j+1, p+=j+1) {
      j = p->cnext;
      if (((j & HT_MASK_BUSY) == 0x00) && (k < j))
         k = j;
   }

   k *= sizeof(heap_tag_t);
   return k;

}

/*****************************************************************************/
bool
   heap_create(
      void*         IN       pbuf,
      usize         IN       cbuf,
      umask         IN       flags,
      heap_ctx_t*   IN OUT   ctx)
/*
 * Initializes heap context
 *
 */
{

   usize i;
   heap_tag_t* p;

   assert(pbuf != NULL);
   assert(ctx  != NULL);

   MemSet(ctx, 0x00, sizeof(*ctx));
#ifdef USE_MALLOC
   list_init_head(&ctx->exts);
#endif
#ifdef USE_POOL
   list_init_head(&ctx->pools);
#endif

   i     = sizeof(heap_tag_t) - (usize)pbuf % sizeof(heap_tag_t);
   cbuf -= i;
   pbuf  = (byte*)pbuf + i;

   if (cbuf <= sizeof(heap_tag_t))
      ERR_SET(err_bad_param);

   sync_mutex_create(&ctx->mutex);
   ctx->cleanup |= cf_mutex;

   ctx->cbuf  = cbuf / sizeof(heap_tag_t);
   ctx->pbuf  = pbuf;
   ctx->flags = flags;

   p = (heap_tag_t*)pbuf;
   p->cprev = 0;
   p->cnext = ctx->cbuf - 1;
   return true;

}

/*****************************************************************************/
bool
   heap_destroy(
      heap_ctx_t*   IN OUT   ctx)
/*
 * Destroys heap context
 *
 */
{

   assert(ctx != NULL);

#ifdef USE_MALLOC
   while (!list_is_empty(&ctx->exts)) {
      heap_ext_t* ext = EXT_FROM_LIST(list_next(&ctx->exts));
      list_remove_entry_simple(&ext->linkage);
      Free(ext);
   }
#endif
   if (ctx->cleanup & cf_mutex)
      sync_mutex_destroy(&ctx->mutex);

   MemSet(ctx, 0x00, sizeof(*ctx));
   return true;

}

/*****************************************************************************/
#ifdef USE_DEBUG
static bool
   heap_alloc_helper_internal(
      void**        OUT      ppbuf,
      usize         IN       cbuf,
      const char*   IN       file,
      unumber       IN       line,
      bool          IN       lock,
      void*         IN       psrc,
      usize         IN       csrc,
      heap_ctx_t*   IN OUT   ctx)
#else
static bool
   heap_alloc_helper_internal(
      void**        OUT      ppbuf,
      usize         IN       cbuf,
      bool          IN       lock,
      void*         IN       psrc,
      usize         IN       csrc,
      heap_ctx_t*   IN OUT   ctx)
#endif
/*
 * Allocates memory from heap, internal function
 *
 */
{

   usize i, j;
   heap_tag_t* p;
   heap_tag_t* q;
#ifdef USE_MALLOC
   usize corg = cbuf;
#endif

   /*
    * Allocation overhead 
    *
    */
   usize k;  

   /*
    * Fragmentation overhead 
    *
    */
   usize l;
   usize n;

   assert(ppbuf != NULL);
   assert(psrc  != NULL);
   assert(ctx   != NULL);

   if (lock) 
      sync_mutex_lock(&ctx->mutex);

   *ppbuf = NULL;
   cbuf   = (cbuf + sizeof(heap_tag_t) - 1) / sizeof(heap_tag_t);

   p = (heap_tag_t*)psrc; /* ctx->pbuf; */
   q = NULL;
   k = (usize)-1;
   l = 0;
   n = 0;

   for (i=csrc; /* ctx->cbuf; */
        i>0;
        j&=HT_MASK_OFFS, i-=j+1, p+=j+1) {
      j = p->cnext;
      if (((j & HT_MASK_BUSY) == 0x00) && (j >= cbuf)) {
         if (i > j+1)
            n += p[j+1].cnext;
         /* 
          * Original strategy
          * if (k > j-cbuf) 
          *
          */
         if ((k == (usize)-1) || ((j-cbuf+(3*n>>2)) < (k+(3*l>>2)))) {
            k = j - cbuf;
            l = n;
            q = p;
         }
      }
      else
         n = p->cnext;
         
   }

   if (q == NULL) {
#ifdef USE_MALLOC
      /*
       * If not malloc'ed buffer, find proper one or allocate
       *
       */
      if ((psrc == ctx->pbuf) && ((ctx->flags & heap_use_malloc) != 0)) {
         bool ret;
         usize calloc;
         list_entry_t* pl;
         heap_ext_t* ext;
         list_for_each(&ctx->exts, &pl) {
            ext = EXT_FROM_LIST(pl);
#ifdef USE_DEBUG
            ret = heap_alloc_helper_internal(
                     ppbuf, 
                     corg, 
                     file, 
                     line, 
                     false, 
                     ext->pbuf, 
                     ext->cbuf, 
                     ctx);
#else
            ret = heap_alloc_helper_internal(
                     ppbuf, 
                     corg, 
                     false, 
                     ext->pbuf, 
                     ext->cbuf, 
                     ctx);
#endif
            if (ret) {
               if (lock)
                  sync_mutex_unlock(&ctx->mutex);
               return true;
            }
         }   
         calloc = (cbuf > csrc-1) ? cbuf+1 : csrc;
         ext = Malloc(sizeof(heap_ext_t)+calloc*sizeof(heap_tag_t));
         if (ext != NULL) {
            list_init_entry(&ext->linkage);
            ext->pbuf = ext + 1;
            ext->cbuf = calloc;
            list_insert_tail(&ctx->exts, &ext->linkage); 
            p = (heap_tag_t*)ext->pbuf;
            p->cprev = 0;
            p->cnext = ext->cbuf - 1;
#ifdef USE_DEBUG
            ret = heap_alloc_helper_internal(
                     ppbuf, 
                     corg, 
                     file, 
                     line, 
                     false, 
                     ext->pbuf, 
                     ext->cbuf, 
                     ctx);
#else
            ret = heap_alloc_helper_internal(
                     ppbuf, 
                     corg, 
                     false, 
                     ext->pbuf, 
                     ext->cbuf, 
                     ctx);
#endif
            if (lock)
               sync_mutex_unlock(&ctx->mutex);
            return ret;
         }
      }
#endif
      if (lock)
         sync_mutex_unlock(&ctx->mutex);
#ifdef USE_DEBUG
      if (lock && ((ctx->flags & heap_no_trace) == 0)) {
         usize free, busy; 
         Fprintf(Stdout, "\nCalled from %s, line=%d\n", file, line);
         heap_stats(&free, &busy, Stdout, ctx);
         Fprintf(Stdout, "\nFree %d, busy %d, required %d\n", 
            (unumber)free, (unumber)busy, (unumber)(cbuf*sizeof(heap_tag_t)));
         Fflush(Stdout);
      }
#endif
      ERR_SET(err_no_memory);
   }

   i        = q->cnext;
   k        = (i-cbuf < 2) ? i : cbuf;
   *ppbuf   = (byte*)(q + 1);
   q->cnext = k | HT_MASK_BUSY;

   if (k != i) {
      q[k+1].cprev = k;
      q[k+1].cnext = i - k - 1;
      if (q+i+1 < (heap_tag_t*)psrc+csrc/*ctx->pbuf+ctx->cbuf*/)
         q[i+1].cprev = q[k+1].cnext;
   }

#ifdef USE_DEBUG
   q->file = file;
   q->line = line;
   ctx->alloc += (q->cnext & HT_MASK_OFFS) * sizeof(heap_tag_t);
   if (ctx->alloc > ctx->max_alloc) {
#if 0
      Fprintf(Stdout, "MA %d - %s %d\n", ctx->alloc, file, line);
#endif
      ctx->max_alloc = ctx->alloc;
   }
#endif

   if (lock)
      sync_mutex_unlock(&ctx->mutex);
   return true;

}

/*****************************************************************************/
#ifdef USE_DEBUG
bool
   heap_alloc_helper(
      void**        OUT      ppbuf,
      usize         IN       cbuf,
      const char*   IN       file,
      unumber       IN       line,
      heap_ctx_t*   IN OUT   ctx)
#else
bool
   heap_alloc_helper(
      void**        OUT      ppbuf,
      usize         IN       cbuf,
      heap_ctx_t*   IN OUT   ctx)
#endif
/*
 * Allocates memory from heap
 *
 */
{
#ifdef USE_DEBUG
   return heap_alloc_helper_internal(
             ppbuf, 
             cbuf, 
             file, 
             line, 
             true, 
             ctx->pbuf,
             ctx->cbuf,
             ctx);
#else
   return heap_alloc_helper_internal(
             ppbuf, 
             cbuf, 
             true, 
             ctx->pbuf,
             ctx->cbuf,
             ctx);
#endif
}

/*****************************************************************************/
#ifdef USE_DEBUG
bool
   heap_alloc_in_range_helper(
      void**        OUT      ppbuf,
      usize*        OUT      calloc,
      usize         IN       cmin,
      usize         IN       cmax,
      const char*   IN       file,
      unumber       IN       line,
      heap_ctx_t*   IN OUT   ctx)
#else
bool
   heap_alloc_in_range_helper(
      void**        OUT      ppbuf,
      usize*        OUT      calloc,
      usize         IN       cmin,
      usize         IN       cmax,
      heap_ctx_t*   IN OUT   ctx)
#endif
/*
 * Allocates amount of memory in specified range from heap
 *
 */
{

   usize c;
   bool  ret;
   
   assert(calloc != NULL);

   sync_mutex_lock(&ctx->mutex);

   /*
    * Determine available size
    *
    */
#ifdef USE_MALLOC
   if ((ctx->flags & heap_use_malloc) != 0) 
      *calloc = cmax;
   else {
#endif
      c = heap_get_max_free_block(ctx);
      if (c < cmin) {
#ifdef USE_DEBUG
         usize free, busy; 
#endif
         sync_mutex_unlock(&ctx->mutex);
#ifdef USE_DEBUG
         if ((ctx->flags & heap_no_trace) == 0) {
            Fprintf(Stdout, "\nCalled from %s, line=%d\n", file, line);
            heap_stats(&free, &busy, Stdout, ctx);
            Fprintf(Stdout, "\nFree %d, busy %d, required %d\n", 
               (unumber)free, (unumber)busy, (unumber)cmin);
            Fflush(Stdout);
         }
#endif
         ERR_SET(err_no_memory);
      }      
      *calloc = (c > cmax) ? cmax: c;
#ifdef USE_MALLOC
   }
#endif
 
#ifdef USE_DEBUG
   ret = heap_alloc_helper_internal(
            ppbuf, 
            *calloc, 
            file, 
            line, 
            false, 
            ctx->pbuf,
            ctx->cbuf,
            ctx);
#else
   ret = heap_alloc_helper_internal(
            ppbuf, 
            *calloc, 
            false, 
            ctx->pbuf,
            ctx->cbuf,
            ctx);
#endif

   sync_mutex_unlock(&ctx->mutex);
   return ret;

}

/*****************************************************************************/
#ifdef USE_POOL
bool
   heap_alloc_from_pool(
      void**        OUT      ppbuf,
      usize         IN       cbuf,
      usize         IN       qty,
      heap_ctx_t*   IN OUT   ctx)
/*
 * Allocates memory from pool of fixed size chunks  
 * 
 */
{

   pool_ctx_t* pool;
   bool ok;

   assert(ctx != NULL);

   sync_mutex_lock(&ctx->mutex);

   /*
    * Find pool, use pool if not found
    *
    */
   if (!heap_find_pool(&pool, cbuf, qty, ctx)) {
      sync_mutex_unlock(&ctx->mutex);
      return false;
   }
   if (pool == NULL) {
      sync_mutex_unlock(&ctx->mutex);
      return heap_alloc(ppbuf, cbuf, ctx);
   }

   /*
    * Alloc from pool, use heap if no space
    *
    */
   if (!pool_alloc(&ok, ppbuf, pool)) {
      sync_mutex_unlock(&ctx->mutex);
      return false;
   }
   if (!ok) {
      sync_mutex_unlock(&ctx->mutex);
      return heap_alloc(ppbuf, cbuf, ctx);
   }

   sync_mutex_unlock(&ctx->mutex);
   return true;

}
#endif

/*****************************************************************************/
static bool
   heap_free_helper(
      void*         IN       pbuf,
      void*         IN OUT   psrc,
      usize         IN       csrc,
      heap_ctx_t*   IN OUT   ctx)
/*
 * Returns memory to heap, helper function 
 *
 */
{

   heap_tag_t* p;
   heap_tag_t* q;
   heap_tag_t* t;

   assert(pbuf != NULL);
   assert(psrc != NULL);

   p = (heap_tag_t*)psrc;
   q = (heap_tag_t*)pbuf - 1;
   q->cnext &= HT_MASK_OFFS;
   if (q+q->cnext+1 > p+csrc) 
      ERR_SET(err_heap_corrupted);

#ifdef USE_DEBUG
   ctx->alloc -= q->cnext * sizeof(heap_tag_t);
#endif

   t = q + q->cnext + 1;
   if (t < p+csrc)
      if ((t->cnext & HT_MASK_BUSY) == 0x00) {
         if (q+q->cnext+t->cnext+2 > p+csrc) 
            ERR_SET(err_heap_corrupted);
         q->cnext += t->cnext + 1;
         if (q+q->cnext+1 < p+csrc)
            q[q->cnext+1].cprev = q->cnext;
      }

   t = q - q->cprev - 1;
   if (t >= p)
      if ((t->cnext & HT_MASK_BUSY) == 0x00) {
         if (t+q->cnext+t->cnext+2 > p+csrc) 
            ERR_SET(err_heap_corrupted);
         t->cnext += q->cnext + 1;
         if (t+t->cnext+1 < p+csrc)
            t[t->cnext+1].cprev = t->cnext;
      }

   return true;

}

/*****************************************************************************/
bool
   heap_free(
      void*         IN       pbuf,
      heap_ctx_t*   IN OUT   ctx)
/*
 * Returns memory to heap
 *
 */
{

   bool ret;
#ifdef USE_POOL
   pool_ctx_t* pool;
#endif

   assert(pbuf != NULL);
   assert(ctx  != NULL);

   sync_mutex_lock(&ctx->mutex);

   if (!heap_check_block(&pool, pbuf, ctx)) {
      sync_mutex_unlock(&ctx->mutex);
      return false;
   }

#ifdef USE_POOL
   /*
    * May be from pool
    *
    */
   if (pool != NULL) {
      ret = pool_free(pbuf, pool);
      if (ret && pool_is_empty(pool)) {
         list_remove_entry_simple(&pool->linkage);
         sync_mutex_unlock(&ctx->mutex);
         ret = heap_free(pool, ctx);
         return ret;
      }
      sync_mutex_unlock(&ctx->mutex);
      return ret;
   }
#endif

   ret = false;
   if (((heap_tag_t*)pbuf >= (heap_tag_t*)ctx->pbuf) && 
       ((heap_tag_t*)pbuf <  (heap_tag_t*)ctx->pbuf+ctx->cbuf)) 
      ret = heap_free_helper(pbuf, ctx->pbuf, ctx->cbuf, ctx);
#ifdef USE_MALLOC
   else {
      list_entry_t* pl;
      list_for_each(&ctx->exts, &pl) {
         heap_ext_t* ext = EXT_FROM_LIST(pl);
         if (((heap_tag_t*)pbuf >= (heap_tag_t*)ext->pbuf) && 
             ((heap_tag_t*)pbuf <  (heap_tag_t*)ext->pbuf+ext->cbuf)) {
            ret = heap_free_helper(pbuf, ext->pbuf, ext->cbuf, ctx);
            if (ret) {
               heap_tag_t* p = (heap_tag_t*)ext->pbuf;
               if (p->cnext == ext->cbuf - 1) {
                  /*
                   * Free heap extension cos it's unused 
                   *
                   */
                  list_remove_entry_simple(&ext->linkage);
                  Free(ext);
               }
            }
            break;
         }
      }
   }
#endif

   if (!ret)
      ERR_SET_NO_RET(err_internal);
   sync_mutex_unlock(&ctx->mutex);
   return ret;

}

/*****************************************************************************/
static bool
   heap_stats_helper(
      usize*        OUT      pfree,
      usize*        OUT      pbusy,
      FILE*         IN       out, 
      void*         IN       psrc,
      usize         IN       csrc,
      heap_ctx_t*   IN OUT   ctx)
/*
 * Returns heap statistics, helper
 *
 */
{

   heap_tag_t* p;
   heap_tag_t* q;
   heap_tag_t* t;

#ifndef USE_DEBUG
   UNUSED(out);
#endif

   assert(pfree != NULL);
   assert(pbusy != NULL);
   assert(psrc  != NULL);

   q = p = (heap_tag_t*)psrc;
   while (q < p+csrc) {

      if ((ctx->flags & heap_no_trace) == 0) {
         Fprintf(
            out,
            "$%p, use=%d, len=%d\n",
            (void*)q,
            (q->cnext & HT_MASK_BUSY) ? 1 : 0,
            (unumber)(q->cnext & HT_MASK_OFFS));
#ifdef USE_DEBUG
         if (q->cnext & HT_MASK_BUSY)
            Fprintf(
               out,
               "-> $%s, line=%d\n",
               q->file,
               (unumber)(q->line & HT_MASK_LINE));
#endif
      }

      if (q->cnext & HT_MASK_BUSY)
         *pbusy += q->cnext & HT_MASK_OFFS;
      else
         *pfree += q->cnext;

      t  = q;
      q += (q->cnext & HT_MASK_OFFS) + 1;
      if (q >= p+csrc)
         break;
      if (q-q->cprev-1 != t) 
         ERR_SET(err_heap_corrupted);

   }

   *pbusy *= sizeof(heap_tag_t);
   *pfree *= sizeof(heap_tag_t);

   if (q != p+csrc) 
      ERR_SET(err_heap_corrupted);
   return true;

}

/*****************************************************************************/
bool
   heap_stats(
      usize*        OUT      pfree,
      usize*        OUT      pbusy,
      FILE*         IN       out, 
      heap_ctx_t*   IN OUT   ctx)
/*
 * Returns heap statistics
 *
 */
{

   bool ret;
#ifndef USE_DEBUG
   UNUSED(out);
#endif

   assert(pfree != NULL);
   assert(pbusy != NULL);
   assert(ctx   != NULL);

   *pfree = 0;
   *pbusy = 0;

   sync_mutex_lock(&ctx->mutex);

   if ((ctx->flags & heap_no_trace)==0) {
      Fprintf(out, "Checking heap:\n");
   }

   ret = heap_stats_helper(pfree, pbusy, out, ctx->pbuf, ctx->cbuf, ctx);
#ifdef USE_MALLOC
   if (ret) {
      list_entry_t* pl;
      list_for_each(&ctx->exts, &pl) {
         heap_ext_t* ext = EXT_FROM_LIST(pl);
         ret = heap_stats_helper(pfree, pbusy, out, ext->pbuf, ext->cbuf, ctx);
         if (!ret)
            break;
      }
   }
#endif
   
   sync_mutex_unlock(&ctx->mutex);
   return ret;

}

/*****************************************************************************/
bool
   heap_is_pointer_within(
      void*               IN   ptr,
      const heap_ctx_t*   IN   ctx)
/*
 * Quick check of pointer validity
 *
 */
{

   heap_tag_t*   p;
#ifdef USE_MALLOC
   list_entry_t* pl;
#endif

   assert(ptr != NULL);
   assert(ctx != NULL);

   p = (heap_tag_t*)ctx->pbuf;
   if (((heap_tag_t*)ptr >= p+1) && ((heap_tag_t*)ptr < p+ctx->cbuf))
      return true;

#ifdef USE_MALLOC
   list_for_each(&ctx->exts, &pl) {
      heap_ext_t* ext = EXT_FROM_LIST(pl);
      p = (heap_tag_t*)ext->pbuf;
      if (((heap_tag_t*)ptr >= p+1) && ((heap_tag_t*)ptr < p+ext->cbuf))
         return true;
   }
#endif

   return false;

}

/*****************************************************************************/
usize
   heap_get_unit(
      heap_ctx_t* ctx)
/*
 * Returns heap unit size
 *
 */
{
   UNUSED(ctx);
   return sizeof(heap_tag_t);
}

/*****************************************************************************/
#ifdef USE_HEAP_MONITOR
bool
   heap_monitor(
      heap_ctx_t*   IN OUT   ctx)
/*
 * Makes heap monitoring
 *
 */
{

   heap_tag_t* p;
   heap_tag_t* q;

   uint cfree;
   uint cused;
   uint cmaxb;
   uint nblok;

   static FILE* pf = NULL;

   if (pf == NULL) {
      pf = fopen("heapmon.csv", "wt");
      fprintf(pf, "cfree;cused;cmaxb;nfrag;\n");
   }

   cfree = 0;
   cused = 0;
   cmaxb = 0;
   nblok = 0;

   sync_mutex_lock(&ctx->mutex);

   q = p = (heap_tag_t*)ctx->pbuf;
   while (q < p+ctx->cbuf) {

      if (q->cnext & HT_MASK_BUSY)
         cused += q->cnext & HT_MASK_OFFS;
      else {
         cfree += q->cnext;
         if ((q->cnext & HT_MASK_OFFS) > cmaxb) 
            cmaxb = q->cnext & HT_MASK_OFFS;
         nblok++;
      }

      q += (q->cnext & HT_MASK_OFFS) + 1;
      if (q >= p+ctx->cbuf)
         break;

   }

   sync_mutex_unlock(&ctx->mutex);
   fprintf(pf, "%d;%d;%d;%d\n", cfree, cused, cmaxb, cfree/nblok);
   return true;

}
#endif

#if 0
         {
            usize i, j;
            heap_stats(&i, &j, Stdout, mod->precomp.mem);
         }
#endif


