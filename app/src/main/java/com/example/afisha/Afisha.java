package com.example.afisha;

import android.app.Activity;
import android.widget.TextView;
import android.os.Bundle;
import com.lge.ria.Ria;

public class Afisha extends Activity
{

    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);

        /* Create a TextView and set its content.
         * the text is retrieved by calling a native
         * function.
         */
        /*
	TextView  tv = new TextView(this);
        tv.setText( stringFromJNI() );
        setContentView(tv);
	*/

        Ria ria = new Ria();
        int engine = ria.riaInit("/sdcard");
        ria.riaLoad("/data/app/afisha.scr", engine);
        ria.riaExecute("query_cities", null, engine);

	super.onCreate(savedInstanceState); 
        setContentView(R.layout.main); 
        TextView tv = new TextView(this); 
        ria.riaExecute("get_next_city_url", null, engine);
        tv.append(ria.Result); 
        tv.append("\n"); 
        ria.riaExecute("get_next_city_url", null, engine);
        tv.append(ria.Result); 
        tv.append("\n"); 
        ria.riaExecute("get_next_city_url", null, engine);
        tv.append(ria.Result); 
        tv.append("\n"); 
        tv.append(ria.riaErrorMsg(engine)); 
        setContentView(tv); 
        ria.riaShutdown(engine);

    }

}
