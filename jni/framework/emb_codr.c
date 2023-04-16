#include "emb_codr.h"

/******************************************************************************
 *   UTF-8 coder/decoder
 */

/*****************************************************************************/
bool 
   utf8_encode_u16_char(                                            
      byte*     IN OUT   putf8,
      usize*    IN OUT   cutf8,
      unicode   IN       u16)
/*
 * Encodes single UNICODE-16 character to UTF-8
 *
 */
{

   usize i;

   assert(putf8 != NULL);
   assert(cutf8 != NULL);

   i      = *cutf8;
   *cutf8 = 0;

   if (u16 < 0x0080) {
      if (i < 1) 
         ERR_SET(err_bad_param);
      putf8[0] = (byte)u16;
      *cutf8   = 1;
   }
   else
   if (u16 < 0x0800) {
      if (i < 2) 
         ERR_SET(err_bad_param);
      putf8[0] = (byte)(0xC0 | ((u16 >>  6) & 0x1F));
      putf8[1] = (byte)(0x80 | ((u16 >>  0) & 0x3F));
      *cutf8   = 2;
   }
   else {
      if (i < 3) 
         ERR_SET(err_bad_param);
      putf8[0] = (byte)(0xE0 | ((u16 >> 12) & 0x0F));
      putf8[1] = (byte)(0xC0 | ((u16 >>  6) & 0x3F));
      putf8[2] = (byte)(0x80 | ((u16 >>  0) & 0x3F));
      *cutf8   = 3;
   }
   
   return true;

}

/*****************************************************************************/
bool 
   utf8_encode_u16(                                            
      buf_t*           IN OUT   utf8,
      const unicode*   IN       pu16,
      usize            IN       cu16)
/*
 * Encodes UNICODE-16 string to UTF-8
 *
 */
{

   usize i, j, c;   
   byte* p;

   assert( utf8 != NULL);
   assert((pu16 != NULL) || (cu16 == 0));

   /*
    * Calculate size
    *
    */
   for (i=c=0; i<cu16; i++) 
      c += utf8_estimate_u16_encoding(pu16[i]);

   /*
    * Encode
    *
    */

   if (!buf_expand(c, utf8))
      return false;

   for (p=buf_get_ptr_bytes(utf8), i=c; cu16>0; cu16--, pu16++) {
      j = i;
      if (!utf8_encode_u16_char(p, &j, *pu16))
         return false;
      p += j;
      i -= j;
   }

   return buf_set_length(c, utf8);

}

/*****************************************************************************/
bool 
   utf8_decode_u16(               
      bool*         OUT      ok,                              
      buf_t*        IN OUT   u16,
      const byte*   IN       putf8,
      usize         IN       cutf8)
/*
 * Decodes UTF-8 string to UNICODE-16
 *
 */
{

   usize    c;   
   unicode* p;

   assert(ok    != NULL);
   assert(u16   != NULL);
   assert(putf8 != NULL);

   *ok = false;

   /*
    * Calculate size
    *
    */
   c = utf8_validate_u16(ok, putf8, cutf8);
   if (!*ok)
      return true;

   /*
    * Decode
    *
    */

   if (!buf_expand(c, u16))
      return false;

   for (p=buf_get_ptr_unicodes(u16); cutf8>0; ) {
      if (*putf8 < 0x80) {
         *(p++) = *putf8;
         putf8 += 1;
         cutf8 -= 1;
      }
      else
      if (*putf8 < 0xDF) {
         *(p++) = (unicode)(((putf8[0] & 0x1F) << 6) | 
                             (putf8[1] & 0x3F));
         putf8 += 2;
         cutf8 -= 2;
      }
      else {
         *(p++) = (unicode)(((putf8[0] & 0x0F) << 12) | 
                            ((putf8[1] & 0x3F) <<  6) | 
                             (putf8[2] & 0x3F));
         putf8 += 3;
         cutf8 -= 3;
      }
   }

   return buf_set_length(c, u16);

}


/*****************************************************************************/
usize
   utf8_validate_u16(                                            
      bool*         OUT   ok,
      const byte*   IN    putf8,
      usize         IN    cutf8)
/*
 * Validates UTF-8 sequence containing UNICODE-16 chars
 *
 */
{

   usize i, c;   

   assert( ok    != NULL);
   assert((putf8 != NULL) || (cutf8 == 0));

   *ok = false;

   /*
    * Check
    *
    */

   for (i=c=0; i<cutf8; c++) {
      if (putf8[i] < 0x80) {
         i += 1;
      }
      else
      if (putf8[i] < 0xDF) {
         if (cutf8-i < 2)
            return 0;
         if (putf8[i+1] > 0xBF)
            return 0;
         i += 2;
      }
      else {
         if (cutf8-i < 3)
            return 0;
         if (putf8[i] > 0xEF)
            return 0;
         if (putf8[i+1] > 0xBF)
            return 0;
         if (putf8[i+2] > 0xBF)
            return 0;
         i += 3;
      }
   }

   if (i != cutf8)
      return 0;

   *ok = true;
   return c;

}


/******************************************************************************
 *   Base64 encoder/decoder
 */

/*
 * Base64 constants
 *
 */
enum b64_constants_e {
   b64_size_encoded_atom = 4,                  /* base64 encoded atom size   */
   b64_size_decoded_atom = 3                   /* base64 decoded atom size   */
};

/*
 * Base64 encoder/decoder states.
 *
 */
enum b64_state_e {
   b64_state_idle,
   b64_state_encode,
   b64_state_decode,
   b64_state_eof,
   b64_state_error
};

/*
 * Standard Base64 encode table (alphabet), defined in RFC4648
 *
 */
static const char std_encode64_tab[] = 
{
   'A','B','C','D','E','F','G','H',
   'I','J','K','L','M','N','O','P',
   'Q','R','S','T','U','V','W','X',
   'Y','Z','a','b','c','d','e','f',
   'g','h','i','j','k','l','m','n',
   'o','p','q','r','s','t','u','v',
   'w','x','y','z','0','1','2','3',
   '4','5','6','7','8','9','+','/'
};

/*
 * Standard Base64 decode table (RFC4648)
 *
 */
static const char std_decode64_tab[] = 
{
   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63,
   52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1,
   -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
   15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,
   -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
   41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51 
};

/*
 * URL/filename-safe encode table (alphabet), defined in RFC4648
 *
 */
static const char urlsafe_encode64_tab[] = 
{
   'A','B','C','D','E','F','G','H',
   'I','J','K','L','M','N','O','P',
   'Q','R','S','T','U','V','W','X',
   'Y','Z','a','b','c','d','e','f',
   'g','h','i','j','k','l','m','n',
   'o','p','q','r','s','t','u','v',
   'w','x','y','z','0','1','2','3',
   '4','5','6','7','8','9','-','_'
};

/*
 * URL/filename-safe decode table (RFC4648)
 *
 */
static const char urlsafe_decode64_tab[] = 
{
   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1,
   52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1,
   -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
   15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, 63,
   -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
   41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51 
};

/*****************************************************************************/
static usize
   base64_encode_atom(
      char*         OUT   pd,
      const byte*   IN    ps,
      usize         IN    len,
      const char    IN    ptab[64])
/*
 * Encodes Base64 atom as specified in RFC4648.
 *
 * Parameters:     pd             NULL or Base64 atom buffer (4 chars size)
 *                 ps             data buffer to encode
 *                 len            data buffer length, octets
 *                 ptab           encode table, 64 characters
 *
 * Return:         number of processed bytes
 *
 */
{

   unumber a, b, c;

   if (len > b64_size_decoded_atom)
      len = b64_size_decoded_atom;
   if (pd == NULL)
      return len;

   a = (len > 0) ? *(ps++) : 0;
   b = (len > 1) ? *(ps++) : 0;
   c = (len > 2) ? *ps     : 0;

   *(pd++) = ptab[((a >> 2) & 0x3F)];
   *(pd++) = ptab[((a << 4) & 0x30) + ((b >> 4) & 0x0F)];

   switch (len) {
   case 1:
      *(pd++) = '=';
      *pd     = '=';
      break;
   case 2:
      *(pd++) = ptab[((b << 2) & 0x3C) + ((c >> 6) & 0x03)];
      *pd     = '=';
      break;
   default:
      *(pd++) = ptab[((b << 2) & 0x3C) + ((c >> 6) & 0x03)];
      *pd++   = ptab[(c & 0x3F)];
      len = b64_size_decoded_atom;
   }
   return len;

}

/*****************************************************************************/
static usize
   b64_decode_atom(
      byte*         OUT   pd,
      const char*   IN    ps,
      const char*   IN    ptab,
      usize         IN    ctab)
/*
 * Decodes Base64 atom as specified in rfc4648.
 *
 * Parameters:     pd             NULL or decoded data buffer (3 bytes size)
 *                 ps             Base64 atom, 4 printable chars
 *                 ptab           decode table
 *                 ctab           decode table length, octets
 *
 * Return:         zero if failed or number of processed bytes
 *
 */
{

   unumber t, i, len;
   unumber a[b64_size_encoded_atom];

   len = b64_size_encoded_atom;
   if (ps[3] == '=')
      len--;
   if (ps[2] == '=')
      len--;

   for (i=0; i<len; i++) {
      t = *(ps++);
      if ((t > ctab) || (ptab[t] & 0x80))   /* ((signed)ptab[t] < 0)) */
         return 0;
      a[i] = ptab[t];
   }
   len--;  /* cdata=catom-1*/

   if (pd == NULL)
      return len;

   switch (len) {
   default:
      pd[2] = (byte)(((a[2] << 6) & 0xC0) | ( a[3]       & 0x3F));
   case 2:
      pd[1] = (byte)(((a[1] << 4) & 0xF0) | ((a[2] >> 2) & 0x0F));
   case 1:
      pd[0] = (byte)(((a[0] << 2) & 0xFC) | ((a[1] >> 4) & 0x03));
   }
   return len;

}

/*****************************************************************************/
bool
   b64_init(
      b64_ctx_t*    IN OUT   ctx,
      bool          IN       encode,
      unumber       IN       cline,
      b64_flags_t   IN       flags)
/*
 * Initializes Base64 coder context
 *
 */
{

   assert(ctx != NULL);

   MemSet(ctx, 0x00, sizeof(*ctx));

   if (cline != 0) {
      if (cline < b64_size_encoded_atom+2)
         ERR_SET(err_bad_param);
      cline -= 2;
      cline /= b64_size_encoded_atom;
      ctx->cline = cline;
   }

   ctx->flags = flags;
   ctx->state = (encode) ? b64_state_encode : b64_state_decode;
   return true;

}

/*****************************************************************************/
bool
   b64_encode(
      b64_ctx_t*    IN OUT   ctx,
      char*         OUT      pdst,
      usize*        IN OUT   cdst,
      const byte*   IN       psrc,
      usize*        IN OUT   csrc,
      bool          IN       final)
/*
 * Base64 encoding (RFC4648)
 *
 */
{

   usize cbuf, i;
   const byte* ps = psrc;
   char* pd = pdst;
   const char* ptab;

   assert(ctx  != NULL);
   assert(csrc != NULL);

   /*
    * Sanity check
    *
    */

   if (ctx->state != b64_state_encode)
      ERR_SET(err_unexpected_call);
   if ((void*)pdst == (void*)psrc)
      ERR_SET(err_invalid_pointer);
   if ((*csrc > 0) && (psrc == NULL))
      ERR_SET(err_invalid_pointer);

   cbuf = 0;
   if (cdst != NULL) {
      cbuf  = *cdst;
      *cdst = 0;
   }
   if (pdst == NULL) {
      if (cdst == NULL)
         cdst = &cbuf;
      cbuf = (usize)-1;
   }

   if (cbuf < b64_size_encoded_atom)
      ERR_SET(err_buffer_too_small);

   /*
    * Encoding loop
    *
    */
   ptab = (ctx->flags & b64_url_safe) ? urlsafe_encode64_tab : std_encode64_tab;
   for (; (*csrc>0) && (cbuf>=b64_size_encoded_atom); ) {

      /*
       * End-of-data
       *
       */
      if (!final && (*csrc < b64_size_decoded_atom))
         break;

      /*
       * Line feed
       *
       */
      if ((ctx->cline != 0) && (ctx->atoms >= ctx->cline)) {
         if (pdst != NULL) {
            pd[0] = '\r';
            pd[1] = '\n';
         }
         pd   += 2;
         cbuf -= 2;
         ctx->atoms = 0;
         continue;
      }

      i = (pdst != NULL) ?
         base64_encode_atom(pd,   ps, *csrc, ptab) :
         base64_encode_atom(NULL, ps, *csrc, ptab);
      ps    += i;
      *csrc -= i;
      pd    += b64_size_encoded_atom;
      cbuf  -= b64_size_encoded_atom;
      ctx->atoms++;

   }

   if (final && (*csrc == 0))
      ctx->state = b64_state_eof;

   *cdst = pd - pdst;
   *csrc = ps - psrc;
   return true;

}

/*****************************************************************************/
bool
   b64_decode(
      bool*         OUT      ok, 
      b64_ctx_t*    IN OUT   ctx,
      byte*         OUT      pdst,
      usize*        IN OUT   cdst,
      const char*   IN       psrc,
      usize*        IN OUT   csrc,
      bool          IN       final)
/*
 * Base64 decoding (RFC4648)
 *
 */
{

   usize cbuf, ctab, len;
   byte* pd = pdst;
   const char* ps = psrc;
   unumber catom;
   char atom[b64_size_encoded_atom];

   const char* pt;
   const char* ptab;
   
   assert(ctx  != NULL);
   assert(csrc != NULL);
   assert(ok   != NULL);

   *ok = false;

   /*
    * Sanity check
    *
    */
   if (ctx->state == b64_state_error)
      return true;
   if (ctx->state != b64_state_decode)
      ERR_SET(err_unexpected_call);
   if ((*csrc > 0) && (psrc == NULL))
      ERR_SET(err_invalid_pointer);

   cbuf = 0;
   if (cdst != NULL) {
      cbuf  = *cdst;
      *cdst = 0;
   }
   if (pdst == NULL) {
      if (cdst == NULL)
         cdst = &cbuf;
      cbuf = (usize)-1;
   }
   if (cbuf < b64_size_encoded_atom)
      ERR_SET(err_buffer_too_small);

   /*
    * Decoding loop
    *
    */
   if (ctx->flags & b64_url_safe) {
      ptab = urlsafe_decode64_tab;
      ctab = sizeof(urlsafe_decode64_tab);
   }
   else {
      ptab = std_decode64_tab;
      ctab = sizeof(std_decode64_tab);
   }
   for (; (ctx->state==b64_state_decode) && (cbuf>0); ) { 

      /*
       * Load Base64 atom if it possible
       *
       */
      for (catom=0, pt=ps; 
          (*csrc>0) && (catom<b64_size_encoded_atom); 
          (*csrc)--) {
         const char t = *(pt++);
         switch (t) {
         case '\r':
         case '\n':
         case '\0':
         case ' ':
         case '\t':
            if (catom == 0)   /* skip ahead ignores */
               ps++;
            continue;
         default:
            break;
         }
         atom[catom++] = t;
      }
      if (catom < b64_size_encoded_atom)
         break;

      /*
       * Try decode
       *
       */
      len = b64_decode_atom(NULL, atom, ptab, ctab);
      if ((cbuf < b64_size_decoded_atom) && (cbuf < len))
         break;
      if (pdst != NULL) 
         len = b64_decode_atom(pd, atom, ptab, ctab);
      if (len == 0) {
         ctx->state = b64_state_error;
         if (ps != psrc)
            goto exit;
         ERR_SET(err_bad_param);
      }

      ps = pt;
      if (len < b64_size_decoded_atom)
         ctx->state = b64_state_eof;
      pd   += len;
      cbuf -= len;

   }

   if (final && (*csrc == 0))
      ctx->state = b64_state_eof;

   *ok = true;

exit:
   *cdst = pd - pdst;
   *csrc = ps - psrc;
   return true;

}


/******************************************************************************
 *   Base32 encoder/decoder
 */

/*
 * Base32 constants
 *
 */
enum b32_constants_e {
   b32_size_encoded_atom = 8,                  /* base32 encoded atom size   */
   b32_size_decoded_atom = 5                   /* base32 decoded atom size   */
};

/*
 * Base32 encoder/decoder states.
 *
 */
enum b32_state_e {
   b32_state_idle,
   b32_state_encode,
   b32_state_decode,
   b32_state_eof,
   b32_state_error
};

/*
 * Standard Base32 encode table (alphabet), defined in RFC4648
 *
 */
static const char std_encode32_tab[] = 
{
   'A','B','C','D','E','F','G','H',
   'I','J','K','L','M','N','O','P',
   'Q','R','S','T','U','V','W','X',
   'Y','Z','2','3','4','5','6','7'
};

/*
 * Standard Base32 decode table (RFC4648)
 *
 */
static const char std_decode32_tab[] = 
{
   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
   -1, -1, 26, 27, 28, 29, 30, 31, -1, -1, -1, -1, -1, -1, -1, -1,
   -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
   15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25
/*
 * standard decode table
 *
 */
                                             , -1, -1, -1, -1, -1,
   -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
   15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25
/*
 * case-insensitive decode table
 *
 */
};


/*****************************************************************************/
static usize
   base32_encode_atom(
      char*         OUT   pd,
      const byte*   IN    ps,
      usize         IN    len)
/*
 * Encodes Base32 atom as specified in RFC4648.
 *
 * Parameters:     pd             NULL or Base32 atom buffer (8 chars size)
 *                 ps             data buffer to encode
 *                 len            data buffer length, octets
 *
 * Return:         number of processed bytes
 *
 */
{
   
   usize i;
   unumber a, t;

   if (len>b32_size_decoded_atom)
      len = b32_size_decoded_atom;
   if (pd==NULL)
      return len;

   /* ps[0]; t is empty */
   a = *ps++;
   t = ((a >> 3) & 0x1f);
   *pd++ = std_encode32_tab[t];
   t = ((a << 2) & 0x1c);

   for (i=len-1; i>0; ) {
      /* ps[1]; ct=3,bits */
      a = *ps++;
      t |= ((a >> 6) & 0x03);
      *pd++ = std_encode32_tab[t];
      t = ((a << 4) & 0x10);
      a = ((a >> 1) & 0x1f);
      *pd++ = std_encode32_tab[a];
      if (--i == 0)
         break;
      /* ps[2]; ct=1,bits */
      a = *ps++;
      t |= ((a >> 4) & 0x0f);
      *pd++ = std_encode32_tab[t];
      t = (a << 1) & 0x1e;
      if (--i == 0)
         break;
      /* ps[3]; ct=4,bits */
      a = *ps++;
      t |= ((a >> 7) & 0x01);
      *pd++ = std_encode32_tab[t];
      t = ((a << 3) & 0x18);
      a = ((a >> 2) & 0x1f);
      *pd++ = std_encode32_tab[a];
      if (--i == 0)
         break;
      /* ps[4]; ct=2,bits */
      a = *ps;
      t |= ((a >> 5) & 0x07);
      *pd++ = std_encode32_tab[t];
      t = (a & 0x1f);
      break;
   }

   *pd++ = std_encode32_tab[t];
   if (len == b32_size_decoded_atom)
      return len;

   /*
    * cpad = b32_size_encoded_atom-((len*8)+4)/5;
    *
    */
   switch (len) {
   case b32_size_decoded_atom-4:  /* 1, cpad=6 */
      *pd++ = '=';
      *pd++ = '=';
   case b32_size_decoded_atom-3:  /* 2, cpad=4 */
      *pd++ = '=';
   case b32_size_decoded_atom-2:  /* 3, cpad=3 */
      *pd++ = '=';
      *pd++ = '=';
   case b32_size_decoded_atom-1:  /* 4, cpad=1 */
      *pd++ = '=';
   }
   return len;

}

/*****************************************************************************/
static usize
   b32_decode_atom(
      byte*         OUT   pd,
      const char*   IN    ps,
      bool          IN    exact)
/*
 * Decodes Base32 atom as specified in rfc4648.
 *
 * Parameters:     pd             NULL or decoded data buffer (5 bytes size)
 *                 ps             Base32 atom, 8 printable chars
 *                 exact          false if case-insensitive decoding
 *
 * Return:         zero if failed or number of processed bytes
 *
 */
{

   unumber t, i, len;
   char  a[b32_size_encoded_atom];
   char* pa;
   usize ctab = exact ? (usize)'Z' : (usize)'z';

   for (i=0; i<b32_size_encoded_atom; i++) {
      t = *(byte*)ps++;
      if (t == '=')
         break;
      if ((t > ctab) || (std_decode32_tab[t] < 0))
         return 0;
      a[i] = std_decode32_tab[t];
   }

   for (len=i+1; len<b32_size_encoded_atom; len++)
      if (*ps++ != '=')
         return 0;

   if (i == b32_size_encoded_atom)
      len=b32_size_decoded_atom;     /* 5 */
   else 
   if (i == 7)
      len=b32_size_decoded_atom-1;   /* 4 */
   else 
   if (i == 5)
      len=b32_size_decoded_atom-2;   /* 3 */
   else 
   if (i == 4)
      len=b32_size_decoded_atom-3;   /* 2 */
   else 
   if (i == 2)
      len=b32_size_decoded_atom-4;   /* 1 */
   else
      return 0;

   if (pd == NULL)
      return len;

   pa = a;
   for (i=len; ; ) {
      /* pd[0]; t is empty */
      t  = (*pa++ << 3);
      t |= (*pa   >> 2);
      *pd++ = (byte)t;
      if (--i == 0)
         break;
      /* pd[1]; ct=2,bits */
      t  = (*pa++ << 6);
      t |= (*pa++ << 1);
      t |= (*pa   >> 4);
      *pd++ = (byte)t;
      if (--i == 0)
         break;
      /* pd[2]; ct=4,bits */
      t  = (*pa++ << 4);
      t |= (*pa   >> 1);
      *pd++ = (byte)t;
      if (--i == 0)
         break;
      /* pd[3]; ct=1,bits */
      t  = (*pa++ << 7);
      t |= (*pa++ << 2);
      t |= (*pa   >> 3);
      *pd++ = (byte)t;
      if (--i == 0)
         break;
      /* pd[3]; ct=3,bits */
      t  = (*pa++ << 5);
      t |= *pa;
      *pd++ = (byte)t;
      break;
   }

   return len;

}

/*****************************************************************************/
bool
   b32_init(
      b32_ctx_t*    IN OUT   ctx,
      bool          IN       encode,
      unumber       IN       cline,
      b32_flags_t   IN       flags)
/*
 * Initializes Base32 coder context
 *
 */
{

   assert(ctx != NULL);
   MemSet(ctx, 0x00, sizeof(*ctx));

   if (cline != 0) {
      if (cline < b32_size_encoded_atom+2)
         ERR_SET(err_bad_param);
      cline -= 2;
      cline /= b32_size_encoded_atom;
      ctx->cline = cline;
   }

   ctx->flags = flags;
   ctx->state = (encode) ? b32_state_encode : b32_state_decode;
   return true;

}

/*****************************************************************************/
bool
   b32_encode(
      b32_ctx_t*    IN OUT   ctx,
      char*         OUT      pdst,
      usize*        IN OUT   cdst,
      const byte*   IN       psrc,
      usize*        IN OUT   csrc,
      bool          IN       final)
/*
 * Base32 encoding (RFC4648)
 *
 */
{

   usize cbuf, i;
   const byte* ps = psrc;
   char* pd = pdst;

   assert(ctx != NULL);
   assert(csrc != NULL);

   /*
    * Sanity check
    *
    */
   if (ctx->state != b32_state_encode)
      ERR_SET(err_unexpected_call);
   if ((void*)pdst == (void*)psrc)
      ERR_SET(err_invalid_pointer);
   if ((*csrc > 0) && (psrc == NULL))
      ERR_SET(err_invalid_pointer);

   cbuf = 0;
   if (cdst != NULL) {
      cbuf  = *cdst;
      *cdst = 0;
   }
   if (pdst == NULL) {
      if (cdst == NULL)
         cdst = &cbuf;
      cbuf = (usize)-1;
   }

   if (cbuf < b32_size_encoded_atom)
      ERR_SET(err_buffer_too_small);

   /*
    * Encoding loop
    *
    */
   for (; (*csrc>0) && (cbuf>=b32_size_encoded_atom); ) {

      /*
       * End-of-data
       *
       */
      if (!final && (*csrc < b32_size_decoded_atom))
         break;

      /*
       * Line feed
       *
       */
      if ((ctx->cline != 0) && (ctx->atoms >= ctx->cline)) {
         if (pdst != NULL) {
            pd[0] = '\r';
            pd[1] = '\n';
         }
         pd   += 2;
         cbuf -= 2;
         ctx->atoms = 0;
         continue;
      }

      i = (pdst != NULL) ?
         base32_encode_atom(pd,   ps, *csrc) :
         base32_encode_atom(NULL, ps, *csrc);
      ps    += i;
      *csrc -= i;
      pd    += b32_size_encoded_atom;
      cbuf  -= b32_size_encoded_atom;
      ctx->atoms++;

   }

   if (final && (*csrc == 0))
      ctx->state = b32_state_eof;

   *cdst = pd - pdst;
   *csrc = ps - psrc;
   return true;

}

/*****************************************************************************/
bool
   b32_decode(
      bool*         OUT      ok, 
      b32_ctx_t*    IN OUT   ctx,
      byte*         OUT      pdst,
      usize*        IN OUT   cdst,
      const char*   IN       psrc,
      usize*        IN OUT   csrc,
      bool          IN       final)
/*
 * Base32 decoding (RFC4648)
 *
 */
{

   usize cbuf, len;
   byte* pd = pdst;
   const char* ps = psrc;
   unumber catom;
   char atom[b32_size_encoded_atom];
   const char* pt;
   bool exact;
   
   assert(ctx  != NULL);
   assert(csrc != NULL);
   assert(ok   != NULL);

   *ok = false;

   /*
    * Sanity check
    *
    */
   if (ctx->state == b32_state_error)
      return true;
   if (ctx->state != b32_state_decode)
      ERR_SET(err_unexpected_call);
   if ((*csrc > 0) && (psrc == NULL))
      ERR_SET(err_invalid_pointer);

   cbuf = 0;
   if (cdst != NULL) {
      cbuf  = *cdst;
      *cdst = 0;
   }
   if (pdst == NULL) {
      if (cdst == NULL)
         cdst = &cbuf;
      cbuf = (usize)-1;
   }
   if (cbuf < b32_size_encoded_atom)
      ERR_SET(err_buffer_too_small);

   exact = ((ctx->flags & b32_ignore_case)==0);
   /*
    * Decoding loop
    *
    */
   for (; (ctx->state==b32_state_decode) && (cbuf>0); ) { 

      /*
       * Load Base32 atom if it possible
       *
       */
      for (catom=0, pt=ps; 
          (*csrc>0) && (catom<b32_size_encoded_atom); 
          (*csrc)--) {
         const char t = *(pt++);
         switch (t) {
         case '\r':
         case '\n':
         case '\0':
         case ' ':
         case '\t':
            if (catom == 0)   /* skip ahead ignores */
               ps++;
            continue;
         default:
            break;
         }
         atom[catom++] = t;
      }
      if (catom < b32_size_encoded_atom)
         break;

      /*
       * Try decode
       *
       */
      len = b32_decode_atom(NULL, atom, exact);
      if ((cbuf < b32_size_decoded_atom) && (cbuf < len))
         break;

      if (pdst != NULL) 
         len = b32_decode_atom(pd, atom, exact);
      if (len == 0) {
         ctx->state = b32_state_error;
         if (ps != psrc)
            goto exit;
         ERR_SET(err_bad_param);
      }

      ps = pt;
      if (len < b32_size_decoded_atom)
         ctx->state = b32_state_eof;
      pd   += len;
      cbuf -= len;

   }

   if (final && (*csrc == 0))
      ctx->state = b32_state_eof;

   *ok = true;

exit:
   *cdst = pd - pdst;
   *csrc = ps - psrc;
   return true;

}

