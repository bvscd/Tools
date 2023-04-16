#ifndef _EMB_CODR_DEFINED
#define _EMB_CODR_DEFINED

#include "emb_buff.h"

#ifdef __cplusplus
extern "C" {
#endif


/******************************************************************************
 *   UTF-8 coder/decoder
 */

/*@@utf8_estimate_u16_encoding
 *
 * Estimates UTF-8 encoding size of single UNICODE-16 character
 *
 * C/C++ Syntax:   
 * uint 
 *    utf8_estimate_u16_encoding(                                
 *       unicode   IN   u16);
 *
 * Parameters:     u16            unicode16 char
 *
 * Return:         UTF-8 encoding size, bytes
 *
 */
#define /* uint */ utf8_estimate_u16_encoding(                                \
                      /* unicode   IN */   u16)                               \
       ( ((u16) < 0x0080) ? 1 : ((u16) < 0x0800) ? 2 : 3 )

/*@@utf8_encode_u16_char
 *
 * Encodes single UNICODE-16 character to UTF-8
 *
 * Parameters:     putf8          pointer to the buffer for UTF-8
 *                 cutf8          on entry: UTF-8 buffer size;
 *                 cutf8          on exit: consumed size
 *                 u16            unicode16 char
 *
 * Return:         true           if successful
 *                 false          if failed
 *
 */
bool 
   utf8_encode_u16_char(                                            
      byte*            IN OUT   putf8,
      usize*           IN OUT   cutf8,
      unicode          IN       u16);

/*@@utf8_encode_u16
 *
 * Encodes UNICODE-16 string to UTF-8
 *
 * Parameters:     utf8           pointer to the buffer for UTF-8
 *                 pu16           pointer to unicode16 data
 *                 cu16           length of above data, unicode chars
 *
 * Return:         true           if successful
 *                 false          if failed
 *
 */
bool 
   utf8_encode_u16(                                            
      buf_t*           IN OUT   utf8,
      const unicode*   IN       pu16,
      usize            IN       cu16);

/*@@utf8_decode_u16
 *
 * Decodes UTF-8 string to UNICODE-16
 *
 * Parameters:     ok             success flag
 *                 u16            pointer to the buffer for UNICODE-16
 *                 putf8          pointer to UTF-8 data
 *                 cutf8          length of above data, bytes
 *
 * Return:         true           if successful
 *                 false          if failed
 *
 */
bool 
   utf8_decode_u16(                                            
      bool*         OUT      ok,                              
      buf_t*        IN OUT   u16,
      const byte*   IN       putf8,
      usize         IN       cutf8);

/*@@utf8_validate_u16
 *
 * Validates UTF-8 sequence containing UNICODE-16 chars
 *
 * Parameters:     ok             success flag
 *                 putf8          pointer to UTF-8 data
 *                 cutf8          length of above data, bytes
 *
 * Return:         number of UNICODE-16 characters inside encoding
 *
 */
usize
   utf8_validate_u16(                                            
      bool*         OUT   ok,
      const byte*   IN    putf8,
      usize         IN    cutf8);


/******************************************************************************
 *   Base64 encoder/decoder
 */

/*
 * Base64 encoder/decoder context
 *
 */
typedef struct b64_ctx_s {
   unumber   state;          /* coder state                                  */
   unumber   cline;          /* zero or limit of encoded line length, atoms  */
   usize     atoms;          /* encoded line length, atoms                   */
   umask     flags;          /* flags                                        */
} b64_ctx_t;

/*
 * Base64 flags
 *
 */
typedef enum b64_flags_e {
   b64_url_safe = 1,         /* "URL and filename safe" Base64 encoding      */
} b64_flags_t;

/*@@b64_init
 *
 * Initializes Base64 coder context
 *
 * Parameters:     pctx           Base64 context to initialize
 *                 encode         encoding/decoding flag
 *                 cline          zero or encoded line length (with CRF), chars
 *                 flags          processing flags (see b64_flags_t)
 *
 * Return:         true           if successful
 *                 false          if failed
 *
 */
bool
   b64_init(
      b64_ctx_t*    IN OUT   ctx,
      bool          IN       encode,
      unumber       IN       cline,
      b64_flags_t   IN       flags);

/*@@b64_encode
 *
 * Base64 encoding (RFC4648)
 *
 * Parameters:     ctx            Base64 context
 *                 pdst           destination buffer or NULL to estimate
 *                 cdst           output buffer/data size, octets
 *                 psrc           source buffer
 *                 csrc           on entry: source buffer size, octets;
 *                 csrc           on exit: processed bytes counter
 *                 final          final portion flag
 *
 * Return:         true           if successful
 *                 false          if failed
 *
 */
bool
   b64_encode(
      b64_ctx_t*    IN OUT   ctx,
      char*         OUT      pdst,
      usize*        IN OUT   cdst,
      const byte*   IN       psrc,
      usize*        IN OUT   csrc,
      bool          IN       final);

/*@@b64_decode
 *
 * Base64 decoding (RFC4648)
 *
 * Parameters:     ok             success flag
 *                 ctx            Base64 context
 *                 pdst           destination buffer or NULL to estimate
 *                 cdst           output buffer/data size, octets
 *                 psrc           source buffer
 *                 csrc           on entry: source buffer size, octets
 *                 csrc           on exit: processed bytes counter
 *                 final          final portion flag
 *
 * Return:         true           if successful
 *                 false          if failed
 *
 */
bool
   b64_decode(
      bool*         OUT      ok, 
      b64_ctx_t*    IN OUT   ctx,
      byte*         OUT      pdst,
      usize*        IN OUT   cdst,
      const char*   IN       psrc,
      usize*        IN OUT   csrc,
      bool          IN       final);


/******************************************************************************
 *   Base32 encoder/decoder
 */

/*
 * Base32 encoder/decoder context
 *
 */
typedef struct b32_ctx_s {
   unumber   state;          /* coder state                                  */
   unumber   cline;          /* zero or limit of encoded line length, atoms  */
   usize     atoms;          /* encoded line length, atoms                   */
   umask     flags;          /* flags                                        */
} b32_ctx_t;

/*
 * Base32 flags
 *
 */
typedef enum b32_flags_e {
   b32_ignore_case = 1,      /* case-insensitive decoding                     */
} b32_flags_t;

/*@@b32_init
 *
 * Initializes Base32 coder context
 *
 * Parameters:     pctx           Base32 context to initialize
 *                 encode         encoding/decoding flag
 *                 cline          zero or encoded line length (with CRF), chars
 *                 flags          processing flags (see b32_flags_t)
 *
 * Return:         true           if successful
 *                 false          if failed
 *
 */
bool
   b32_init(
      b32_ctx_t*    IN OUT   ctx,
      bool          IN       encode,
      unumber       IN       cline,
      b32_flags_t   IN       flags);

/*@@b32_encode
 *
 * Base32 encoding (RFC4648)
 *
 * Parameters:     ctx            Base32 context
 *                 pdst           destination buffer or NULL to estimate
 *                 cdst           output buffer/data size, octets
 *                 psrc           source buffer
 *                 csrc           on entry: source buffer size, octets;
 *                 csrc           on exit: processed bytes counter
 *                 final          final portion flag
 *
 * Return:         true           if successful
 *                 false          if failed
 *
 */
bool
   b32_encode(
      b32_ctx_t*    IN OUT   ctx,
      char*         OUT      pdst,
      usize*        IN OUT   cdst,
      const byte*   IN       psrc,
      usize*        IN OUT   csrc,
      bool          IN       final);

/*@@b32_decode
 *
 * Base32 decoding (RFC4648)
 *
 * Parameters:     ok             success flag
 *                 ctx            Base32 context
 *                 pdst           destination buffer or NULL to estimate
 *                 cdst           output buffer/data size, octets
 *                 psrc           source buffer
 *                 csrc           on entry: source buffer size, octets
 *                 csrc           on exit: processed bytes counter
 *                 final          final portion flag
 *
 * Return:         true           if successful
 *                 false          if failed
 *
 */
bool
   b32_decode(
      bool*         OUT      ok, 
      b32_ctx_t*    IN OUT   ctx,
      byte*         OUT      pdst,
      usize*        IN OUT   cdst,
      const char*   IN       psrc,
      usize*        IN OUT   csrc,
      bool          IN       final);


#ifdef __cplusplus
}
#endif

#endif
