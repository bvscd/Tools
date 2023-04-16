#ifndef _EMB_DEFS_DEFINED
#define _EMB_DEFS_DEFINED

#include "emb_port.h"

#ifdef __cplusplus
extern "C" {
#endif


/******************************************************************************
 *   General switches
 */

/*
 * Porting layer defines following compile environment macros:
 *   
 * #define ENDIAN_BIG     - big endian CPU if defined
 * #define ENDIAN_LITTLE  - little endian CPU if defined 
 * #define PLATFORM_32BIT - 32-bit CPU if defined
 * #define PLATFORM_64BIT - 64-bit CPU if defined
 * #define ADDRESS_32BIT  - 32-bit address if defined
 * #define ADDRESS_64BIT  - 64-bit address if defined
 *
 */

/******************************************************************************
 *   Environment-independent types
 */

/*
 * Porting layer defines following environment-dependent types:
 *   
 * bool    - boolean value
 * byte    - byte value                   
 * int8    - 8-bit signed integer value          
 * int16   - 16-bit signed integer value          
 * int32   - 32-bit signed integer value          
 * int64   - 64-bit signed integer value         
 * int128  - 128-bit signed integer value         
 * uint8   - 8-bit unsigned integer value  
 * uint16  - 16-bit unsigned integer value 
 * uint32  - 32-bit unsigned integer value 
 * uint64  - 64-bit unsigned integer value
 * uint128 - 128-bit unsigned integer value
 * ulong   - unsigned long integer value  
 * uint    - unsigned integer value        
 * ushort  - unsigned short integer value 
 * unicode - UNICODE character (16-bits)
 * handle  - generic void* pointer
 */


#if defined(PLATFORM_32BIT)
typedef uint32 digit;                       /* CPU word (register size)      */
typedef uint64 ddigit;                      /* CPU double word               */
typedef int64  sddigit;                     /* CPU signed double word        */
#elif defined(PLATFORM_64BIT)
typedef uint64  digit;                      /* CPU word (register size)      */
typedef uint128 ddigit;                     /* CPU double word               */
typedef int128  sddigit;                    /* CPU signed double word        */
#else
#error Cannot define digit types
#endif

#ifndef NULL
#ifdef __cplusplus
#define NULL 0
#else
#define NULL (void*) 0
#endif
#endif


/******************************************************************************
 *   Utility macros
 */

#define BITS_UMASK                                                            \
           ( sizeof(umask) << 3 )

#define LBIT_UMASK                                                            \
           ( (umask)1UL )

#define HBIT_UMASK                                                            \
           ( (umask) ((umask)1UL << (BITS_UMASK - 1)) )

#define BITS_UNUMBER                                                          \
           ( sizeof(unumber) << 3 )

#define LBIT_UNUMBER                                                          \
           ( (unumber)1UL )

#define HBIT_UNUMBER                                                          \
           ( (unumber) ((unumber)1UL << (BITS_UNUMBER - 1)) )

#define BITS_USIZE                                                            \
           ( sizeof(usize) << 3 )

#define LBIT_USIZE                                                            \
           ( (usize)1UL )

#define HBIT_USIZE                                                            \
           ( (usize) ((usize)1UL << (BITS_USIZE - 1)) )

#ifndef MAX
#define MAX(x, y)                                                             \
           ( ((x) < (y)) ? (y) : (x) )
#endif
                                                    
#ifndef MIN                                     
#define MIN(x, y)                                                             \
           ( ((x) < (y)) ? (x) : (y) )
#endif

#define LBIT_U32                                                              \
           ( (uint32)1UL )                  /* 0x00000001 */

#define HBIT_U32                                                              \
           ( (uint32)(1UL << 31) )          /* 0x80000000 */

#define LBIT_M32(x)                                                           \
           ( 0xFFFFFFFFUL >> (32 - (x)) )   /* 0x00000FFF */

#define HBIT_M32(x)                                                           \
           ( 0xFFFFFFFFUL << (32 - (x)) )   /* 0xFFF00000 */

#define SBIT_U32(x)                                                           \
           ( ((x) & ((x) - 1)) == 0 )       /* 0x00001000 */

#if defined(_MSC_VER) && defined(PLATFORM_32BIT)                   
#define ROTL_U32(x, y)                                                        \
           ( (uint32)_rotl(x, y) )
#else
#define ROTL_U32(x, y)                                                        \
           (                                                                  \
              (uint32)                                                        \
              (                                                               \
                 ( (x) << (y) )                                               \
                    |                                                         \
                 ( (x) >> (32 - (y)) )                                        \
              )                                                               \
           )
#endif

#if defined(_MSC_VER) && defined(PLATFORM_32BIT)
#define ROTR_U32(x, y)                                                        \
           ( (uint32)_rotr(x, y) )
#else                                                                         
#define ROTR_U32(x, y)                                                        \
           (                                                                  \
              (uint32)                                                        \
              (                                                               \
                 ( (x) >> (y) )                                               \
                    |                                                         \
                 ( (x) << (32 - (y)) )                                        \
              )                                                               \
           )
#endif

#define ROTL_U16(x, y)                                                        \
           (                                                                  \
              (uint16)                                                        \
              (                                                               \
                 ( (x) << (y) )                                               \
                    |                                                         \
                 ( (x) >> (16 - (y)) )                                        \
              )                                                               \
           )

#define ROTR_U16(x, y)                                                        \
           (                                                                  \
              (uint16)                                                        \
              (                                                               \
                 ( (x) >> (y) )                                               \
                    |                                                         \
                 ( (x) << (16 - (y)) )                                        \
              )                                                               \
           )

#define ROTL_U64(x, y)                                                        \
           (                                                                  \
              (uint64)                                                        \
              (                                                               \
                 ( (x) << (y) )                                               \
                    |                                                         \
                 ( (x) >> (64 - (y)) )                                        \
              )                                                               \
           )

#define ROTR_U64(x, y)                                                        \
           (                                                                  \
              (uint64)                                                        \
              (                                                               \
                 ( (x) >> (y) )                                               \
                    |                                                         \
                 ( (x) << (64 - (y)) )                                        \
              )                                                               \
           )

#if defined(_MSC_VER) && defined(_M_IX86)
#define BYTE0_U32(x)                                                          \
           ( (byte)(x) )
#define BYTE1_U32(x)                                                          \
           ( (byte)((x) >> 8) )
#define BYTE2_U32(x)                                                          \
           ( (byte)((x) >> 16) )
#define BYTE3_U32(x)                                                          \
           ( (byte)((x) >> 24) )
#else
#define BYTE0_U32(x)                                                          \
           ( (byte)((x) & 0xFF) )
#define BYTE1_U32(x)                                                          \
           ( (byte)(((x) >> 8) & 0xFF) )
#define BYTE2_U32(x)                                                          \
           ( (byte)(((x) >> 16) & 0xFF) )
#define BYTE3_U32(x)                                                          \
           ( (byte)((x) >> 24) )
#endif

#define BITS_DIGIT                                                            \
           ( sizeof(digit) << 3 )

#define LBIT_DIGIT                                                            \
           ( (digit)(1UL) )        

#define HBIT_DIGIT                                                            \
           ( (digit)(1UL << (BITS_DIGIT-1)) )        

#define BITS_TO_DIGITS(bits)                                                  \
           ( (usize) (((usize)(bits)+BITS_DIGIT-1) / BITS_DIGIT) )

#define BITS_TO_BYTES(bits)                                                   \
           ( (usize) (((usize)(bits)+7) / 8) )

#define BYTES_TO_BITS(bytes)                                                  \
           ( (usize)(bytes)*8 )

#define LDIGIT_DDIGIT(dd)                                                     \
           ( (digit)(dd) )

#define HDIGIT_DDIGIT(dd)                                                     \
           ( (digit)((dd) >> (sizeof(digit) << 3)) )

#define MAKE_DDIGIT(d1, d2)                                                   \
           ( (ddigit)((ddigit)(d2) << (BITS_DIGIT)) | (d1) )

#define DIGITS_TO_BYTES(digits)                                               \
           ( (digits)*sizeof(digit) )

#define BYTES_TO_DIGITS(bytes)                                                \
           ( ((bytes)+sizeof(digit)-1)/sizeof(digit) )

#define EXTRA_BITS_IN_BYTE(bits)                                              \
           ( (8-(bits)%8)%8 )

#define SWAP_VARS(x, y, type)                                                 \
           { type _t; _t=x; x=y; y=_t; }

#ifdef __cplusplus
#define OFFSETOF(s, x)                                                        \
           offsetof(s, x)
#else
#define OFFSETOF(s, x)                                                        \
           ( (usize) &(((s*) 0)->x) )
#endif

#define W64(x)                                                                \
           PORTABLE_W64(x)

#define INLINE                                                                \
           PORTABLE_INLINE

#define NOT_FOUND_MARKER                                                      \
           ( (usize)-1 )

#define SIZE_U16                                                              \
           ( sizeof(uint16) )
#define SIZE_U32                                                              \
           ( sizeof(uint32) )
#define SIZE_U64                                                              \
           ( sizeof(uint64) )

#if defined(PLATFORM_64BIT)
#define XOR_BUF_INPLACE(pd, ps, c)                                            \
           {                                                                  \
              byte* _pd = (pd);                                               \
              const byte* _ps = (ps);                                         \
              usize _c  = (c), _i, _j;                                        \
              for (; _c>0; ) {                                                \
                 if (((usize)_pd%SIZE_U64) || ((usize)_ps%SIZE_U64)) {        \
                    for (_i=0, _j=_c/SIZE_U64, _c-=_j*SIZE_U64; _i<_j; _i++)  \
                       ((uint64*)_pd)[_i] ^= ((uint64*)_ps)[_i];              \
                    _pd += SIZE_U64, _ps += SIZE_U64;                         \
                 }                                                            \
                 if (((usize)_pd%SIZE_U32) || ((usize)_ps%SIZE_U32)) {        \
                    for (_i=0, _j=_c/SIZE_U32, _c-=_j*SIZE_U32; _i<_j; _i++)  \
                       ((uint32*)_pd)[_i] ^= ((uint32*)_ps)[_i];              \
                    _pd += SIZE_U32, _ps += SIZE_U32;                         \
                 }                                                            \
                 if (((usize)_pd%SIZE_U16) || ((usize)_ps%SIZE_U16)) {        \
                    for (_i=0, _j=_c/SIZE_U16, _c-=_j*SIZE_U16; _i<_j; _i++)  \
                       ((uint16*)_pd)[_i] ^= ((uint16*)_ps)[_i];              \
                    _pd += SIZE_U16, _ps += SIZE_U16;                         \
                 }                                                            \
                 *(_pd++) ^= *(_ps++);                                        \
                 _c--;                                                        \
              }                                                               \
           }
#else
#define XOR_BUF_INPLACE(pd, ps, c)                                            \
           {                                                                  \
              byte* _pd = (pd);                                               \
              const byte* _ps = (ps);                                         \
              usize _c  = (c), _i, _j;                                        \
              for (; _c>0; ) {                                                \
                 if (((usize)_pd%SIZE_U32) || ((usize)_ps%SIZE_U32)) {        \
                    for (_i=0, _j=_c/SIZE_U32, _c-=_j*SIZE_U32; _i<_j; _i++)  \
                       ((uint32*)_pd)[_i] ^= ((uint32*)_ps)[_i];              \
                    _pd += SIZE_U32, _ps += SIZE_U32;                         \
                 }                                                            \
                 if (((usize)_pd%SIZE_U16) || ((usize)_ps%SIZE_U16)) {        \
                    for (_i=0, _j=_c/SIZE_U16, _c-=_j*SIZE_U16; _i<_j; _i++)  \
                       ((uint16*)_pd)[_i] ^= ((uint16*)_ps)[_i];              \
                    _pd += SIZE_U16, _ps += SIZE_U16;                         \
                 }                                                            \
                 *(_pd++) ^= *(_ps++);                                        \
                 _c--;                                                        \
              }                                                               \
           }
#endif


/******************************************************************************
 *   Byte order macros
 */

#define SWAP16(x)                                                             \
           (                                                                  \
              (uint16)                                                        \
              (((x) & 0xFF00) >> 8)                                           \
                 |                                                            \
              (((x) & 0x00FF) << 8)                                           \
           )

#define SWAP32(x)                                                             \
           (                                                                  \
              (uint32)                                                        \
              (((x) & 0x000000FF) << 24)                                      \
                 |                                                            \
              (((x) & 0x0000FF00) <<  8)                                      \
                 |                                                            \
              (((x) & 0x00FF0000) >>  8)                                      \
                 |                                                            \
              (((x) & 0xFF000000) >> 24)                                      \
           )

#if defined(PLATFORM_32BIT)

#define SWAP64(x)                                                             \
           (                                                                  \
              (uint64)                                                        \
              ((uint64)(SWAP32((uint32)(x))) << 32)                           \
                 |                                                            \
              (SWAP32((uint32)((uint64)(x) >> 32)))                           \
           )                                                  

#else

#define SWAP64(x)                                                             \
           (                                                                  \
              (uint64)                                                        \
              (((x) & W64(0x00000000000000FF)) << 56)                         \
                 |                                                            \
              (((x) & W64(0x000000000000FF00)) << 40)                         \
                 |                                                            \
              (((x) & W64(0x0000000000FF0000)) << 24)                         \
                 |                                                            \
              (((x) & W64(0x00000000FF000000)) <<  8)                         \
                 |                                                            \
              (((x) & W64(0x000000FF00000000)) >>  8)                         \
                 |                                                            \
              (((x) & W64(0x0000FF0000000000)) >> 24)                         \
                 |                                                            \
              (((x) & W64(0x00FF000000000000)) >> 40)                         \
                 |                                                            \
              (((x) & W64(0xFF00000000000000)) >> 56)                         \
           )

#endif /* PLATFORM_ ... */

#if defined(ENDIAN_BIG)

#define NTOH32(x)                                                             \
           ( (uint32)(x) )

#define NTOH16(x)                                                             \
           ( (uint16)(x) )

#elif defined(ENDIAN_LITTLE)

#define NTOH32(x)                                                             \
           ( (uint32)SWAP32(x) )

#define NTOH16(x)                                                             \
           ( (uint16)SWAP16(x) )

#else
#error Unknown endian 
#endif /* ENDIAN_... */

#define HTON32(x)                                                             \
           NTOH32(x)

#define HTON16(x)                                                             \
           NTOH16(x)

#if defined(ADDRESS_32BIT)

#define SWAP_PTR(x)                                                           \
           SWAP32((uint32)(x))

#elif defined(ADDRESS_64BIT)

#define SWAP_PTR(x)                                                           \
           SWAP64((uint64)(x))

#else
#error Unknown pointer size
#endif /* ADDRESS_... */

/*@@DEC2INT
 *
 * Converts decimal character to integer
 *
 * C/C++ Syntax:   
 * bool 
 *    DEC2INT(         
 *       uint*   IN OUT   i,
 *       char    IN       c);
 *
 * Parameters:     i              integer to accumulate result in 
 *                 c              char to recognize
 *
 * Return:         true           if converted successfully
 *                 false          if failed
 *
 */
#define /* bool */ DEC2INT(                                                   \
                      /* uint*   IN OUT */   i,                               \
                      /* char    IN     */   c)                               \
       (                                                                      \
          (((c) >= '0') && ((c) <= '9')) ?                                    \
             *(i) = (*(i) * 10) + c - '0', true : false                       \
       )
       
/*@@HEX2INT
 *
 * Converts hex character to integer
 *
 * C/C++ Syntax:   
 * bool 
 *    HEX2INT(         
 *       uint*   IN OUT   i,
 *       char    IN       c);
 *
 * Parameters:     i              integer to accumulate result in 
 *                 c              char to recognize
 *
 * Return:         true           if converted successfully
 *                 false          if failed
 *
 */
#define /* bool */ HEX2INT(                                                   \
                      /* uint*   IN OUT */   i,                               \
                      /* char    IN     */   c)                               \
       (                                                                      \
          (((c) >= '0') && ((c) <= '9')) ?                                    \
             *(i) = (*(i) << 4) + c - '0', true :                             \
          (((c) >= 'A') && ((c) <= 'F')) ?                                    \
             *(i) = (*(i) << 4) + c - 'A' + 10, true :                        \
          (((c) >= 'a') && ((c) <= 'f')) ?                                    \
             *(i) = (*(i) << 4) + c - 'a' + 10, true :                        \
             false                                                            \
       )

/*@@BYTE2HEX
 *
 * Converts byte to pair of hex characters
 *
 * C/C++ Syntax:   
 * bool 
 *    BYTE2HEX(                                             
 *       char*   IN OUT   _p,                              
 *       uint    IN       _l,                              
 *       byte    IN       _b);
 *
 * Parameters:     _p             char buf pointer
 *                 _l             char buf size
 *                 _b             byte to convert from
 *
 * Return:         true           if converted successfully
 *                 false          if failed
 *
 */
#define /* bool */ BYTE2HEX(                                                  \
                      /* char*   IN OUT */   _p,                              \
                      /* uint    IN     */   _l,                              \
                      /* byte    IN     */   _b)                              \
       (                                                                      \
          ((_l) < 2) ? false :                                                \
             (_p[0] = (byte)(((_b)>>4) & 0x0F),                               \
              _p[0] = (byte)((_p[0] > 9) ? _p[0]+'A'-10 : _p[0]+'0'),         \
              _p[1] = (byte)(((_b)>>0) & 0x0F),                               \
              _p[1] = (byte)((_p[1] > 9) ? _p[1]+'A'-10 : _p[1]+'0'),         \
              true)                                                           \
       )


/******************************************************************************
 *   Octet string conversions
 */

#define U16_OS_B(p, u)          /* uint16 to octet string, big endian */      \
           {                                                                  \
              p[0] = (byte)((u) >>  8);                                       \
              p[1] = (byte) (u);                                              \
              p += sizeof(uint16);                                            \
           }

#define U16_OS_B_NO_STEP(p, u)  /* same, no pointer move */                   \
           {                                                                  \
              (p)[0] = (byte)((u) >>  8);                                     \
              (p)[1] = (byte) (u);                                            \
           }

#define U16_OS_L(p, u)          /* uint16 to octet string, little endian */   \
           {                                                                  \
              p[0] = (byte) (u);                                              \
              p[1] = (byte)((u) >>  8);                                       \
              p += sizeof(uint16);                                            \
           }

#define U16_OS_L_NO_STEP(p, u)  /* same, no pointer move */                   \
           {                                                                  \
              (p)[0] = (byte) (u);                                            \
              (p)[1] = (byte)((u) >>  8);                                     \
           }

#define OS_B_U16(u, p)          /* octet string to uint16, big endian */      \
           {                                                                  \
              u = (uint16)(((uint16)p[0] <<  8) | ((uint16)p[1]));            \
              p += sizeof(uint16);                                            \
           }

#define OS_B_U16_NO_STEP(u, p)  /* same, no pointer move */                   \
           {                                                                  \
              u = (uint16)(((uint16)(p)[0] <<  8) | ((uint16)(p)[1]));        \
           }

#define OS_L_U16(u, p)          /* octet string to uint16, little endian */   \
           {                                                                  \
              u = (uint16)(((uint16)p[1] <<  8) | ((uint16)p[0]));            \
              p += sizeof(uint16);                                            \
           }

#define OS_L_U16_NO_STEP(u, p)  /* same, no pointer move */                   \
           {                                                                  \
              u = (uint16)(((uint16)(p)[1] <<  8) | ((uint16)(p)[0]));        \
           }

#define U32_OS_B(p, u)          /* uint32 to octet string, big endian */      \
           {                                                                  \
              p[0] = (byte)((u) >> 24);                                       \
              p[1] = (byte)((u) >> 16);                                       \
              p[2] = (byte)((u) >>  8);                                       \
              p[3] = (byte) (u);                                              \
              p += sizeof(uint32);                                            \
           }

#define U32_OS_B_NO_STEP(p, u)  /* same, no pointer move */                   \
           {                                                                  \
              (p)[0] = (byte)((u) >> 24);                                     \
              (p)[1] = (byte)((u) >> 16);                                     \
              (p)[2] = (byte)((u) >>  8);                                     \
              (p)[3] = (byte) (u);                                            \
           }

#define U32_OS_L(p, u)          /* uint32 to octet string, little endian */   \
           {                                                                  \
              p[0] = (byte) (u);                                              \
              p[1] = (byte)((u) >>  8);                                       \
              p[2] = (byte)((u) >> 16);                                       \
              p[3] = (byte)((u) >> 24);                                       \
              p += sizeof(uint32);                                            \
           }

#define U32_OS_L_NO_STEP(p, u)  /* same, no pointer move */                   \
           {                                                                  \
              (p)[0] = (byte) (u);                                            \
              (p)[1] = (byte)((u) >>  8);                                     \
              (p)[2] = (byte)((u) >> 16);                                     \
              (p)[3] = (byte)((u) >> 24);                                     \
           }

#define OS_B_U32(u, p)          /* octet string to uint32, big endian */      \
           {                                                                  \
              u = ((uint32)p[0] << 24) | ((uint32)p[1] << 16) |               \
                  ((uint32)p[2] <<  8) | ((uint32)p[3]);                      \
              p += sizeof(uint32);                                            \
           }

#define OS_B_U32_NO_STEP(u, p)  /* same, no pointer move */                   \
           {                                                                  \
              u = ((uint32)(p)[0] << 24) | ((uint32)(p)[1] << 16) |           \
                  ((uint32)(p)[2] <<  8) | ((uint32)(p)[3]);                  \
           }

#define OS_B_U32_XOR(u, p, x)   /* octet string to uint32+xor, big endian */  \
           {                                                                  \
              u = ((uint32)p[0] << 24) ^ ((uint32)p[1] << 16) ^               \
                  ((uint32)p[2] <<  8) ^ ((uint32)p[3]) ^ (x);                \
              p += sizeof(uint32);                                            \
           }

#define OS_B_U32_XOR_NO_STEP(u, p, x)  /* same, no pointer move */            \
           {                                                                  \
              u = ((uint32)(p)[0] << 24) ^ ((uint32)(p)[1] << 16) ^           \
                  ((uint32)(p)[2] <<  8) ^ ((uint32)(p)[3]) ^(x);             \
           }

#define OS_L_U32(u, p)          /* octet string to uint32, little endian */   \
           {                                                                  \
              u = ((uint32)p[3] << 24) | ((uint32)p[2] << 16) |               \
                  ((uint32)p[1] <<  8) | ((uint32)p[0]);                      \
              p += sizeof(uint32);                                            \
           }

#define OS_L_U32_NO_STEP(u, p)  /* same, no pointer move */                   \
           {                                                                  \
              u = ((uint32)(p)[3] << 24) | ((uint32)(p)[2] << 16) |           \
                  ((uint32)(p)[1] <<  8) | ((uint32)(p)[0]);                  \
           }

#define U64_OS_B(p, u)          /* uint64 to octet string, big endian */      \
           {                                                                  \
              p[0] = (byte)((u) >> 56);                                       \
              p[1] = (byte)((u) >> 48);                                       \
              p[2] = (byte)((u) >> 40);                                       \
              p[3] = (byte)((u) >> 32);                                       \
              p[4] = (byte)((u) >> 24);                                       \
              p[5] = (byte)((u) >> 16);                                       \
              p[6] = (byte)((u) >>  8);                                       \
              p[7] = (byte) (u);                                              \
              p += sizeof(uint64);                                            \
           }

#define U64_OS_B_NO_STEP(p, u)  /* same, no pointer move */                   \
           {                                                                  \
              (p)[0] = (byte)((u) >> 56);                                     \
              (p)[1] = (byte)((u) >> 48);                                     \
              (p)[2] = (byte)((u) >> 40);                                     \
              (p)[3] = (byte)((u) >> 32);                                     \
              (p)[4] = (byte)((u) >> 24);                                     \
              (p)[5] = (byte)((u) >> 16);                                     \
              (p)[6] = (byte)((u) >>  8);                                     \
              (p)[7] = (byte) (u);                                            \
           }

#define OS_B_U64(u, p)          /* octet string to uint64, big endian */      \
           {                                                                  \
              u = ((uint64)p[0] << 56) | ((uint64)p[1] << 48) |               \
                  ((uint64)p[2] << 40) | ((uint64)p[3] << 32) |               \
                  ((uint64)p[4] << 24) | ((uint64)p[5] << 16) |               \
                  ((uint64)p[6] <<  8) | ((uint64)p[7] <<  0);                \
              p += sizeof(uint64);                                            \
           }

#define OS_B_U64_NO_STEP(u, p)  /* same, no pointer move */                   \
           {                                                                  \
              u = ((uint64)p[0] << 56) | ((uint64)p[1] << 48) |               \
                  ((uint64)p[2] << 40) | ((uint64)p[3] << 32) |               \
                  ((uint64)p[4] << 24) | ((uint64)p[5] << 16) |               \
                  ((uint64)p[6] <<  8) | ((uint64)p[7] <<  0);                \
           }

#define OS_B_U64_XOR(u, p, x)   /* octet string to uint64+xor, big endian */  \
           {                                                                  \
              u = ((uint64)p[0] << 56) ^ ((uint64)p[1] << 48) ^               \
                  ((uint64)p[2] << 40) ^ ((uint64)p[3] << 32) ^               \
                  ((uint64)p[4] << 24) ^ ((uint64)p[5] << 16) ^               \
                  ((uint64)p[6] <<  8) ^ ((uint64)p[7] <<  0) ^ (x);          \
              p += sizeof(uint64);                                            \
           }

#define OS_B_U64_XOR_NO_STEP(u, p, x)  /* same, no pointer move */            \
           {                                                                  \
              u = ((uint64)p[0] << 56) ^ ((uint64)p[1] << 48) ^               \
                  ((uint64)p[2] << 40) ^ ((uint64)p[3] << 32) ^               \
                  ((uint64)p[4] << 24) ^ ((uint64)p[5] << 16) ^               \
                  ((uint64)p[6] <<  8) ^ ((uint64)p[7] <<  0) ^ (x);          \
           }

#if defined (PLATFORM_32BIT)
#define D_OS_B(p, d)            /* digit to octet string, big endian */       \
           { U32_OS_B(p, (d)); } 
#else
#define D_OS_B(p, d)            /* digit to octet string, big endian */       \
           { D_OS_B_COUNT(p, sizeof(digit), (d)); }
#endif

#define D_OS_B_COUNT(p, c, d)   /* same, counted output */                    \
           {                                                                  \
              usize _i;                                                       \
              for (_i=(c); _i>0; _i--, *(p++) = (byte)((d) >> (byte)(_i*8))); \
           }

#if defined (PLATFORM_32BIT)
#define D_OS_L(p, d)            /* digit to octet string, little endian */    \
           { U32_OS_L(p, (d)); } 
#else
#define D_OS_L(p, d)            /* digit to octet string, little endian */    \
           { D_OS_L_COUNT(p, sizeof(digit), (d)); }
#endif

#define D_OS_L_COUNT(p, c, d)   /* same, counted output */                    \
           {                                                                  \
              usize _i;                                                       \
              digit _d;                                                       \
              for (_i=(c),_d=(d); _i>0; _i--, *(p++) = (byte)_d, _d>>=8);     \
           }

#if defined (PLATFORM_32BIT)
#define OS_B_D(d, p)            /* octet string to digit, big endian */       \
           { OS_B_U32(d, p); } 
#else
#define OS_B_D(d, p)            /* octet string to digit, big endian */       \
           { OS_B_D_COUNT(d, p, sizeof(digit)); }
#endif

#define OS_B_D_COUNT(d, p, c)   /* same, counted output */                    \
           {                                                                  \
              usize _i;                                                       \
              for (d=0, _i=(c); _i>0; _i--, d = (digit)((d << 8) | *(p++)));  \
           }

#if defined (PLATFORM_32BIT)
#define OS_L_D(d, p)            /* octet string to digit, little endian */    \
           { OS_L_U32(d, p); } 
#else
#define OS_L_D(d, p)            /* octet string to digit, little endian */    \
           { OS_L_D_COUNT(d, p, sizeof(digit)); }
#endif

#define OS_L_D_COUNT(d, p, c)   /* same, counted output */                    \
           {                                                                  \
              usize _i;                                                       \
              for (d=0,_i=0; _i<(c); _i++, d |= (digit)*(p++)<<(byte)(_i*8)); \
           }

#if defined (PLATFORM_32BIT)
#define U_OS_B(p, u)            /* uint to octet string, big endian */        \
           { U32_OS_B(p, (u)); } 
#else
#define U_OS_B(p, u)            /* uint to octet string, big endian */        \
           { U_OS_B_COUNT(p, sizeof(uint), (u)); }
#endif

#define U_OS_B_COUNT(p, c, u)   /* same, counted output */                    \
           {                                                                  \
              usize _i;                                                       \
              for (_i=(c); _i>0; _i--, *(p++) = (byte)((u) >> (byte)(_i*8))); \
           }

#if defined (PLATFORM_32BIT)
#define U_OS_L(p, u)            /* uint to octet string, little endian */     \
           { U32_OS_L(p, (u)); } 
#else
#define U_OS_L(p, u)            /* uint to octet string, little endian */     \
           { U_OS_L_COUNT(p, sizeof(uint), (u)); }
#endif

#define U_OS_L_COUNT(p, c, u)   /* same, counted output */                    \
           {                                                                  \
              usize _i;                                                       \
              unumber _u;                                                     \
              for (_i=(c),_u=(u); _i>0; _i--, *(p++) = (byte)_u, _u>>=8);     \
           }

#if defined (PLATFORM_32BIT)
#define OS_B_U(u, p)            /* octet string to uint, big endian */        \
           { OS_B_U32(u, p); } 
#else
#define OS_B_U(u, p)            /* octet string to uint, big endian */        \
           { OS_B_U_COUNT(u, p, sizeof(unumber)); }
#endif

#define OS_B_U_COUNT(u, p, c)   /* same, counted output */                    \
           {                                                                  \
              usize _i;                                                       \
              for (u=0, _i=(c); _i>0; _i--, u = (unumber)((u << 8) | *(p++)));\
           }

#if defined (PLATFORM_32BIT)
#define OS_L_U(u, p)            /* octet string to uint, little endian */     \
           { OS_L_U32(u, p); } 
#else
#define OS_L_U(u, p)            /* octet string to uint, little endian */     \
           { OS_L_U_COUNT(u, p, sizeof(unumber)); }
#endif

#define OS_L_U_COUNT(u, p, c)   /* same, counted output */                    \
           {                                                                  \
              usize _i;                                                       \
              for (u=0,_i=0; _i<(c); _i++, u|=(unumber)*(p++)<<(byte)(_i*8)); \
           }

#define PTR_OS(p, ptr)          /* pointer to octet string */                 \
           MemCpy((p), &(ptr), sizeof(void*)), (p) += sizeof(void*)

#define OS_PTR(ptr, p)          /* octet string to pointer */                 \
           MemCpy(&(ptr), (p), sizeof(void*)), (p) += sizeof(void*)


/******************************************************************************
 *   System/RTL call wrappers
 */

#define Clock()                                                               \
           ( PORTABLE_CLOCK() )
#define Delay(x)                                                              \
           PORTABLE_DELAY((x))
#define IsDigit(x)                                                            \
           ( ((x) >= '0') && ((x) <= '9') )
#define IsXDigit(x)                                                           \
           (                                                                  \
             (((x) >= '0') && ((x) <= '9'))                                   \
                ||                                                            \
             (((x) >= 'A') && ((x) <= 'F'))                                   \
                ||                                                            \
             (((x) >= 'a') && ((x) <= 'f'))                                   \
           )
#define MemCmp(pd, ps, c)                                                     \
           ( PORTABLE_MEMCMP((pd), (ps), (c)) )
#define MemCpy(pd, ps, c)                                                     \
           ( PORTABLE_MEMCPY((pd), (ps), (c)) )
#define MemMove(p, b, c)                                                      \
           ( PORTABLE_MEMMOVE((p), (b), (c)) )
#define MemSet(p, b, c)                                                       \
           ( PORTABLE_MEMSET((p), (b), (c)) )
#define MkTime(t)                                                             \
           ( PORTABLE_MKTIME((t)) )
#define Rand()                                                                \
           ( PORTABLE_RAND() )

#if defined(_MSC_VER)
#define Sprintf PORTABLE_SPRINTF
#else
#define Sprintf(_s, _c, ...)                                                  \
           ( PORTABLE_SPRINTF((_s), (_c), __VA_ARGS__) )
#endif

#define Srand(s)                                                              \
           ( PORTABLE_SRAND((s)) )
#define StrChr(p, ch)                                                         \
           ( PORTABLE_STRCHR((p), (ch)) )
#define StrCmp(p1, p2)                                                        \
           ( PORTABLE_STRCMP((p1), (p2)) )
#define StrNChr(p, ch, n)                                                     \
           ( PORTABLE_STRNCHR((p), (ch), (n)) )
#define StrICmp(p1, p2)                                                       \
           ( PORTABLE_STRICMP((p1), (p2)) )
#define StrLen(p)                                                             \
           ( PORTABLE_STRLEN((p)) )
#define StrNICmp(p1, p2, n)                                                   \
           ( PORTABLE_STRNICMP((p1), (p2), (n)) )
#define TICKS_PER_SEC                                                         \
           ( PORTABLE_TICKS_PER_SEC )
#define Time(t)                                                               \
           ( PORTABLE_TIME((t)) )
#define TimeEx(tm)                                                            \
           ( PORTABLE_TIME_EX((tm)) )


/******************************************************************************
 *   I/O support
 */

#define Stdout                                                                \
           PORTABLE_STDOUT
#define Stderr                                                                \
           PORTABLE_STDERR
#define Stdnul                                                                \
           PORTABLE_STDNUL

#define Fopen(fname, fmode)                                                   \
           ( PORTABLE_FOPEN((fname), (fmode)) )
#define Fgets(buf, cnt, f)                                                    \
           ( PORTABLE_FGETS((buf), (cnt), (f)) )
#define Fsize(f)                                                              \
           ( PORTABLE_FSIZE((f)) )
#define Fseek(f, ofs, org)                                                    \
           ( PORTABLE_FSEEK((f), (ofs), (org)) )
#define Ftell(f)                                                              \
           ( PORTABLE_FTELL((f)) )
#define Fread(buf, item, items, f)                                            \
           ( PORTABLE_FREAD((buf), (item), (items), (f)) )
#define Fwrite(buf, item, items, f)                                           \
           ( PORTABLE_FWRITE((buf), (item), (items), (f)) )
#define Fclose(f)                                                             \
           ( PORTABLE_FCLOSE((f)) )

#ifdef USE_DEBUG
#define Fflush(f)                                                             \
           ( PORTABLE_FFLUSH((f)) )
#if defined(_MSC_VER)
#define Fprintf PORTABLE_FPRINTF
#else    
#define Fprintf(f, ...)                                                       \
           ( PORTABLE_FPRINTF(f, __VA_ARGS__) ) 
#endif
#else
#define Fflush(f)                                                             
#if defined(_MSC_VER) && (_MSC_VER <= 1200)
#define Fprintf   fprintf
#else    
#define Fprintf(f, fmt, ...)                                                      
#endif
#endif /* USE_DEBUG */


/******************************************************************************
 *   Memory allocation support
 */

#ifdef USE_MALLOC
#define Malloc(c)                                                             \
           ( PORTABLE_MALLOC((c)) )
#define Free(p)                                                               \
           ( PORTABLE_FREE((p)) )
#endif 

/******************************************************************************
 *   Error handling
 */

/*
 * Error context, maintains latest error in current thread
 *
 */
typedef struct err_ctx_s {
#ifdef USE_DEBUG
   const char*   file;                      /* File-origin of error          */
   unumber       line;                      /* Line-origin of error          */
#endif
   int           err;                       /* Generic error code            */
} err_ctx_t;

extern err_ctx_t*
   err_get_context(
      void);

#define GET_ERR_CONTEXT err_get_context()

#ifdef USE_DEBUG

#define ERR_SET(e)                                                            \
           {                                                                  \
              err_ctx_t* _perr = GET_ERR_CONTEXT;                             \
              _perr->file = __FILE__;                                         \
              _perr->line = __LINE__;                                         \
              _perr->err  = (e);                                              \
              return false;                                                   \
           }

#define ERR_SET_NO_RET(e)                                                     \
           (                                                                  \
              GET_ERR_CONTEXT->file = __FILE__,                               \
              GET_ERR_CONTEXT->line = __LINE__,                               \
              GET_ERR_CONTEXT->err  = (e)                                     \
           )

#else

#define ERR_SET(e)                                                            \
           {                                                                  \
              err_ctx_t* _perr = GET_ERR_CONTEXT;                             \
              _perr->err  = (e);                                              \
              return false;                                                   \
           }

#define ERR_SET_NO_RET(e)                                                     \
           (                                                                  \
              GET_ERR_CONTEXT->err  = (e)                                     \
           )

#endif /* USE_DEBUG */

/*
 * Error info, binds code to textual description
 *
 */
typedef struct err_info_s {
   int           err;                  /* Error code                         */
   const char*   txt;                  /* Error description                  */
} err_info_t;

/*@@err_get_description
 *
 * Returns error description by its code
 *
 * Parameters:     code           error code to find
 *
 * Return:         description text
 * 
 */
const char*
   err_get_description(
      int   IN   code);

#define ERR_DEFINE(module, code)                                              \
        ( (int)(((module) & 0xFFFF) | ((int)code << 16)) )


/******************************************************************************
 *   Base errors
 */

#define MODULE_BASE   0

/*     
 * Error codes (basic set)
 *
 */                                                                          
typedef enum base_err_e {
   /* No error */                                 
   err_none 
      /*<?*/ = ERR_DEFINE(MODULE_BASE,  0) /*?>*/,
   /* Bad parameter was passed to function */
   err_bad_param         
      /*<?*/ = ERR_DEFINE(MODULE_BASE,  1) /*?>*/,
   /* No free heap */ 
   err_no_memory
      /*<?*/ = ERR_DEFINE(MODULE_BASE,  2) /*?>*/, 
   /* Invalid pointer was passed to function */  
   err_invalid_pointer   
      /*<?*/ = ERR_DEFINE(MODULE_BASE,  3) /*?>*/,
   /* Heap is corrupted */
   err_heap_corrupted    
      /*<?*/ = ERR_DEFINE(MODULE_BASE,  4) /*?>*/,
   /* Value out of bounds */
   err_out_of_bounds     
      /*<?*/ = ERR_DEFINE(MODULE_BASE,  5) /*?>*/,
   /* Unexpected function call */
   err_unexpected_call   
      /*<?*/ = ERR_DEFINE(MODULE_BASE,  6) /*?>*/,
   /* Used object is bad (corrupted) */
   err_bad_object        
      /*<?*/ = ERR_DEFINE(MODULE_BASE,  7) /*?>*/,
   /* Feature is not supported */
   err_not_supported     
      /*<?*/ = ERR_DEFINE(MODULE_BASE,  8) /*?>*/,
   /* Output buffer is too small */
   err_buffer_too_small  
      /*<?*/ = ERR_DEFINE(MODULE_BASE,  9) /*?>*/,
   /* Bad length value */
   err_bad_length        
      /*<?*/ = ERR_DEFINE(MODULE_BASE, 10) /*?>*/,
   /* Data is corrupted */ 
   err_data_corrupted    
      /*<?*/ = ERR_DEFINE(MODULE_BASE, 11) /*?>*/,
   /* Internal error */
   err_internal          
      /*<?*/ = ERR_DEFINE(MODULE_BASE, 12) /*?>*/
} base_err_t;


/******************************************************************************
 *   Debugging features
 */

#ifdef USE_DEBUG

#define DUMP_INDENT_FILE(f, ind)                                              \
           {                                                                  \
              unumber _j;                                                     \
              for (_j=0; _j<(ind); _j++)                                      \
                 Fprintf((f), "  ");                                          \
           }

#define DUMP_FILE(f, p, c, ind)                                               \
           {                                                                  \
              usize _i;                                                       \
              for (_i=0; _i<(c); _i++) {                                      \
                 if (_i%16 == 0)                                              \
                    DUMP_INDENT_FILE((f), (ind));                             \
                 Fprintf((f), "%02X%c", (byte)(p)[_i], ((_i+1)%16)?' ':'\n'); \
              }                                                               \
              if (_i%16 != 0)                                                 \
                 Fprintf((f), "\n");                                          \
           }

/*@@ERR_PRINT
 *
 * Prints last error information to console
 *
 * Parameters:     none
 *
 * Return:         none
 * 
 */
#define ERR_PRINT                                                             \
           {                                                                  \
              err_ctx_t* _perr = GET_ERR_CONTEXT;                             \
              if ((_perr != NULL) && (_perr->err == err_none)) {              \
                 Fprintf(Stdout, "\nNo errors\n");                            \
              }                                                               \
              else                                                            \
              if (_perr != NULL) {                                            \
                 Fprintf(                                                     \
                    Stdout,                                                   \
                    "\nError! %s, file %s, line %d\n",                        \
                    err_get_description(_perr->err),                          \
                    _perr->file,                                              \
                    _perr->line);                                             \
              }                                                               \
           }

#else
#define DUMP_INDENT_FILE(f, ind)                                              
#define DUMP_FILE(f, p, c, ind)
#define ERR_PRINT
#endif /* USE_DEBUG */


/******************************************************************************
 *   Synchronization
 */

/*
 * Generic mutex object
 *
 */
#define generic_mutex_t portable_mutex_t

/*@@sync_interlocked_exchange32
 *
 * Performs 32-bit interlocked_exchange primitive
 *
 * C/C++ Syntax:   
 * uint32  
 *   sync_interlocked_exchange32(
 *     uint32*   IN OUT   shared,
 *     uint32    IN       exchange);
 *
 * Parameters:     shared         points to shared variable
 *                 exchange       value to exchange
 *
 * Return:         old value of shared variable
 *
 */
#define /* uint32 */ sync_interlocked_exchange32(                             \
                        /* uint32*   IN OUT */   shared,                      \
                        /* uint32    IN     */   exchange)                    \
       PORTABLE_INTERLOCKED_EXCHANGE32((shared), (exchange)) 

/*@@sync_mutex_create
 *
 * Initializes a mutex object
 *
 * C/C++ Syntax:   
 * void 
 *    sync_mutex_create(                                             
 *       generic_mutex_t*   IN OUT   pmux);
 *
 * Parameters:     pmux           pointer to a mutex object
 *
 * Return:         none
 *
 */
#define /* void */ sync_mutex_create(                                         \
                      /* generic_mutex_t*   IN OUT */   pmux)                 \
       PORTABLE_MUTEX_CREATE((pmux)) 
          
/*@@sync_mutex_destroy
 *
 * Releases a mutex object
 *
 * C/C++ Syntax:   
 * void 
 *    sync_mutex_destroy(                                             
 *       generic_mutex_t*   IN OUT   pmux);
 *
 * Parameters:     pmux           pointer to a mutex object
 *
 * Return:         none
 *
 */
#define /* void */ sync_mutex_destroy(                                        \
                      /* generic_mutex_t*   IN OUT */   pmux)                 \
       PORTABLE_MUTEX_DESTROY((pmux)) 

/*@@sync_mutex_lock
 *
 * Locks a mutex object
 *
 * C/C++ Syntax:   
 * void 
 *    sync_mutex_lock(                                             
 *       generic_mutex_t*   IN OUT   pmux);
 *
 * Parameters:     pmux           pointer to a mutex object
 *
 * Return:         none
 *
 */
#define /* void */ sync_mutex_lock(                                           \
                      /* generic_mutex_t*   IN OUT */   pmux)                 \
       PORTABLE_MUTEX_LOCK((pmux)) 
       
/*@@sync_mutex_unlock
 *
 * Unlocks a mutex object
 *
 * C/C++ Syntax:   
 * void 
 *    sync_mutex_lock(                                             
 *       generic_mutex_t*   IN OUT   pmux);
 *
 * Parameters:     pmux           pointer to a mutex object
 *
 * Return:         none
 *
 */
#define /* void */ sync_mutex_unlock(                                         \
                      /* generic_mutex_t*   IN OUT */   pmux)                 \
       PORTABLE_MUTEX_UNLOCK((pmux)) 


/******************************************************************************
 *   Module handling
 */

struct heap_ctx_s;

/*
 * Default system heap size
 *
 */
#define DEF_SYS_HEAP_SIZE  8192

/*
 * Initialization function 
 *
 * Parameters:     heap           heap to use
 *
 * Return:         true           if initialized successfully
 *                 false          if failed
 *
 */
typedef 
   bool init_func_t(
      struct heap_ctx_s*   IN OUT   heap);

/*
 * Destruction function
 *
 * Parameters:     none
 *
 * Return:         true           if destroyed successfully
 *                 false          if failed
 *
 */
typedef 
   bool done_func_t(
      void);

/*
 * Submodule descriptor
 *
 */
typedef struct submodule_info_s {
   const char*    txt;                        /* Description                 */
   init_func_t*   init;                       /* Initializer                 */
   done_func_t*   done;                       /* Destroyer                   */
} submodule_info_t;

/*
 * Module descriptor
 *
 */
typedef struct module_info_s {
   unumber                          id;       /* Module Id                   */
   const char*                      txt;      /* Description                 */
   init_func_t*                     init;     /* Initializer                 */
   done_func_t*                     done;     /* Destroyer                   */
   const submodule_info_t* const*   psub;     /* Submodule info array        */
   usize                            csub;     /* Above array size, items     */
   const err_info_t*                perr;     /* Error info array            */
   usize                            cerr;     /* Above array size, items     */
   struct module_info_s*            next;     /* Next module                 */
} module_info_t;

/*@@module_register
 *
 * Registers module in framework
 *
 * Parameters:     module         module info pointer
 *
 * Return:         true           if successful
 *                 false          if failed
 * 
 */
bool
   module_register(
      module_info_t*   IN   module);


#ifdef __cplusplus
}
#endif


#endif

