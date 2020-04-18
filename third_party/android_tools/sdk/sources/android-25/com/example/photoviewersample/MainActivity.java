package com.example.photoviewersample;

import android.app.Activity;
import android.os.Bundle;
import android.view.View;
import android.view.View.OnClickListener;

import com.android.ex.photo.Intents;
import com.android.ex.photo.Intents.PhotoViewIntentBuilder;

public class MainActivity extends Activity implements OnClickListener {

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        View b = findViewById(R.id.button);
        b.setOnClickListener(this);
    }

    @Override
    public void onClick(View v) {
        final PhotoViewIntentBuilder builder =
                Intents.newPhotoViewActivityIntentBuilder(this);
        builder
            .setPhotosUri("content://com.example.photoviewersample.SampleProvider/photos");

        startActivity(builder.build());
    }
}
