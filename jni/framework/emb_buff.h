#ifndef _EMB_BUFF_DEFINED
#define _EMB_BUFF_DEFINED

#include "emb_defs.h"
#include "emb_heap.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 *   Memory block API
 */

/*
 * Memory block
 *
 */
typedef struct mem_blk_s {
   byte*   p;
   usize   c;
} mem_blk_t;

/*
 * Readonly memory block
 *
 */
typedef struct mem_blk_readonly_s {
   const byte*   p;
   usize         c;
} mem_blk_readonly_t;

/*@@mem_blk_get_token
 * 
 * Gets token from memory block
 *
 * Parameters:     dst            destination 
 *                 src            source
 *                 offset         offset in source, bytes
 *                 separator      separation char
 *                 skip_before    flag: skip whitespaces before token
 *                 remove         flag: remove token from source
 *
 * Return:         true           if successful
 *                 false          if failed
 * 
 */
bool
   mem_blk_get_token(
      mem_blk_t*   IN OUT   dst, 
      mem_blk_t*   IN       src, 
      usize        IN       offset, 
      byte         IN       separator,
      bool         IN       skip_before,
      bool         IN       remove);

/*@@mem_blk_pop_token
 * 
 * Pops out first token from memory block
 *
 * C/C++ Syntax:   
 * bool 
 *    mem_blk_pop_token(                       
 *       mem_blk_t*   IN OUT   dst, 
 *       mem_blk_t*   IN       src, 
 *       byte         IN       separator,
 *       bool         IN       skip_before);
 *
 * Parameters:     dst            destination 
 *                 src            source
 *                 separator      separation char
 *                 skip_before    flag: skip whitespaces before token
 *
 * Return:         true           if successful
 *                 false          if failed
 * 
 */
#define /* bool */ mem_blk_pop_token(                                         \
                      /* mem_blk_t*          IN OUT   */   dst,               \
                      /* mem_blk_t*          IN       */   src,               \
                      /* byte                IN       */   separator,         \
                      /* bool                IN       */   skip_before)       \
       ( mem_blk_get_token((dst), (src), 0, (separator), (skip_before), true) )


/******************************************************************************
 *   String block API
 */
 
/*
 * String block
 *
 */
typedef mem_blk_t str_blk_t;

/*
 * Utility macros
 *
 */
#define STR_BLK_DECLARE(_p)                                                   \
           { (byte*)#_p, sizeof(#_p)-1 }
#define STR_BLK_SAME_TEXT(_s1, _s2)                                           \
           (                                                                  \
              ((_s1)->c == (_s2)->c) &&                                       \
              !StrNICmp((char*)(_s1)->p, (char*)(_s2)->p, (_s1)->c)           \
           ) 


/******************************************************************************
 *   Buffer API
 */

/* 
 * Buffer constants
 *
 */
typedef enum buf_const_e {
  buf_secured    = 0x20,                    /* Buffer securing flag          */
  buf_attached   = 0x40,                    /* Buffer attaching flag         */
  buf_shared     = 0x80,                    /* Shared memory flag            */
  buf_atom_mask  = 0x1F,                    /* Mask for atom size            */
  buf_alloc_mask = 0x3FFFFF00,              /* Mask for allocated size       */
  buf_no_growth  = (int)0x40000000          /* No buffer growth expected     */
} buf_const_t;

/*
 * Buffer, supports buffer of variable length
 *
 */
typedef struct buf_s {
   usize         cdata;                     /* Data size, atoms              */
   usize         flags;                     /* Reserved                      */
   heap_ctx_t*   mem;                       /* Memory allocator              */
   union __u {                              /* Attached buffer               */ 
      void*      pbuf;                      /* General pointer               */
      byte**     pptrs;                     /* Pointer to pointers           */
      byte*      pbytes;                    /* Byte pointer                  */
      unicode*   punicodes;                 /* Unicode-16 pointer            */
      uint16*    puints16;                  /* Uint16 pointer                */
      uint32*    puints32;                  /* Uint32 pointer                */
      usize*     pusizes;                   /* Usize pointer                 */
      digit*     pdigits;                   /* Digit pointer                 */
   } uptr;                                  /* Attached buffer               */ 
} buf_t;

/*@@buf_create
 *
 * Creates buffer
 *
 * C/C++ Syntax:   
 * bool 
 *    buf_create(                                                
 *       usize         IN       catom,                      
 *       usize         IN       cbuf,
 *       umask         IN       flags,
 *       buf_t*        IN OUT   buf,                        
 *       heap_ctx_t*   IN OUT   mem);
 *
 * Parameters:     catom          buffer atom size, bytes
 *                 cbuf           desired initial buffer length, atoms
 *                 flags          buffer flags, reserved
 *                 buf            buffer
 *                 mem            memory allocation context 
 *
 * Return:         true           if successful
 *                 false          if failed
 * 
 */
#ifdef USE_DEBUG

bool
   buf_create_helper(
      usize         IN       catom,
      usize         IN       cbuf,
      umask         IN       flags,
      const char*   IN       file, 
      unumber       IN       line, 
      buf_t*        IN OUT   buf,
      heap_ctx_t*   IN OUT   mem);

#define /* bool */ buf_create(                                                \
                      /* usize         IN     */  catom,                      \
                      /* usize         IN     */  cbuf,                       \
                      /* umask         IN     */  flags,                      \
                      /* buf_t*        IN OUT */  buf,                        \
                      /* heap_ctx_t*   IN OUT */  mem)                        \
       (                                                                      \
          buf_create_helper((catom), (cbuf), (flags), __FILE__, __LINE__,     \
             (buf), (mem))                                                    \
       )

#else

bool
   buf_create_helper(
      usize         IN       catom,
      usize         IN       cbuf,
      umask         IN       flags,
      buf_t*        IN OUT   buf,
      heap_ctx_t*   IN OUT   mem);

#define /* bool */ buf_create(                                                \
                      /* usize         IN     */  catom,                      \
                      /* usize         IN     */  cbuf,                       \
                      /* umask         IN     */  flags,                      \
                      /* buf_t*        IN OUT */  buf,                        \
                      /* heap_ctx_t*   IN OUT */  mem)                        \
       (                                                                      \
          buf_create_helper((catom), (cbuf), (flags), (buf), (mem))           \
       )

#endif

/*@@buf_destroy
 *
 * Destroys buffer
 *
 * Parameters:     buf            buffer
 *
 * Return:         true           if successful
 *                 false          if failed
 * 
 */
bool
   buf_destroy(
      buf_t*   IN OUT   buf);

/*@@buf_get_ptr_ptrs
 *
 * Returns pointer buffer pointer
 *
 * C/C++ Syntax:   
 * byte**
 *    buf_get_ptr_ptrs(                                        
 *       buf_t*   IN OUT   xbuf);
 *
 * Parameters:     xbuf           buffer
 *
 * Return:         pointer buffer pointer
 * 
 */
#define /* byte** */ buf_get_ptr_ptrs(                                        \
                       /* buf_t*   IN OUT */   xbuf)                          \
                    ( (xbuf)->uptr.pptrs )

/*@@buf_get_ptr_bytes
 *
 * Returns byte buffer pointer
 *
 * C/C++ Syntax:   
 * byte* 
 *    buf_get_ptr_bytes(                                        
 *       buf_t*   IN OUT   xbuf);
 *
 * Parameters:     xbuf           buffer
 *
 * Return:         byte buffer pointer
 * 
 */
#define /* byte* */ buf_get_ptr_bytes(                                        \
                       /* buf_t*   IN OUT */   xbuf)                          \
                    ( (xbuf)->uptr.pbytes )

/*@@buf_get_ptr_digits
 *
 * Returns digit buffer pointer
 *
 * C/C++ Syntax:   
 * digit* 
 *    buf_get_ptr_digits(                                        
 *       buf_t*   IN OUT   xbuf);
 *
 * Parameters:     xbuf           buffer
 *
 * Return:         digit buffer pointer
 * 
 */
#define /* digit* */ buf_get_ptr_digits(                                      \
                        /* buf_t*   IN OUT */   xbuf)                         \
                     ( (xbuf)->uptr.pdigits )
 
/*@@buf_get_ptr_unicodes
 *
 * Returns unicode buffer pointer
 *
 * C/C++ Syntax:   
 * unicode* 
 *    buf_get_ptr_unicodes(                                        
 *       buf_t*   IN OUT   xbuf);
 *
 * Parameters:     xbuf           buffer
 *
 * Return:         unicode buffer pointer
 * 
 */
#define /* unicode* */ buf_get_ptr_unicodes(                                  \
                         /* buf_t*   IN OUT */   xbuf)                        \
                      ( (xbuf)->uptr.punicodes )

/*@@buf_get_ptr_uints16
 *
 * Returns uint16 buffer pointer
 *
 * C/C++ Syntax:   
 * uint16* 
 *    buf_get_ptr_uints16(                                        
 *       buf_t*   IN OUT   xbuf);
 *
 * Parameters:     xbuf           buffer
 *
 * Return:         uint16 buffer pointer
 * 
 */
#define /* uint16* */ buf_get_ptr_uints16(                                    \
                         /* buf_t*   IN OUT */   xbuf)                        \
                      ( (xbuf)->uptr.puints16 )

/*@@buf_get_ptr_uints32
 *
 * Returns uint32 buffer pointer
 *
 * C/C++ Syntax:   
 * uint32* 
 *    buf_get_ptr_uints32(                                        
 *       buf_t*   IN OUT   xbuf);
 *
 * Parameters:     xbuf           buffer
 *
 * Return:         uint32 buffer pointer
 * 
 */
#define /* uint32* */ buf_get_ptr_uints32(                                    \
                         /* buf_t*   IN OUT */   xbuf)                        \
                      ( (xbuf)->uptr.puints32 )

/*@@buf_get_ptr_usizes
 *
 * Returns usize buffer pointer
 *
 * C/C++ Syntax:   
 * usize* 
 *    buf_get_ptr_usizes(                                        
 *       buf_t*   IN OUT   xbuf);
 *
 * Parameters:     xbuf           buffer
 *
 * Return:         usize buffer pointer
 * 
 */
#define /* usize */ buf_get_ptr_usizes(                                       \
                       /* buf_t*   IN OUT */   xbuf)                          \
                    ( (xbuf)->uptr.pusizes )

/*@@buf_get_length
 *
 * Returns buffer data length 
 *
 * C/C++ Syntax:   
 * bool 
 *    buf_get_length(                                        
 *       buf_t*   IN OUT   xbuf);
 *
 * Parameters:     xbuf           buffer
 *
 * Return:         data length, atoms
 * 
 */
#define /* bool */ buf_get_length(                                            \
                      /* buf_t*   IN OUT */   xbuf)                           \
                   ( (xbuf)->cdata )

/*@@buf_set_length
 *
 * Sets buffer data length (in range < allocated size)
 *
 * Parameters:     len            desired data length, atoms
 *                 buf            buffer
 *                 
 *
 * Return:         none           if successful
 *                 false          if failed
 * 
 */
bool 
   buf_set_length(                                           
      usize    IN       len,
      buf_t*   IN OUT   buf);

/*@@buf_set_empty
 *
 * Clears buffer length 
 *
 * C/C++ Syntax:   
 * void 
 *    buf_set_empty(                                        
 *       buf_t*   IN OUT   xbuf);
 *
 * Parameters:     xbuf           buffer
 *
 * Returns:        none                
 * 
 */
#define /* void */ buf_set_empty(                                             \
                      /* buf_t*   IN OUT */   xbuf)                           \
       {                                                                      \
          usize cbuf;                                                         \
          assert((xbuf) != NULL);                                             \
          cbuf = buf_get_allocated_size(xbuf);                                \
          if ((xbuf)->flags & buf_secured)                                    \
             MemSet((xbuf)->uptr.pbuf, 0x00, cbuf);                           \
          (xbuf)->flags &= ~buf_alloc_mask;                                   \
          (xbuf)->flags |= cbuf << 8;                                         \
          (xbuf)->cdata  = 0;                                                 \
       }

/*@@buf_expand
 *
 * Expands buffer 
 *
 * C/C++ Syntax:   
 * bool 
 *    buf_expand(                                                
 *       uint     IN       len,                            
 *       buf_t*   IN OUT   buf);
 * 
 * Parameters:     len            minimal required buffer length, atoms
 *                 buf            buffer
 *
 * Return:         true           if successful
 *                 false          if failed
 * 
 */
#ifdef USE_DEBUG

bool
   buf_expand_helper(
      usize         IN       len, 
      const char*   IN       file, 
      unumber       IN       line, 
      buf_t*        IN OUT   buf);

#define /* bool */ buf_expand(                                                \
                      /* usize    IN     */   len,                            \
                      /* buf_t*   IN OUT */   buf)                            \
       (  buf_expand_helper((len), __FILE__, __LINE__, (buf)) )

#else

bool
   buf_expand_helper(
      usize         IN       len, 
      buf_t*        IN OUT   buf);

#define /* bool */ buf_expand(                                                \
                      /* usize    IN     */   len,                            \
                      /* buf_t*   IN OUT */   buf)                            \
       (  buf_expand_helper((len), (buf)) )

#endif

/*@@buf_load
 *
 * Loads atoms from raw buffer 
 *
 * C/C++ Syntax:   
 * bool 
 *    buf_load(                                                  
 *       const void*   IN       pdata,                     
 *       uint          IN       offset,                    
 *       uint          IN       cdata,                     
 *       buf_t*        IN OUT   buf);
 * 
 * Parameters:     pdata          raw buffer pointer
 *                 offset         write offset, atoms
 *                 cdata          raw data length, atoms
 *                 buf            buffer 
 *
 * Return:         true           if successful
 *                 false          if failed
 * 
 */
#ifdef USE_DEBUG

bool
   buf_load_helper(
      const void*   IN       pdata, 
      usize         IN       offset, 
      usize         IN       cdata,
      const char*   IN       file, 
      unumber       IN       line, 
      buf_t*        IN OUT   buf);

#define /* bool */ buf_load(                                                  \
                      /* const void*   IN     */   pdata,                     \
                      /* usize         IN     */   offset,                    \
                      /* usize         IN     */   cdata,                     \
                      /* buf_t*        IN OUT */   buf)                       \
       (                                                                      \
          buf_load_helper(                                                    \
             (pdata),                                                         \
             (offset),                                                        \
             (cdata),                                                         \
             __FILE__,                                                        \
             __LINE__,                                                        \
             (buf))                                                           \
       )

#else

bool
   buf_load_helper(
      const void*   IN       pdata, 
      usize         IN       offset, 
      usize         IN       cdata,
      buf_t*        IN OUT   buf);

#define /* bool */ buf_load(                                                  \
                      /* const void*   IN     */   pdata,                     \
                      /* usize         IN     */   offset,                    \
                      /* usize         IN     */   cdata,                     \
                      /* buf_t*        IN OUT */   buf)                       \
       (                                                                      \
          buf_load_helper(                                                    \
             (pdata),                                                         \
             (offset),                                                        \
             (cdata),                                                         \
             (buf))                                                           \
       )

#endif

/*@@buf_append
 * 
 * Appends atoms from raw buffer 
 *
 * C/C++ Syntax:   
 * bool 
 *    buf_append(                                                
 *       const void*   IN       ptr,                       
 *       uint          IN       cnt,                       
 *       buf_t*        IN OUT   buf);
 * 
 * Parameters:     ptr            raw buffer pointer
 *                 cnt            raw data length, atoms
 *                 buf            buffer 
 *
 * Return:         true           if successful
 *                 false          if failed
 * 
 */
#define /* bool */ buf_append(                                                \
                      /* const void*   IN     */   ptr,                       \
                      /* uint          IN     */   cnt,                       \
                      /* buf_t*        IN OUT */   buf)                       \
       (                                                                      \
          ((buf) == NULL) ?                                                   \
             ERR_SET_NO_RET(err_bad_param), false :                           \
             buf_load((ptr), (buf)->cdata, (cnt), (buf))                      \
       )

/*@@buf_fill
 * 
 * Fills buffer with sample atom
 *
 * Parameters:     sample         sample atom
 *                 offset         fill offset, atoms
 *                 cfill          fill length, atoms
 *                 buf            buffer 
 *
 * Return:         true           if successful
 *                 false          if failed
 * 
 */
bool
   buf_fill(
      unumber  IN       sample, 
      usize    IN       offset, 
      usize    IN       cfill,
      buf_t*   IN OUT   buf);

/*@@buf_attach
 * 
 * Attaches raw buffer to buffer object
 *
 * Parameters:     pdata          source buffer 
 *                 cval           length of meaningful part, atoms
 *                 cbuf           length of allocated part, atoms
 *                 shared         shared memory flag
 *                 buf            buffer 
 *
 * Return:         true           if successful
 *                 false          if failed
 * 
 */
bool
   buf_attach(
      void*    IN       pdata,
      usize    IN       cval,
      usize    IN       cbuf,
      bool     IN       shared, 
      buf_t*   IN OUT   buf);

/*@@buf_detach 
 *
 * Detaches raw buffer (if any) from buffer object
 *
 * Parameters:     buf            buffer 
 *
 * Return:         true           if successful
 *                 false          if failed
 * 
 */
bool 
   buf_detach(                                                                
      buf_t*   IN OUT   buf);

/*@@buf_equal 
 *
 * Checks buffers content equality
 *
 * C/C++ Syntax:   
 * bool 
 *    buf_equal(
 *       const buf_t*   IN   buf1,                         
 *       const buf_t*   IN   buf2);
 *
 * Parameters:     buf1           first buffer 
 *                 buf2           second buffer
 *
 * Return:         true           if buffers contents are the same
 *                 false          if buffers contain different data
 * 
 */
#define /* bool */ buf_equal(                                                 \
                      /* const buf_t*   IN */   buf1,                         \
                      /* const buf_t*   IN */   buf2)                         \
       (                                                                      \
          (buf_get_atom_size(buf1) == buf_get_atom_size(buf2))                \
             &&                                                               \
          (buf_get_length(buf1) == buf_get_length(buf2))                      \
             &&                                                               \
          (MemCmp(                                                            \
              (buf1)->uptr.pbuf,                                              \
              (buf2)->uptr.pbuf,                                              \
              buf_get_length(buf1)*buf_get_atom_size(buf1)) == 0)             \
       )

/*@@buf_get_atom_size
 *
 * Returns buffer atom size
 *
 * C/C++ Syntax:   
 * uint 
 *    buf_get_atom_size(
 *       const buf_t*   IN   buf);
 *
 * Parameters:     buf            buffer 
 *
 * Return:         buffer atom size, bytes
 * 
 */
#define /* uint */ buf_get_atom_size(                                         \
                      /* const buf_t*   IN */   buf)                          \
       ( (unsigned)((buf)->flags & buf_atom_mask) )

/*@@buf_get_allocated_size
 *
 * Returns buffer allocated size
 *
 * C/C++ Syntax:   
 * uint 
 *    buf_get_allocated_size(
 *       const buf_t*   IN   buf);
 *
 * Parameters:     buf            buffer 
 *
 * Return:         buffer allocated size, atoms
 * 
 */
#define /* uint */ buf_get_allocated_size(                                    \
                      /* const buf_t*   IN */   buf)                          \
       ( (unsigned)((((buf)->flags & buf_alloc_mask) >> 8) + (buf)->cdata) )


/******************************************************************************
 *   Memory chunk list API
 */

/*
 * Memory chunk list
 *
 */
#ifdef USE_MCL
typedef struct mem_chunk_list_s {
   list_entry_t   items;                         /* Chunks list              */
   umask          flags;                         /* Reserved                 */
   heap_ctx_t*    mem;                           /* Heap context             */
} mem_chunk_list_t;
#else
typedef struct buf_s mem_chunk_list_t;
#endif

/*@@mem_chunk_list_create
 *
 * Creates empty list of memory chunks
 *
 * Parameters:     lst            chunk list
 *                 mem            heap context
 *
 * Return:         true           if successful,
 *                 false          if failed
 *
 */
#ifdef USE_MCL
bool
   mem_chunk_list_create(
      mem_chunk_list_t*   IN OUT   lst,
      heap_ctx_t*         IN OUT   mem);
#else
#define /* bool */ mem_chunk_list_create(                                     \
                      /* mem_chunk_list_t*   IN OUT */   lst,                 \
                      /* heap_ctx_t*         IN OUT */   mem)                 \
       ( buf_create(1, 0, 0, (lst), (mem)) )
#endif

/*@@mem_chunk_list_destroy
 *
 * Destroys list of memory chunks
 *
 * Parameters:     lst            chunk list
 *
 * Return:         true           if successful,
 *                 false          if failed
 *
 */
#ifdef USE_MCL
bool
   mem_chunk_list_destroy(
      mem_chunk_list_t*   IN OUT   lst);
#else
#define /* bool */ mem_chunk_list_destroy(                                    \
                      /* mem_chunk_list_t*   IN OUT */   lst)                 \
       ( buf_destroy((lst)) )
#endif

/*@@mem_chunk_list_attach
 *
 * Attaches given data to chunk list
 *
 * Parameters:     lst            chunk list
 *                 blk            memory block to attach
 *
 * Return:         true           if successful,
 *                 false          if failed
 *
 */
#ifdef USE_MCL
bool
   mem_chunk_list_attach(
      mem_chunk_list_t*   IN OUT   lst,
      byte*               IN       pdata,
      usize               IN       cdata);
#else
#define /* bool */ mem_chunk_list_attach(                                     \
                      /* mem_chunk_list_t*   IN OUT */   lst,                 \
                      /* byte*               IN     */   pdata,               \
                      /* usize               IN     */   cdata)               \
       ( buf_attach((pdata), (cdata), (cdata), true, (lst)) )
#endif

/*@@mem_chunk_list_move
 *
 * Moves data from one chunk list to another 
 *
 * Parameters:     dst            destination chunk list
 *                 src            source chunk list
 *                 to_head        insert-to-head flag
 *                 bytes          bytes to move or 0 if move all
 *
 * Return:         true           if successful,
 *                 false          if failed
 *
 */
bool
   mem_chunk_list_move(
      mem_chunk_list_t*   IN OUT   dst,
      mem_chunk_list_t*   IN OUT   src,
      bool                IN       to_head,
      usize               IN       bytes);

/*@@mem_chunk_list_reuse
 *
 * Reuses old list (destroys & pre-allocates required number of bytes)
 *
 * Parameters:     lst            chunk list
 *                 cbuf           number of free space to reuse, bytes
 *
 * Return:         true           if successful,
 *                 false          if failed
 *
 */
bool
   mem_chunk_list_reuse(
      mem_chunk_list_t*   IN OUT   lst,
      usize               IN       cbuf);

/*@@mem_chunk_list_pre_alloc
 *
 * Pre-allocates required number of bytes in list
 *
 * Parameters:     lst            chunk list
 *                 cbuf           number of bytes to pre-allocate
 *                 to_head        insert-to-head flag
 *
 * Return:         true           if successful,
 *                 false          if failed
 *
 */
bool
   mem_chunk_list_pre_alloc(
      mem_chunk_list_t*   IN OUT   lst,
      usize               IN       cbuf,
      bool                IN       to_head);

/*@@mem_chunk_list_acquire_block
 *
 * Acquires new block of memory from list
 *
 * Parameters:     blk            acquired block
 *                 lst            chunk list
 *                 cbuf           desired number of bytes in block
 *                 to_head        insert-to-head flag
 *
 * Return:         true           if successful,
 *                 false          if failed
 *
 */
bool
   mem_chunk_list_acquire_block(
      mem_blk_t*          OUT      blk,
      mem_chunk_list_t*   IN OUT   lst,
      usize               IN       cbuf,
      bool                IN       to_head);

/*@@mem_chunk_list_release_block
 *
 * Releases previously acquired block of memory 
 *
 * Parameters:     lst            chunk list
 *                 blk            block to release
 *                 cdata          used number of bytes is block
 *
 * Return:         true           if successful,
 *                 false          if failed
 *
 */
bool
   mem_chunk_list_release_block(
      mem_chunk_list_t*   IN OUT   lst,
      const mem_blk_t*    IN       blk,
      usize               IN       cdata);

/*@@mem_chunk_list_push
 *
 * Pushes given data into list 
 *
 * Parameters:     lst            chunk list
 *                 pdata          data to push
 *                 cdata          number of bytes to push
 *                 to_head        insert-to-head flag
 *
 * Return:         true           if successful,
 *                 false          if failed
 *
 */
bool
   mem_chunk_list_push(
      mem_chunk_list_t*   IN OUT   lst,
      const byte*         IN       pdata,
      usize               IN       cdata,
      bool                IN       to_head);

/*@@mem_chunk_list_get
 *
 * Gets requested number of bytes from list 
 *
 * Parameters:     ok             success flag
 *                 pbuf           pointer to destination (may be NULL if skip)
 *                 cbuf           number of bytes to get 
 *                 lst            chunk list
 *                 from_head      take-from-head flag
 *                 remove         removal-from-list flag
 *
 * Return:         true           if successful,
 *                 false          if failed
 *
 */
bool
   mem_chunk_list_get(
      bool*               OUT      ok,
      byte*               IN OUT   pbuf,
      usize               IN       cbuf,
      mem_chunk_list_t*   IN OUT   lst,
      bool                IN       from_head,
      bool                IN       remove);

/*@@mem_chunk_list_pop
 *
 * Pops out requested number of bytes from list 
 *
 * C/C++ Syntax:   
 * bool 
 *    mem_chunk_list_pop(                       
 *       bool*               OUT      ok,        
 *       byte*               IN OUT   pdata,
 *       uint                IN       cdata,      
 *       mem_chunk_list_t*   IN OUT   lst,       
 *       bool                IN       from_head);
 *
 * Parameters:     ok             success flag
 *                 pdata          pointer to destination (may not be NULL)
 *                 cdata          number of bytes to pop out
 *                 lst            chunk list
 *                 from_head      take-from-head flag
 *
 * Return:         true           if successful,
 *                 false          if failed
 *
 */
#define /* bool */ mem_chunk_list_pop(                                        \
                      /* bool*               OUT    */   ok,                  \
                      /* byte*               IN OUT */   pdata,               \
                      /* uint                IN     */   cdata,               \
                      /* mem_chunk_list_t*   IN OUT */   lst,                 \
                      /* bool                IN     */   from_head)           \
       ( mem_chunk_list_get((ok), (pdata), (cdata), (lst), from_head, true) )

/*@@mem_chunk_list_skip
 *
 * Skips out requested number of bytes from list 
 *
 * C/C++ Syntax:   
 * bool 
 *    mem_chunk_list_skip(                       
 *       bool*               OUT      ok,        
 *       uint                IN       count,      
 *       mem_chunk_list_t*   IN OUT   lst,       
 *       bool                IN       from_head);
 *
 * Parameters:     ok             success flag
 *                 count          number of bytes to skip out
 *                 lst            chunk list
 *                 from_head      take-from-head flag
 *
 * Return:         true           if successful,
 *                 false          if failed
 *
 */
#define /* bool */ mem_chunk_list_skip(                                       \
                      /* bool*               OUT    */   ok,                  \
                      /* uint                IN     */   count,               \
                      /* mem_chunk_list_t*   IN OUT */   lst,                 \
                      /* bool                IN     */   from_head)           \
       ( mem_chunk_list_get((ok), NULL, (count), (lst), from_head, true) )

/*@@mem_chunk_list_get_size
 *
 * Returns size of chunk list data 
 *
 * Parameters:     cdata          data size
 *                 lst            chunk list
 *
 * Return:         true           if successful,
 *                 false          if failed
 *
 */
#ifdef USE_MCL
bool
   mem_chunk_list_get_size(
      usize*                    OUT   cdata,
      const mem_chunk_list_t*   OUT   lst);
#else
#define /* bool */ mem_chunk_list_get_size(                                   \
                      /* usize*                    OUT */   cdata,            \
                      /* const mem_chunk_list_t*   OUT */   lst)              \
       ( assert((cdata) != NULL), *(cdata)=buf_get_length(lst), true )
#endif

/*@@mem_chunk_list_iterate
 *
 * Iterates through chunk list
 *
 * Parameters:     ok             success flag
 *                 blk            block pointer
 *                 iterator       iterator handle, NULL for first
 *                 lst            chunk list
 *
 * Return:         true           if successful,
 *                 false          if failed
 *
 */
bool
   mem_chunk_list_iterate(
      bool*                     OUT      ok, 
      mem_blk_t*                OUT      blk,
      handle*                   IN OUT   iterator,
      const mem_chunk_list_t*   IN       lst);

/*@@mem_chunk_list_find_byte
 *
 * Searches for first entry of specified byte in chunk list
 *
 * Parameters:     found          index of found byte, or NOT_FOUND_MARKER
 *                 offset         search offset   
 *                 sample         sample to search for
 *                 lst            chunk list
 *
 * Return:         true           if successful,
 *                 false          if failed
 *
 */
bool
   mem_chunk_list_find_byte(
      usize*                    OUT   found, 
      usize                     IN    offset,
      byte                      IN    sample,
      const mem_chunk_list_t*   IN    lst);

#ifdef USE_DEBUG
/*@@mem_chunk_list_dump
 * 
 * Dumps memory chunk list
 *
 * Parameters:     lst            chunk list
 *
 * Return:         true           if successful,
 *                 false          if failed
 *
 */
bool
   mem_chunk_list_dump(
      mem_chunk_list_t*   IN OUT   lst);
#endif      

/*@@buf_to_mem_chunk_list
 *
 * Converts buffer to memory chunk list
 *
 * Parameters:     dst            destination chunk list
 *                 src            source buffer
 *                 append         append flag
 *                 copy           copy source flag
 *
 * Return:         true           if successful,
 *                 false          if failed
 *
 */
bool
   buf_to_mem_chunk_list(
      mem_chunk_list_t*   IN OUT   dst,
      buf_t*              IN OUT   src,
      bool                IN       append,
      bool                IN       copy);

/*@@buf_from_mem_chunk_list
 *
 * Converts memory chunk list to buffer
 *
 * Parameters:     dst            destination buffer
 *                 src            source chunk list
 *                 append         append flag
 *                 copy           copy source flag
 *
 * Return:         true           if successful,
 *                 false          if failed
 *
 */
bool
   buf_from_mem_chunk_list(
      buf_t*              IN OUT   dst,
      mem_chunk_list_t*   IN OUT   src,
      bool                IN       append,
      bool                IN       copy);

#ifdef __cplusplus
}
#endif


#endif

