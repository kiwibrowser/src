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
package com.google.ipc.invalidation.ticl.android2;

import com.google.ipc.invalidation.external.client.SystemResources;
import com.google.ipc.invalidation.external.client.SystemResources.Storage;
import com.google.ipc.invalidation.external.client.types.Callback;
import com.google.ipc.invalidation.external.client.types.SimplePair;
import com.google.ipc.invalidation.external.client.types.Status;
import com.google.ipc.invalidation.ticl.InvalidationClientCore;
import com.google.ipc.invalidation.util.Preconditions;

import android.content.Context;

import java.io.DataInputStream;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;

/**
 * Implementation of {@link Storage} for the Android Ticl. This implementation supports only
 * the {@link InvalidationClientCore#CLIENT_TOKEN_KEY}. As required by Android storage
 * implementations, it executes all callbacks synchronously.
 *
 */
public class AndroidStorage implements Storage {
  /** Name of the file in which state will be stored. */
  private static final String STATE_FILENAME = "ticl_storage.bin";

  /** Maximum size of the file which we are willing to read. */
  private static final int MAX_STATE_FILE_SIZE_BYTES = 4096;

  private final Context context;

  public AndroidStorage(Context context) {
    this.context = Preconditions.checkNotNull(context);
  }

  @Override
  public void setSystemResources(SystemResources resources) {
  }

  @Override
  public void writeKey(String key, byte[] value, Callback<Status> done) {
    // We only support the CLIENT_TOKEN_KEY.
    if (!key.equals(InvalidationClientCore.CLIENT_TOKEN_KEY)) {
      done.accept(Status.newInstance(Status.Code.PERMANENT_FAILURE, "Key unsupported: " + key));
      return;
    }
    // Write the data.
    FileOutputStream outstream = null;
    Status status = null;
    try {
      outstream = context.openFileOutput(STATE_FILENAME, Context.MODE_PRIVATE);
      outstream.write(value);
      status = Status.newInstance(Status.Code.SUCCESS, "");
    } catch (FileNotFoundException exception) {
      status = Status.newInstance(Status.Code.PERMANENT_FAILURE, "File not found: " + exception);
    } catch (IOException exception) {
      status = Status.newInstance(Status.Code.PERMANENT_FAILURE, "File not found: " + exception);
    } finally {
      if (outstream != null) {
        try {
          outstream.close();
        } catch (IOException exception) {
          status = Status.newInstance(
              Status.Code.PERMANENT_FAILURE, "Failed to close file: " + exception);
        }
      }
    }
    done.accept(status);
  }

  @Override
  public void readKey(String key, Callback<SimplePair<Status, byte[]>> done) {
    // We only support the CLIENT_TOKEN_KEY.
    if (!key.equals(InvalidationClientCore.CLIENT_TOKEN_KEY)) {
      Status status = Status.newInstance(Status.Code.PERMANENT_FAILURE, "Key unsupported: " + key);
      done.accept(SimplePair.of(status, (byte[]) null));
      return;
    }
    // Read and return the data.
    FileInputStream instream = null;
    SimplePair<Status, byte[]> result = null;
    try {
      instream = context.openFileInput(STATE_FILENAME);
      long fileSizeBytes = instream.getChannel().size();
      if (fileSizeBytes > MAX_STATE_FILE_SIZE_BYTES) {
        Status status =
            Status.newInstance(Status.Code.PERMANENT_FAILURE, "File too big: " + fileSizeBytes);
        result = SimplePair.of(status, (byte[]) null);
      }
      // Cast to int must be safe due to the above size check.
      DataInputStream input = new DataInputStream(instream);
      byte[] fileData = new byte[(int) fileSizeBytes];
      input.readFully(fileData);
      result = SimplePair.of(Status.newInstance(Status.Code.SUCCESS, ""), fileData);
    } catch (FileNotFoundException exception) {
      Status status =
          Status.newInstance(Status.Code.PERMANENT_FAILURE, "File not found: " + exception);
      result = SimplePair.of(status, (byte[]) null);
    } catch (IOException exception) {
      Status status =
          Status.newInstance(Status.Code.TRANSIENT_FAILURE, "IO exception: " + exception);
      result = SimplePair.of(status, (byte[]) null);
    } finally {
      if (instream != null) {
        try {
          instream.close();
        } catch (IOException exception) {
          Status status =
              Status.newInstance(
                  Status.Code.TRANSIENT_FAILURE, "Failed to close file: " + exception);
          result = SimplePair.of(status, (byte[]) null);
        }
      }
    }
    done.accept(result);
  }

  @Override
  public void deleteKey(String key, Callback<Boolean> done) {
    // We only support the CLIENT_TOKEN_KEY.
    if (!key.equals(InvalidationClientCore.CLIENT_TOKEN_KEY)) {
      done.accept(false);
      return;
    }
    if (!context.getFileStreamPath(STATE_FILENAME).exists()) {
      // Deletion "succeeds" if the key didn't exist.
      done.accept(true);
    } else {
      // Otherwise it succeeds based on whether the IO operation succeeded.
      done.accept(context.deleteFile(STATE_FILENAME));
    }
  }

  @Override
  public void readAllKeys(Callback<SimplePair<Status, String>> keyCallback) {
    // If the state file exists, supply the CLIENT_TOKEN_KEY as a present key.
    if (context.getFileStreamPath(STATE_FILENAME).exists()) {
      Status status = Status.newInstance(Status.Code.SUCCESS, "");
      keyCallback.accept(SimplePair.of(status, InvalidationClientCore.CLIENT_TOKEN_KEY));
    }
    keyCallback.accept(null);
  }

  static void deleteStateForTest(Context context) {
    context.deleteFile(STATE_FILENAME);
  }
}
