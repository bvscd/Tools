#include <string.h>
#include <jni.h>
#include <stdio.h> 
#include <stdlib.h> 
#include "curl/curl.h" 
#include "ria_uapi.h"


#define SIZE_RESULT 512


/*****************************************************************************/
jint
   Java_com_lge_ria_Ria_riaInit(
      JNIEnv*  env,
      jobject  this,
      jstring  jtempdir)
/*
 * Creates RIA engine
 *
 * Parameters:     tempdir          path to temporary directory
 *
 * Return:         engine handle or NULL if error
 *
 */
{  
   jint ret; 
   const char *tempdir = (*env)->GetStringUTFChars(env, jtempdir, NULL); 
   ret = (jint)ria_uapi_init(tempdir);
   (*env)->ReleaseStringUTFChars(env, jtempdir, tempdir); 
   return ret;
}

/*****************************************************************************/
jstring
   Java_com_lge_ria_Ria_riaErrorMsg(
      JNIEnv*  env,
      jobject  this,
      jint     jengine)
/*
 * Returns last cached error message
 *
 * Parameters:     engine         engine handle
 *
 * Return:         message text
 *
 */
{   
пррр
   handle engine = (handle)jengine;
   return (*env)->NewStringUTF(env, ria_uapi_error_msg(engine));
}

/*****************************************************************************/
jboolean
   Java_com_lge_ria_Ria_riaShutdown(
      JNIEnv*  env,
      jobject  this,
      jint     jengine)
/*
 * Destroy RIA engine
 *
 * Parameters:     engine           engine handle
 *
 * Return:         true             if successful
 *                 false            if failed
 *  
 */ 
{
   handle engine = (handle)jengine;
   return ria_uapi_shutdown(engine);
}

/*****************************************************************************/
jboolean
   Java_com_lge_ria_Ria_riaLoad(
      JNIEnv*  env,
      jobject  this,
      jstring  jpath,
      jint     jengine)
/*
 * Loads scenario script
 *
 * Parameters:     path           path to script
 *                 engine         engine handle
 *
 * Return:         true           if successful
 *                 false          if failed
 *
 */
{
   jboolean ret; 
   handle engine = (handle)jengine;
   const char *path = (*env)->GetStringUTFChars(env, jpath, NULL); 
   ret = ria_uapi_load(path, engine);
   (*env)->ReleaseStringUTFChars(env, jpath, path); 
   return ret;
}

/*****************************************************************************/
jboolean
   Java_com_lge_ria_Ria_riaExecute(
      JNIEnv*  env,
      jobject  this,
      jstring  jname,
      jarray   jparams,
      jint     jengine)
/*
 * Executes script
 *
 * Parameters:     ctx            execution context
 *                 name           script name
 *                 params         parameters array (nul-terminated strings)
 *                 engine         engine handle
 *
 * Return:         true           if successful
 *                 false          if failed
 *
 */
{

   char* presult;
   uint  cresult;
   uint  cparams, i;
   const char*  name;
   char const** pparams;

   jboolean ret = false;
   ria_exec_status_t status;
   handle engine = (handle)jengine;
   jstring  jparam;
   jclass   jria;
   jfieldID jstatus;
   jfieldID jresult;

   enum {
      cleanup_result = 0x01,
      cleanup_params = 0x02
   } cleanup = 0;

   if (!ria_uapi_alloc((void**)&presult, cresult=SIZE_RESULT))
      goto exit;
   else
      cleanup |= cleanup_result;
   if (jparams != NULL) {
      cparams = (*env)->GetArrayLength(env, jparams);
      if (!ria_uapi_alloc((void**)&pparams, cparams*sizeof(char*)))
         goto exit;
      else
         cleanup |= cleanup_params;
   }
   else {
      pparams = NULL;
      cparams = 0;
   }

   for (i=0; i<cparams; i++) {
      jparam = (*env)->GetObjectArrayElement(env, jparams, i);
      pparams[i] = (*env)->GetStringUTFChars(env, jparam, NULL); 
   }      
   
   name= (*env)->GetStringUTFChars(env, jname, NULL); 
   ret = ria_uapi_execute(
            &status, 
            presult,
            &cresult,
            name,
            pparams,
            cparams,
            engine);

   jria = (*env)->GetObjectClass(env, this);
   jstatus = (*env)->GetFieldID(env, jria, "Status", "I");
   if (jstatus == NULL) {
      ERR_SET_NO_RET(err_internal);
      goto exit;
   }
   jresult = (*env)->GetFieldID(env, jria, "Result", "Ljava/lang/String;");
   if (jresult == NULL) {
      ERR_SET_NO_RET(err_internal);
      goto exit;
   }

   if (ret) {
      (*env)->SetIntField(env, this, jstatus, status);
      (*env)->SetObjectField(
                 env, 
                 this, 
                 jresult, 
                 (*env)->NewStringUTF(env, presult));
   }
   else {
      (*env)->SetIntField(env, this, jstatus, ria_exec_unknown);
      (*env)->SetObjectField(
                 env, 
                 this, 
                 jresult, 
                 (*env)->NewStringUTF(env, ""));
   }   

   (*env)->ReleaseStringUTFChars(env, jname, name); 
   for (i=0; i<cparams; i++) {
      jparam = (*env)->GetObjectArrayElement(env, jparams, i);
      (*env)->ReleaseStringUTFChars(env, jparam, pparams[i]); 
   } 

exit:
   if (cleanup & cleanup_result)     
      ret = ria_uapi_free(presult) && ret;
   if (cleanup & cleanup_params)     
      ret = ria_uapi_free(pparams) && ret;
   return ret;

}



/* This is a trivial JNI example where we use a native method
 * to return a new VM String. See the corresponding Java source
 * file located at:
 *
 *   apps/samples/hello-jni/project/src/com/example/HelloJni/HelloJni.java
 */
jstring
Java_com_example_afisha_Afisha_stringFromJNI( JNIEnv* env,
                                              jobject thiz )
{   

   handle h;
   bool   b;
   ria_exec_status_t status;
   byte   res[1000];
   char*  presult;
   uint   cresult;

#if 0
{

  CURL *curl; 
  CURLcode res; 
  char buffer[10]; 


  curl = curl_easy_init(); 
  if(curl) { 
    curl_easy_setopt(curl, CURLOPT_URL, "yahoo.com"); 
    res = curl_easy_perform(curl); 
        /* always cleanup */ 
    curl_easy_cleanup(curl); 
   if(res == 0) {
   Fprintf0(Stdout, "0 response\n");
   Fflush(Stdout);
   }
   else {
   Fprintf1(Stdout, "code: %i\n", res);
   Fflush(Stdout);
   }
  } else { 
   Fprintf0(Stdout, "no curl\n");
   Fflush(Stdout);
  }

}
#endif


   h = ria_uapi_init("/sdcard");
   b = ria_uapi_load("/data/app/afisha.scr", h);

   if (b) {
      presult = res;
      cresult = sizeof(res);
      ria_uapi_execute(&status, presult, &cresult, "query_cities", NULL, 0, h);
      cresult = sizeof(res);
      ria_uapi_execute(&status, presult, &cresult, "get_next_city_url", NULL, 0, h);
   }

   Fprintf0(Stdout, ria_uapi_error_msg(h));
   Fprintf0(Stdout, "\n");
   Fflush(Stdout);

   ria_uapi_shutdown(h);


   Fprintf0(Stdout, "$$$\n");
   Fflush(Stdout);


   if (h == NULL)
      return (*env)->NewStringUTF(env, "cannot init RIA"); 
   if (!b)
      return (*env)->NewStringUTF(env, "cannot load RIA"); 
   if (status != ria_exec_ok)
      return (*env)->NewStringUTF(env, "cannot exec RIA"); 
   return (*env)->NewStringUTF(env, res); 


} 

