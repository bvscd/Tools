
#include "ria_papi.h"
#include "ria_exec.h"
#include "ria_func.h"
#include "ria_core.h"
#ifdef ANDROID
#include <curl/curl.h>
#endif
#ifdef WISE12
#include "..\..\StdPxe.h"
#include "ApiLink.h"
#include "PalFS.h"
#endif



/******************************************************************************
 *   File API
 */

/*****************************************************************************/
bool 
   ria_papi_fopen(
      void*         OUT   file,
      const char*   IN    name,
      const char*   IN    mode)
/*
 * fopen() from platform
 *
 */
{

   assert(file != NULL);
   assert(name != NULL);
   assert(mode != NULL);

#if defined(WIN32_APP)
   if (fopen_s((FILE**)file, name, mode) == 0) 
      return true;
   ERR_SET(err_internal); 
#elif defined(ANDROID)
   *((FILE**)file) = fopen(name, mode);
   if (*((FILE**)file) != NULL)
      return true;
   ERR_SET(err_internal); 
#elif defined(WISE12)
   {
	  int hFile = 0;
	  int __mode = -1;
	  int rez_open = 0;
	  A_CHAR    w_name[255] = L"";

      if(mode[0] == 'r')
		  __mode = 0;
	  if(mode[0] == 'w')   
		  __mode = 1;
	  if(mode[0] == 'a')	
		  __mode = PALFS_O_CREAT | PALFS_O_APPEND | PALFS_O_WRONLY | 0x8000;

	  UniLib_UTF8ToUCS2(name, w_name);
	  hFile = PalFS_open(w_name, __mode );
	  if(hFile > 0)
	  {
	     *((int *)file) = hFile;
		 return true;
	  }
	  *((int *)file) = 0;
	  return false;

		
	  //UniLib_UTF8ToUCS2(name, w_name);	  

	  //rez_open = PalFS_open(w_name, __mode);
	  //if(rez_open > 0)
	  //{
	  //   *((int *)file) = rez_open;
		 //return true;
	  //}
   //   ERR_SET(err_internal); 
  	//  return false;
   }
#else
#error Not implemented
#endif

}

/*****************************************************************************/
bool
   ria_papi_fread(
      usize*   OUT   cdata,
      byte*    IN    pbuf,
      usize    IN    cbuf,
      void*    IN    file)
/*
 * fread() from platform
 *
 */
{

   assert(cdata != NULL);
   assert(pbuf  != NULL);
   assert(file  != NULL);

#if defined(WIN32_APP) || defined(ANDROID)
   *cdata = fread(pbuf, 1, cbuf, file);
   if (*cdata == 0) 
      if (ferror((FILE*)file)) 
         ERR_SET(err_internal);
   return true;   
#elif defined(WISE12)
   {
	   int rez_count = 0;
	   rez_count = PalFS_read(file, pbuf, cbuf);
	   if(rez_count > 0)
	   {
	       *cdata = rez_count;
		   return true;
	   }
       ERR_SET(err_internal);
       return false;
   }
#else
#error Not implemented
#endif

}

/*****************************************************************************/
bool
   ria_papi_fwrite(
      const byte*   IN    data,
      usize         IN    cnt,
      void*         OUT   file)
/*
 * fwrite() from platform
 *
 */
{

   assert(data != NULL);
   assert(file != NULL);

#if defined(WIN32_APP) || defined(ANDROID)
   if (fwrite(data, 1, cnt, file) == cnt) 
      return true;
   ERR_SET(err_internal);
#elif defined(WISE12)
   {
	  int written = PalFS_write(file, data, cnt);
	  if (written == cnt) 
		  return true;
	  ERR_SET(err_internal);
   }

   
#else
#error Not implemented
#endif

}

/*****************************************************************************/
bool
   ria_papi_fseek(
      ioffset             IN   pos,
      ria_file_origin_t   IN   origin,
      void*               IN   file)
/*
 * fseek() from platform
 *
 */
{

#if defined(WIN32_APP) || defined(ANDROID) || defined(WISE12)
   int _origin;
#endif

   assert(file != NULL);

#if defined(WIN32_APP) || defined(ANDROID)
   switch (origin) {
   case ria_file_seek_begin:
      _origin = SEEK_SET;
      break;
   case ria_file_seek_end:
      _origin = SEEK_END;
      break;
   case ria_file_seek_current:
      _origin = SEEK_CUR;
      break;
   default:
      ERR_SET(err_bad_param);   
   }
   if (fseek(file, (long)pos, _origin) == 0) 
      return true;
   ERR_SET(err_internal);
#elif defined(WISE12)
   
   switch (origin) {
   case ria_file_seek_begin:
      _origin = SEEK_SET;
      break;
   case ria_file_seek_end:
      _origin = SEEK_END;
      break;
   case ria_file_seek_current:
      _origin = SEEK_CUR;
      break;
   default:
      ERR_SET(err_bad_param);      
   }   
   if (PalFS_lseek(file, pos, _origin) >= 0) 
      return true;
   ERR_SET(err_internal);
#else
#error Not implemented
#endif

}

/*****************************************************************************/
bool
   ria_papi_ftell(
      usize*   OUT   pos,
      void*    IN    file)
/*
 * ftell() from platform
 *
 */
{

   assert(file != NULL);
   assert(pos  != NULL);

#if defined(WIN32_APP) || defined(ANDROID)
   *pos = ftell(file);
   if (*pos != -1) 
      return true;
   ERR_SET(err_internal);
#elif defined(WISE12)
   *pos = PalFS_Tell(file);
   if (*pos != -1) 
      return true;
   ERR_SET(err_internal);
#else
#error Not implemented
#endif

}

/*****************************************************************************/
bool
   ria_papi_fclose(
      void*   OUT   file)
/*
 * fclose() from platform
 *
 */
{

   assert(file != NULL);

#if defined(WIN32_APP) || defined(ANDROID)
   if (fclose(file) == 0) 
      return true;
   ERR_SET(err_internal);
#elif defined(WISE12)   

   if (PalFS_close(file) == 0) 
      return true;   
#else
#error Not implemented
#endif

}


/******************************************************************************
 *   HTTP API
 */

/*****************************************************************************/
#ifdef WIN32_APP
#ifdef USE_RIA_ASYNC_CALLS
void CALLBACK 
   ria_papi_status_callback(
      HINTERNET   IN   hInternet,
      DWORD_PTR   IN   dwContext,
      DWORD       IN   dwInternetStatus,
      LPVOID      IN   lpvStatusInformation,
      DWORD       IN   dwStatusInformationLength)
/*
 * Status callback for HTTP
 *
 */
{

   ria_http_t* ctx;

   /*
    * Context is HTTP context
    *
    */
   ctx = (ria_http_t*)dwContext;
   if (ctx == NULL) {
      ERR_SET_NO_RET(err_internal);
      return;    
   }   
   if (ctx->async.owner == NULL) {
      ERR_SET_NO_RET(err_internal);
      return;    
   }   

   RIA_TRACE_MSG("Internet status =");
   RIA_TRACE_INT(dwInternetStatus);
   RIA_TRACE_MSG("\n");

   /*
    * Check for status
    *
    */
   switch (ctx->async.owner->status) {
   case ria_exec_transit:
   case ria_exec_pending:
      break;
   default:   
      ERR_SET_NO_RET(err_internal);
      return;
   }   
   switch (ctx->async.state) {
   case ria_http_async_connect:
      RIA_TRACE_MSG("HTTP async state: CONNECT\n");
      if (dwInternetStatus != INTERNET_STATUS_HANDLE_CREATED)
         return;
      if (lpvStatusInformation != NULL)   
         ctx->connect = *((void**)lpvStatusInformation);
      break;
   case ria_http_async_receive:
      RIA_TRACE_MSG("HTTP async state: RECEIVE\n");
      if (dwInternetStatus == INTERNET_STATUS_HANDLE_CREATED) {
         if (lpvStatusInformation != NULL)   
            ctx->request = *((void**)lpvStatusInformation);
         return;   
      }            
      if (dwInternetStatus != INTERNET_STATUS_REQUEST_COMPLETE)
         return;
      break;
   case ria_http_async_rcvmore:
      RIA_TRACE_MSG("HTTP async state: RCVMORE\n");
      if (dwInternetStatus != INTERNET_STATUS_REQUEST_COMPLETE)
         return;
      break;
   default:
      ERR_SET_NO_RET(err_internal);
      return;   
   } 
   
   /*
    * Cleanup pending state
    *
    */
   ria_async_set_status(&ctx->async, ria_exec_proceed);
   if (ctx->async.callback != NULL)
      (*ctx->async.callback)(ctx->async.owner);
   
   hInternet,
   lpvStatusInformation,
   dwStatusInformationLength;
      
}
#endif
#endif

/*****************************************************************************/
#ifdef ANDROID
size_t 
   ria_papi_receive_callback(
      void*    IN   ptr, 
      size_t   IN   size, 
      size_t   IN   nmemb, 
      void*    IN   stream)
/*
 * libCurl receive callback for HTTP
 *
 */
{
 
   bool pending;
   ria_http_t* ctx;

   /*
    * Sanity checks
    *
    */
   if (stream == NULL) {
      ERR_SET_NO_RET(err_internal);
      return 0;
   }   
   if (ptr == NULL) {
      ERR_SET_NO_RET(err_internal);
      return 0;
   }   
      
   /*
    * Call conventional reciever
    *
    */
   ctx = (ria_http_t*)stream; 
   ctx->pdata = ptr;
   ctx->cdata = size*nmemb;
   if (!ria_http_receive(
           &pending, 
           ctx->tofile, 
           ctx->todump, 
           ctx->normalize, 
           ctx->reentry, 
           ctx))
      return 0;
   ctx->reentry = true;   
   if (pending) {
      ERR_SET_NO_RET(err_internal);
      return 0;
   }      
   
   /*
    * Success 
    *
    */
   return size*nmemb-ctx->cdata; 

} 
#endif

/*****************************************************************************/
#ifdef ANDROID
size_t 
   ria_papi_header_callback(
      void*    IN   ptr, 
      size_t   IN   size, 
      size_t   IN   nmemb, 
      void*    IN   stream)
/*
 * libCurl header receive callback for HTTP
 *
 */
{
 
   uint  c;
   byte* p;
   double tmp;
   ria_http_t* ctx;
   
   /*
    * Sanity checks
    *
    */
   if (stream == NULL) {
      ERR_SET_NO_RET(err_internal);
      return 0;
   }      
   if (ptr == NULL) {
      ERR_SET_NO_RET(err_internal);
      return 0;
   }      

   /*
    * Append header 
    *
    */
   ctx = (ria_http_t*)stream; 
   c = buf_get_length(&ctx->hdrs); 
   if (!buf_expand(c+size*nmemb+2, &ctx->hdrs))
      return 0;
   p = buf_get_ptr_bytes(&ctx->hdrs) + c;
   MemCpy(p, ptr, size*nmemb);
   p += size*nmemb;
   p[0] = '\r';
   p[1] = '\n';
   if (!buf_set_length(c+size*nmemb+2, &ctx->hdrs))
      return 0;
   ctx->hdrs_recv = true;
   
   /*
    * Check for zero-size body
    *
    */
   if (curl_easy_getinfo(
          ctx->proto, 
          CURLINFO_CONTENT_LENGTH_DOWNLOAD, 
          &tmp) != CURLE_OK)
      ERR_SET(err_internal);
   if (tmp == 0) {
      if (ria_papi_receive_callback(ptr, 0, 0, stream) != 0)
         ERR_SET(err_internal);
   }    
   
   /*
    * Success 
    *
    */
   return size*nmemb;    

} 
#endif

/*****************************************************************************/
#ifdef WISE12
#include "ria_uapi.h"

extern ria_http_t* ctxRia;

TS_WAPAPI_HTTPRESPONSE pParam_tmp;
bool 
   ria_papi_receive_callback(
      ria_exec_status_t*   OUT      status,
      char*                IN OUT   presult,
      uint*                IN OUT   cresult,
      ria_handle_t         IN       engine,
      TS_WAPAPI_HTTPRESPONSE *    IN   pParam)
/*
 * WISE receive callback for HTTP
 *
 */
{
	int hLen = 0;
    int count = 0;
	pParam_tmp.objectId = pParam->objectId;
	pParam_tmp.requestId = pParam->requestId;
	hLen = strlen(pParam->headers);
		   
	pParam_tmp.headers = SysHeap_Alloc(hLen);
	for(count = 0; count < hLen; count++)
	   ((char*)pParam_tmp.headers)[count] = ((char*)pParam->headers)[count];

	pParam_tmp.data = SysHeap_Alloc(pParam->dataLen);
	   for(count = 0; count < pParam->dataLen; count++)
	      ((char*)pParam_tmp.data)[count] = ((char*)pParam->data)[count];

    pParam_tmp.dataLen = pParam->dataLen;
	pParam_tmp.moreData = pParam->moreData;
	pParam_tmp.errorNo = pParam->errorNo;
	pParam_tmp.socketId = pParam->socketId;
	pParam_tmp.protocol = pParam->protocol;
   /*
    * pParam
    *
    */

   /*
    * Cleanup pending state
    *
    */
   ria_async_set_status(&ctxRia->async, ria_exec_proceed);

   /*
    * Continue receive
    *
    */
   if (!ria_uapi_continue(status, presult, cresult, engine))
   {
	  //*status = ria_exec_proceed;
      return false;	    
   }

   /*
    * Success 
    *
    */
   return true; 

} 
#endif

/*****************************************************************************/
bool
   ria_papi_http_init(
      const char*   IN    agent,
      ria_http_t*   OUT   ctx)
/*
 * http_init() from platform
 *
 */
{

#ifdef WIN32_APP
   DWORD dw;
#endif

   assert(agent != NULL);
   assert(ctx   != NULL);

#ifdef WIN32_APP

#ifdef USE_RIA_ASYNC_CALLS
   dw = INTERNET_FLAG_ASYNC;
#else
   dw = 0;
#endif
   ctx->proto = InternetOpenA(
                   agent, 
                   INTERNET_OPEN_TYPE_PRECONFIG, 
                   NULL, 
                   NULL, 
                   dw);
   if (ctx->proto == NULL) 
      ERR_SET(err_internal);
#ifdef USE_RIA_ASYNC_CALLS
   if (InternetSetStatusCallbackA(ctx->proto, ria_papi_status_callback)
          ==  INTERNET_INVALID_STATUS_CALLBACK)
      ERR_SET(err_internal);
#endif      
   return true;              

#elif defined(ANDROID) /* WIN32_APP */

   ctx->proto = (CURL*)curl_easy_init();
   if (ctx->proto == NULL) 
      ERR_SET(err_internal);
   if (curl_easy_setopt(ctx->proto, CURLOPT_USERAGENT, agent) != CURLE_OK)
      ERR_SET(err_internal);
   return true;      
#elif defined(WISE12) /* ANDROID */      
//   ERR_SET(err_internal);
   {
	   bool rez = false;
	   rez = Hopper_Init_WapHttp_Obj();
	   return rez;
   }
   return true;
#else /* WISE12 */
#error Not implemented
#endif

}

/*****************************************************************************/
bool
   ria_papi_http_shutdown(
      ria_http_t*   OUT   ctx)
/*
 * http_shutdown() from platform
 *
 */
{

   void* p;

   assert(ctx != NULL);
   
   if (ctx->proto == NULL)
      ERR_SET(err_unexpected_call);
   p = ctx->proto;
   ctx->proto = NULL;      

#ifdef WIN32_APP

   if (!InternetCloseHandle(p)) 
      ERR_SET(err_internal);   
   return true;
   
#elif defined(ANDROID) /* WIN32_APP */

   curl_easy_cleanup(p);
   return true;   

#elif defined(WISE12) /* ANDROID */
   ERR_SET(err_internal); 
#else /* WISE12 */
#error Not implemented
#endif

}

/*****************************************************************************/
bool
   ria_papi_http_connect(
      bool*         OUT   pending, 
      const char*   IN    site,
      ria_http_t*   OUT   ctx)
/*
 * http_connect() from platform
 *
 */
{

#ifdef WIN32_APP
   DWORD dw;
#endif
#ifdef ANDROID
   static const char _http[] = "http://";
   uint  c;
   byte* p;
#endif

   assert(ctx     != NULL);
   assert(pending != NULL);
   
   *pending = false;

#ifdef WIN32_APP

#ifdef RIA_ASYNC_CONNECT
   dw = (DWORD)ctx;
#else
   dw = INTERNET_NO_CALLBACK;
#endif
   ctx->connect = InternetConnectA(
                     ctx->proto, 
                     site, 
                     INTERNET_DEFAULT_HTTP_PORT, 
                     "", 
                     "", 
                     INTERNET_SERVICE_HTTP, 
                     0, 
                     dw);
   if (ctx->connect == NULL) {
#ifdef RIA_ASYNC_CONNECT
      if (GetLastError() == ERROR_IO_PENDING) {
         *pending = true;
         return true;   
      }   
#endif
      ERR_SET(err_internal);
   }      
   return true;
   
#elif defined (ANDROID) /* WIN32_APP */

   /*
    * No separate connect primitive in easyCURL, 
    * create URL buffer 
    *
    */
   if (ctx->connect == NULL) {
      if (!heap_alloc((void**)&ctx->connect, sizeof(buf_t), ctx->mem))
         return false;
      if (!buf_create(1, 0, 0, (buf_t*)ctx->connect, ctx->mem)) {
         heap_free(ctx->connect, ctx->mem);
         return false;
      }
   }
   
   /*
    * Cache site name 
    *
    */
   c = StrLen(site);
   if (!buf_expand(c+sizeof(_http)-1, (buf_t*)ctx->connect))
      return false;
   p = buf_get_ptr_bytes((buf_t*)ctx->connect);
   MemCpy(p, _http, sizeof(_http)-1);
   p += sizeof(_http) - 1;
   MemCpy(p, site, c);
   return buf_set_length(c+sizeof(_http)-1, (buf_t*)ctx->connect);

#elif defined (WISE12) /* ANDROID */
   Hopper_Save_Site(site, ctx);
   return true;
//   ERR_SET(err_internal);
#else /*Android*//* WISE12 */
#error Not implemented
#endif

}

/*****************************************************************************/
bool
   ria_papi_http_disconnect(
      ria_http_t*   OUT   ctx)
/*
 * http_disconnect() from platform
 *
 */
{

   void* p;
#ifdef ANDROID
   bool  ret;
#endif   

   assert(ctx != NULL);
   
   if (ctx->connect == NULL)
      ERR_SET(err_unexpected_call);
   p = ctx->connect;
   ctx->connect = NULL;   

#ifdef WIN32_APP

   if (!InternetCloseHandle(p)) 
      ERR_SET(err_internal);
   return true;
   
#elif defined(ANDROID) /* WIN32_APP */

   ret = buf_destroy((buf_t*)p);
   ret = heap_free(p, ctx->mem) && ret;
   return ret;

#elif defined(WISE12) /* ANDROID */  
   ERR_SET(err_internal);
#else /* WISE12 */
#error Not implemented
#endif

}

/*****************************************************************************/
bool
   ria_papi_http_send(
      bool*         OUT   pending, 
      const char*   IN    url,
      const char*   IN    pval,
      usize         IN    cval,
      ria_http_t*   OUT   ctx)
/*
 * http_send() from platform
 *
 */
{

#ifdef WIN32_APP
   static const char _get[]  = "GET";
   static const char _post[] = "POST";
   DWORD dw;
#endif
#ifdef ANDROID 
   uint  c, curl;
   byte* p;
   byte* q;
   bool  ret = false;
   struct curl_slist *headerlist = NULL;
#endif

   assert(url     != NULL);
   assert(ctx     != NULL);
   assert(pending != NULL);
   
   *pending = false;

   /*
    * Close previous response
    *
    */
   if (ctx->request != NULL) {
      ria_papi_http_close_request(ctx);
      ctx->request = NULL;
   }      

#ifdef WIN32_APP

#ifdef RIA_ASYNC_RECEIVE
   dw = (DWORD)ctx;
#else
   dw = INTERNET_NO_CALLBACK;
#endif
   ctx->request = HttpOpenRequestA(
                     ctx->connect, 
                     (pval==NULL)?_get:_post,
                     url,
                     NULL, 
                     "", 
                     NULL, 
	                  INTERNET_FLAG_RELOAD|
                     INTERNET_FLAG_KEEP_CONNECTION|
                     INTERNET_FLAG_NO_CACHE_WRITE|
                     INTERNET_FLAG_NO_COOKIES|
                     INTERNET_FLAG_NO_AUTO_REDIRECT,
                     dw);
   if (ctx->request == NULL)
      ERR_SET(err_internal);
   if (HttpSendRequestA(
          ctx->request, 
          (ctx->hdrs_recv)?NULL:(LPCSTR)buf_get_ptr_bytes(&ctx->hdrs),
          (DWORD)((ctx->hdrs_recv)?0:buf_get_length(&ctx->hdrs)),
          (pval==NULL)?NULL:(LPVOID)pval, 
          (DWORD)((pval==NULL)?0:cval)))
      return true;
   else {
#ifdef RIA_ASYNC_RECEIVE
      if (GetLastError() == ERROR_IO_PENDING) {
         *pending = true;
         return true;
      }   
#endif         
      ERR_SET(err_internal);
   }   
   
#elif defined(ANDROID) /* WIN32_APP */

   if (ctx->connect == NULL) {
      ERR_SET_NO_RET(err_internal);
      goto cleanup;
   }   

   /*
    * Reassemble URL 
    *
    */
   curl = StrLen(url); 
   c = buf_get_length((buf_t*)ctx->connect);
   if (!buf_expand(c+curl+1, (buf_t*)ctx->connect))
      goto cleanup;
   p = buf_get_ptr_bytes((buf_t*)ctx->connect);
   MemCpy(p+c, url, curl);
   p[c+curl] = 0x00;
   if (curl_easy_setopt(ctx->proto, CURLOPT_URL, p) != CURLE_OK) {
      ERR_SET_NO_RET(err_internal);
      goto cleanup;
   }   

   /*
    * Add required HTTP headers 
    *
    */
   if (!ctx->hdrs_recv) {
      c = buf_get_length(&ctx->hdrs);
      p = buf_get_ptr_bytes(&ctx->hdrs);
      for (; c>0; p=q+2, c-=2) {
         for (q=p; c>0; c--, q++) 
            if (q[0] == 0x0D)
               break;
         if (c == 1) {
            ERR_SET_NO_RET(err_internal);
            goto cleanup;
         }   
         if (q[1] != 0x0A) {
            ERR_SET_NO_RET(err_internal);
            goto cleanup;
         }   
         q[0] = 0x00;      
         headerlist = curl_slist_append(headerlist, p);
         if (headerlist == NULL) {
            ERR_SET_NO_RET(err_internal);
            goto cleanup;
         }   
         q[0] = 0x0D;
      }
   }   
   if (curl_easy_setopt(
          ctx->proto, 
          CURLOPT_HTTPHEADER, 
          headerlist) != CURLE_OK) {
      ERR_SET_NO_RET(err_internal);
      goto cleanup;
   }      
   if (!buf_destroy(&ctx->hdrs)) {
      ERR_SET_NO_RET(err_internal);
      goto cleanup;
   }
      
   /*
    * Setup HTTP options
    *
    */
   if (curl_easy_setopt(
          ctx->proto, 
          CURLOPT_WRITEFUNCTION, 
          ria_papi_receive_callback) != CURLE_OK) {
      ERR_SET_NO_RET(err_internal);
      goto cleanup;
   }   
   if (curl_easy_setopt(ctx->proto, CURLOPT_WRITEDATA, ctx) != CURLE_OK) {
      ERR_SET_NO_RET(err_internal);
      goto cleanup;
   }   
   if (curl_easy_setopt(
          ctx->proto, 
          CURLOPT_HEADERFUNCTION, 
          ria_papi_header_callback) != CURLE_OK) {
      ERR_SET_NO_RET(err_internal);
      goto cleanup;
   }   
   if (curl_easy_setopt(ctx->proto, CURLOPT_WRITEHEADER, ctx) != CURLE_OK) {
      ERR_SET_NO_RET(err_internal);
      goto cleanup;
   }   
   if (curl_easy_setopt(ctx->proto, CURLOPT_FOLLOWLOCATION, 0) != CURLE_OK) {
      ERR_SET_NO_RET(err_internal);
      goto cleanup;
   }   
   if (pval == NULL) {
      if (curl_easy_setopt(ctx->proto, CURLOPT_HTTPGET, 0) != CURLE_OK) {
         ERR_SET_NO_RET(err_internal);
         goto cleanup;
      }   
   }   
   else {
      if (curl_easy_setopt(ctx->proto, CURLOPT_POST, 0) != CURLE_OK) {
         ERR_SET_NO_RET(err_internal);
         goto cleanup;
      }   
      if (curl_easy_setopt(ctx->proto, CURLOPT_POSTFIELDS, pval) != CURLE_OK) {
         ERR_SET_NO_RET(err_internal);
         goto cleanup;
      }   
      if (curl_easy_setopt(
             ctx->proto, 
             CURLOPT_POSTFIELDSIZE, 
             cval) != CURLE_OK) {
         ERR_SET_NO_RET(err_internal);
         goto cleanup;
      }         
   }         

   /*
    * Do the HTTP job
    *
    */
   curl = curl_easy_perform(ctx->proto);
   if (curl != CURLE_OK) {
      if (GET_ERR_CONTEXT->err == err_none)
         ERR_SET_NO_RET(err_internal);
      goto cleanup;
   }   
   ret = true;   
      
cleanup:      
   if (headerlist != NULL) 
      curl_slist_free_all(headerlist);
   return ret;

#elif defined(WISE12) /* ANDROID */
   {
      int rez = false;
      rez = Hopper_Http_request(url, pval, cval, ctx);
	  if(rez == false)
	  {
		  ERR_SET(err_internal);
		  return false;
	  }
	  *pending = true;
	  return true;
   }
   
#else /* WISE12 */
#error Not implemented
#endif

}

/*****************************************************************************/
bool
   ria_papi_http_get_status(
      unumber*      OUT   code,
      ria_http_t*   OUT   ctx)
/*
 * http_get_status() from platform
 *
 */
{

#ifdef WIN32_APP
   char  status[5];
   DWORD dwl;
#endif
#ifdef ANDROID
   long tmp;
#endif

   assert(code != NULL);
   assert(ctx  != NULL);

#ifdef WIN32_APP

   dwl = sizeof(status)-1;
   if (!HttpQueryInfoA(
           ctx->request, 
           HTTP_QUERY_STATUS_CODE,
           status,
           &dwl,
           NULL)) 
      ERR_SET(err_internal);
   sscanf_s(status, "%d", code);
   dwl = 0;
   HttpQueryInfoA(
      ctx->request, 
      HTTP_QUERY_RAW_HEADERS_CRLF,
      NULL,
      &dwl,
      NULL);
   if (!buf_expand(dwl, &ctx->hdrs))
      return false;
   if (!HttpQueryInfoA(
           ctx->request, 
           HTTP_QUERY_RAW_HEADERS_CRLF,
           buf_get_ptr_bytes(&ctx->hdrs),
           &dwl,
           NULL))
      ERR_SET(err_internal);
   if (!buf_set_length(dwl, &ctx->hdrs))
      return false;
   ctx->hdrs_recv = true;   
   return true;
   
#elif defined(ANDROID) /* WIN32_APP */

   if (curl_easy_getinfo(ctx->proto, CURLINFO_RESPONSE_CODE, &tmp) != CURLE_OK)
      ERR_SET(err_internal);
   *code = tmp;
   return true;   

#elif defined(WISE12) /* ANDROID */

   *code = pParam_tmp.errorNo;
   if (!buf_load(
	       pParam_tmp.headers, 
	       0, 
		   StrLen(pParam_tmp.headers)+1, 
		   &ctx->hdrs))
      return false;
   ctx->hdrs_recv = true;   
   return true;

#else /* WISE12 */
#error Not implemented
#endif

}

/*****************************************************************************/
bool
   ria_papi_http_receive(
      bool*         OUT   pending,
      usize*        OUT   cdata,
      byte*         IN    pbuf,
      usize         IN    cbuf,
      ria_http_t*   OUT   ctx)
/*
 * http_receive() from platform
 *
 */
{

#ifdef WIN32_APP
   DWORD dw;
#endif

   assert(cdata   != NULL);
   assert(pbuf    != NULL);
   assert(ctx     != NULL);
   assert(pending != NULL);
   
   *pending = false;

#ifdef WIN32_APP

#ifdef RIA_ASYNC_RECEIVE
   if (!InternetQueryDataAvailable(
           ctx->request,
           &dw, 
           0,
           0)) {
      if (GetLastError() != ERROR_IO_PENDING) 
         ERR_SET(err_internal);
      *pending = true;
      *cdata   = 0;
      RIA_TRACE_MSG("PENDING RECEIVE\n"); 
      return true;
   }         
   if (dw < cbuf)
      cbuf = dw;
#endif      
	if (!InternetReadFile(
           ctx->request,
           pbuf, 
           (DWORD)cbuf, 
           &dw)) 
      ERR_SET(err_internal);
   *cdata = dw;
   return true;
   
#elif defined(ANDROID) /* WIN32_APP */

   *cdata = (ctx->cdata > cbuf) ? cbuf : ctx->cdata;
   MemCpy(pbuf, ctx->pdata, *cdata);
   ctx->pdata += *cdata;
   ctx->cdata -= *cdata;
   return true;

#elif defined(WISE12) /* ANDROID */

   *cdata = (pParam_tmp.dataLen > cbuf) ? cbuf : pParam_tmp.dataLen;
   MemCpy(pbuf, pParam_tmp.data, *cdata);
   (char*)pParam_tmp.data += *cdata;
   pParam_tmp.dataLen -= *cdata;
   if (pParam_tmp.dataLen == 0)
	  if (pParam_tmp.moreData) 
		 *pending = true;
   return true;

#else /* WISE12 */
#error Not implemented
#endif

}

/*****************************************************************************/
bool
   ria_papi_http_close_request(
      ria_http_t*   OUT   ctx)
/*
 * http_close_request() from platform
 *
 */
{

   void* p;

   assert(ctx != NULL);
   
   if (ctx->request == NULL)
      ERR_SET(err_unexpected_call);
   p = ctx->request;
   ctx->request = NULL;   

#ifdef WIN32_APP
   if (!InternetCloseHandle(ctx->request))
      ERR_SET(err_internal);
   return true;
#elif defined(ANDROID) || defined (WISE12)
   /*
    * Do nothing
    *
    */
#else
#error Not implemented
#endif

}

#if 0
/*****************************************************************************/
bool
   ria_papi_http_get_header(
      buf_t*              IN OUT   dst,
      const char*         IN       phdr,
      uint                IN       chdr,
      const ria_http_t*   OUT      ctx)
/*
 * http_get_header() from platform
 *
 */
{

#ifdef WIN32_APP
   typedef struct hdr_s {
      const char* name;
      DWORD       code;
   } hdr_t;

   static const hdr_t _headers[] = 
   {
      { "date",       HTTP_QUERY_DATE       },
      { "location",   HTTP_QUERY_LOCATION   },
      { "set-cookie", HTTP_QUERY_SET_COOKIE }
   };
   
   DWORD dwl, dwx, dwi = 0;
   uint c, l;
   byte *p;

#endif

   assert(dst  != NULL);
   assert(phdr != NULL);
   assert(ctx  != NULL);

#ifdef WIN32_APP
   
   for (dwl=0; dwl<sizeof(_headers)/sizeof(_headers[0]); dwl++) {
      if (StrLen(_headers[dwl].name) != chdr)
         continue;
      if (StrNICmp(_headers[dwl].name, phdr, chdr))
         continue;
      dwi = _headers[dwl].code;
      break;
   }
   if (dwl == sizeof(_headers)/sizeof(_headers[0])) 
      dwi = HTTP_QUERY_CUSTOM;
   
   dwl = 0;
   dwx = 0;
   l = StrLen(_multiheader);
   for (c=0;; ) {
      if (dwi == HTTP_QUERY_CUSTOM) {
         if (!buf_expand(c+chdr+1, dst))
            return false;
         if (!buf_set_length(c+chdr+1, dst))
            return false;
         p = buf_get_ptr_bytes(dst) + c;   
         MemCpy(p, phdr, chdr);
         p[chdr] = 0x00;
      }
      else 
         p = NULL;
      if (!HttpQueryInfoA(
            ctx->request, 
            dwi,
            (dwi==HTTP_QUERY_CUSTOM)?p:NULL,
            &dwl,
            &dwx)) {
         if (GetLastError() == ERROR_HTTP_HEADER_NOT_FOUND) {
            dwl = 0;
            break;
         }   
      }
      if (c > 0) 
         c += l;
      if (!buf_expand(c+dwl, dst))
         return false;
      if (!buf_set_length(c+dwl, dst))
         return false;
      p = buf_get_ptr_bytes(dst) + c;   
      if (c > 0)   
         MemCpy(p-l, _multiheader, l);
      if (!HttpQueryInfoA(
              ctx->request, 
              dwi,
              p,
              &dwl,
              &dwx)) 
         ERR_SET(err_internal);
      c += dwl;
   }
   if (!buf_set_length(c+dwl+1, dst))
      return false;
   return true;
#else
#error Not implemented
#endif

}
#endif 


