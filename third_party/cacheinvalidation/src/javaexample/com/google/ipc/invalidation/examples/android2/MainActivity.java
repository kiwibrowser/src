/*
 * Copyright 2011 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.google.ipc.invalidation.examples.android2;

import com.google.ipc.invalidation.external.client.contrib.MultiplexingGcmListener;
import com.google.ipc.invalidation.external.client.types.ObjectId;

import android.app.Activity;
import android.content.Context;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup.LayoutParams;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.TextView;

import java.nio.charset.Charset;

/**
 * A simple  sample application that displays information about object registrations and
 * versions.
 *
 * <p>To submit invalidations, you can run the ExampleServlet:
 *
 * <p><code>
 * blaze run java/com/google/ipc/invalidation/examples:ExampleServlet -- \
       --publisher_spec="" \
       --port=8888 \
       --channel_uri="talkgadget.google.com"
 * </code>
 *
 * <p>and open http://localhost:8888/publisher.
 *
 * <p>Just publish invalidations with ids similar to 'Obj1', 'Obj2', ... 'Obj3'
 *
 */
public final class MainActivity extends Activity {

  /** Tag used in logging. */
  private static final String TAG = "TEA2:MainActivity";

  /**
   * Keep track of current registration and object status. This should probably be implemented
   * using intents rather than static state but I don't want to distract from the invalidation
   * client essentials in this example.
   */
  public static final class State {
    private static volatile String info;
    private static volatile MainActivity currentActivity;

    public static void setInfo(String info) {
      State.info = info;
      refreshInfo();
    }
  }

  /** Text view showing current {@link ExampleListenerState}. */
  private TextView info;

  /** Text view used to display error messages. */
  private TextView error;

  /** Called when the activity is first created. */
  @Override
  public void onCreate(Bundle savedInstanceState) {
    Log.i(TAG, "[onCreate] Creating main activity");
    super.onCreate(savedInstanceState);

    MultiplexingGcmListener.initializeGcm(this);

    // Setup UI.
    LinearLayout layout = new LinearLayout(this);
    layout.setOrientation(LinearLayout.VERTICAL);
    layout.addView(createClientLifetimeButtons(), LayoutParams.WRAP_CONTENT);
    layout.addView(createRegistrationControls(), LayoutParams.WRAP_CONTENT);
    error = new TextView(this);
    layout.addView(error, LayoutParams.WRAP_CONTENT);
    info = new TextView(this);
    layout.addView(info, LayoutParams.WRAP_CONTENT);
    setContentView(layout);

    // Remember the current activity since the TICL service in this example communicates via
    // static state.
    State.currentActivity = this;
    Log.i(TAG, "[onCreate] Calling refresh data from main activity");
    refreshInfo();
  }

  /** Creates start and stop buttons. */
  private View createClientLifetimeButtons() {
    LinearLayout layout = new LinearLayout(this);
    layout.setOrientation(LinearLayout.HORIZONTAL);

    // Start button.
    Button startButton = new Button(this);
    startButton.setText("Start");
    startButton.setOnClickListener(new OnClickListener() {
      @Override
      public void onClick(View v) {
        v.getContext().startService(ExampleListener.createStartIntent(v.getContext()));
      }
    });
    layout.addView(startButton, LayoutParams.WRAP_CONTENT);

    // Stop button.
    Button stopButton = new Button(this);
    stopButton.setText("Stop");
    stopButton.setOnClickListener(new OnClickListener() {
      @Override
      public void onClick(View v) {
        v.getContext().startService(ExampleListener.createStopIntent(v.getContext()));
      }
    });
    layout.addView(stopButton, LayoutParams.WRAP_CONTENT);

    return layout;
  }

  private View createRegistrationControls() {
    LinearLayout layout = new LinearLayout(this);
    layout.setOrientation(LinearLayout.VERTICAL);

    // Create the source text field
    LinearLayout fieldsLayout = new LinearLayout(this);
    fieldsLayout.setOrientation(LinearLayout.HORIZONTAL);
    TextView sourceText = new TextView(this);
    sourceText.setText("source:");
    fieldsLayout.addView(sourceText, LayoutParams.WRAP_CONTENT);
    final EditText sourceField = new EditText(this);
    sourceField.setText("DEMO");
    fieldsLayout.addView(sourceField, LayoutParams.WRAP_CONTENT);

    // Create the name text field
    TextView nameText = new TextView(this);
    nameText.setText("name:");
    fieldsLayout.addView(nameText, LayoutParams.WRAP_CONTENT);
    final EditText nameField = new EditText(this);
    nameField.setText("Obj1");
    fieldsLayout.addView(nameField, LayoutParams.WRAP_CONTENT);
    layout.addView(fieldsLayout, LayoutParams.WRAP_CONTENT);

    // Shared click listener class for registration (register and unregister).
    abstract class RegistrationClickListener implements OnClickListener {
      @Override
      public void onClick(View v) {
        String sourceText = sourceField.getText().toString();
        // Build object id from relevant fields. Can't use reflection because this is Android.
        final int source;
        if ("DEMO".equals(sourceText)) {
          source = 4;
        } else if ("TEST".equals(sourceText)) {
          source = 2;
        } else {
          error.setText("Unrecognized source: " + sourceText);
          return;
        }

        String name = nameField.getText().toString();
        ObjectId objectId =
            ObjectId.newInstance(source, name.getBytes(Charset.forName("UTF-8")));
        performRegistration(v.getContext(), objectId);
      }

      abstract void performRegistration(Context context, ObjectId objectId);
    }

    // Create the reg/unreg buttons
    LinearLayout buttonsLayout = new LinearLayout(this);
    buttonsLayout.setOrientation(LinearLayout.HORIZONTAL);
    Button regButton = new Button(this);
    regButton.setText("Register");
    regButton.setOnClickListener(new RegistrationClickListener() {
      @Override
      void performRegistration(Context context, ObjectId objectId) {
        context.startService(ExampleListener.createRegisterIntent(context, objectId));
      }
    });
    Button unregButton = new Button(this);
    unregButton.setText("Unregister");
    unregButton.setOnClickListener(new RegistrationClickListener() {
      @Override
      void performRegistration(Context context, ObjectId objectId) {
        context.startService(ExampleListener.createUnregisterIntent(context, objectId));
      }
    });
    buttonsLayout.addView(regButton, LayoutParams.WRAP_CONTENT);
    buttonsLayout.addView(unregButton, LayoutParams.WRAP_CONTENT);
    layout.addView(buttonsLayout, LayoutParams.WRAP_CONTENT);
    return layout;
  }

  /** Updates UI with current registration status and object versions. */
  private static void refreshInfo() {
    final MainActivity activity = State.currentActivity;
    if (null != activity) {
      activity.info.post(new Runnable() {
        @Override
        public void run() {
          activity.info.setText(State.info);
        }
      });
    }
  }
}
