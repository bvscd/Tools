package com.lge.ria;

public class Ria 
{
   
   /* 
    * Execution status
    *
    */
   public static final int riaExecUnknown = 0;
   public static final int riaExecOk      = 1;
   public static final int riaExecFailed  = 2;

   /* 
    * Execution context
    *
    */
   public int    Status;
   public String Result;

   /* 
    * Creates RIA engine
    *
    * Parameters:     tempdir          path to temporary directory
    *
    * Return:         engine handle or NULL if error
    *
    */
   public native int riaInit(String tempdir);

   /*
    * Destroy RIA engine
    *
    * Parameters:     engine           engine handle
    *
    * Return:         true             if successful
    *                 false            if failed
    *  
    */ 
   public native boolean riaShutdown(int engine);     
      
   /*
    * Returns last cached error message
    *
    * Parameters:     engine         engine handle
    *
    * Return:         message text
    *
    */
   public native String riaErrorMsg(int engine);

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
   public native boolean riaLoad(String path, int engine);
     
   /*
    * Executes script
    *
    * Parameters:     name           script name
    *                 params         parameters array (nul-terminated strings)
    *                 engine         engine handle
    *
    * Return:         true           if successful
    *                 false          if failed
    *
    */
   public native boolean riaExecute(String name, String[] params, int engine); 
     
   /* 
    * Load native part
    *
    */
   static {
      System.loadLibrary("ria");
   }

}

 
