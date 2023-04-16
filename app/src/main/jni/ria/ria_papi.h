#ifndef _RIA_PAPI
#define _RIA_PAPI


#include "ria_core.h"
#include "ria_http.h"


/******************************************************************************
 *   File API
 */
 
 typedef enum ria_file_origin_e {
    ria_file_seek_begin,
    ria_file_seek_end,
    ria_file_seek_current
 } ria_file_origin_t;

/*@@ria_papi_fopen
 *
 * fopen() from platform
 *
 * Parameters:     file           file pointer buffer
 *                 name           file name
 *                 mode           open mode
 *
 * Return:         true           if successful,
 *                 false          if failed
 *
 */
bool
   ria_papi_fopen(
      void*         OUT   file,
      const char*   IN    name,
      const char*   IN    mode);

/*@@ria_papi_fread
 *
 * fread() from platform
 *
 * Parameters:     cdata          size of read date
 *                 pbuf           buffer to read to
 *                 cbuf           buffer size, bytes
 *                 file           file pointer 
 *
 * Return:         true           if successful,
 *                 false          if failed
 *
 */
bool
   ria_papi_fread(
      usize*   OUT   cdata,
      byte*    IN    pbuf,
      usize    IN    cbuf,
      void*    IN    file);
      
/*@@ria_papi_fwrite
 *
 * fwrite() from platform
 *
 * Parameters:     data           data to write
 *                 cnt            data size, bytes
 *                 file           file pointer 
 *
 * Return:         true           if successful,
 *                 false          if failed
 *
 */
bool
   ria_papi_fwrite(
      const byte*   IN   data,
      usize         IN   cnt,
      void*         IN   file);

/*@@ria_papi_fseek
 *
 * fseek() from platform
 *
 * Parameters:     pos            new position
 *                 origin         seek origin
 *                 file           file pointer 
 *
 * Return:         true           if successful,
 *                 false          if failed
 *
 */
bool
   ria_papi_fseek(
      ioffset             IN   pos,
      ria_file_origin_t   IN   origin,
      void*               IN   file);
      
/*@@ria_papi_ftell
 *
 * ftell() from platform
 *
 * Parameters:     pos            position storage
 *                 file           file pointer 
 *
 * Return:         true           if successful,
 *                 false          if failed
 *
 */
bool
   ria_papi_ftell(
      usize*   OUT   pos,
      void*    IN    file);

/*@@ria_papi_fclose
 *
 * fclose() from platform
 *
 * Parameters:     file           file pointer 
 *
 * Return:         true           if successful,
 *                 false          if failed
 *
 */
bool
   ria_papi_fclose(
      void*   IN   file);


/******************************************************************************
 *   HTTP API
 */

/*@@ria_papi_http_init
 *
 * http_init() from platform
 *
 * Parameters:     agent          user agent string
 *                 ctx            HTTP context 
 *
 * Return:         true           if successful,
 *                 false          if failed
 *
 */
bool
   ria_papi_http_init(
      const char*   IN   agent,
      ria_http_t*   OUT   ctx);

/*@@ria_papi_http_shutdown
 *
 * http_shutdown() from platform
 *
 * Parameters:     ctx            HTTP context 
 *
 * Return:         true           if successful,
 *                 false          if failed
 *
 */
bool
   ria_papi_http_shutdown(
      ria_http_t*   OUT   ctx);

/*@@ria_papi_http_connect
 *
 * http_connect() from platform
 *
 * Parameters:     pending        pending connect flag
 *                 site           site address
 *                 ctx            HTTP context
 *
 * Return:         true           if successful,
 *                 false          if failed
 *
 */
bool
   ria_papi_http_connect(
      bool*         OUT   pending,
      const char*   IN    site,
      ria_http_t*   OUT   ctx);

/*@@ria_papi_http_disconnect
 *
 * http_disconnect() from platform
 *
 * Parameters:     ctx            HTTP context
 *
 * Return:         true           if successful,
 *                 false          if failed
 *
 */
bool
   ria_papi_http_disconnect(
      ria_http_t*   OUT   ctx);

/*@@ria_papi_http_send
 *
 * http_send() from platform
 *
 * Parameters:     pending        pending send flag
 *                 url            URL to send to
 *                 pval           value to POST (NULL for GET)
 *                 cval           value size (0 for GET)
 *                 ctx            HTTP context
 *
 * Return:         true           if successful,
 *                 false          if failed
 *
 */
bool
   ria_papi_http_send(
      bool*         OUT   pending,
      const char*   IN    url,
      const char*   IN    pval,
      usize         IN    cval,
      ria_http_t*   OUT   ctx);

/*@@ria_papi_http_get_status
 *
 * http_get_status() from platform
 *
 * Parameters:     code           status code
 *                 ctx            HTTP context
 *
 * Return:         true           if successful,
 *                 false          if failed
 *
 */
bool
   ria_papi_http_get_status(
      unumber*      OUT   code,
      ria_http_t*   OUT   ctx);

/*@@ria_papi_http_receive
 *
 * http_receive() from platform
 *
 * Parameters:     pending        pending receive flag
 *                 cdata          received data size
 *                 pbuf           buffer 
 *                 cbuf           buffer size
 *                 ctx            HTTP context
 *
 * Return:         true           if successful,
 *                 false          if failed
 *
 */
bool
   ria_papi_http_receive(
      bool*         OUT   pending,
      usize*        OUT   cdata,
      byte*         IN    pbuf,
      usize         IN    cbuf,
      ria_http_t*   OUT   ctx);

/*@@ria_papi_http_close_request
 *
 * http_close_request() from platform
 *
 * Parameters:     ctx            HTTP context
 *
 * Return:         true           if successful,
 *                 false          if failed
 *
 */
bool
   ria_papi_http_close_request(
      ria_http_t*   OUT   ctx);

#endif
