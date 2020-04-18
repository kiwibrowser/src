/*
 * Copyright (C) 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License"),
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.ex.camera2.portability;

class CameraActions {
    // Camera initialization/finalization
    public static final int OPEN_CAMERA = 1;
    public static final int RELEASE =     2;
    public static final int RECONNECT =   3;
    public static final int UNLOCK =      4;
    public static final int LOCK =        5;
    // Preview
    public static final int SET_PREVIEW_TEXTURE_ASYNC =        101;
    public static final int START_PREVIEW_ASYNC =              102;
    public static final int STOP_PREVIEW =                     103;
    public static final int SET_PREVIEW_CALLBACK_WITH_BUFFER = 104;
    public static final int ADD_CALLBACK_BUFFER =              105;
    public static final int SET_PREVIEW_DISPLAY_ASYNC =        106;
    public static final int SET_PREVIEW_CALLBACK =             107;
    public static final int SET_ONE_SHOT_PREVIEW_CALLBACK =    108;
    // Parameters
    public static final int SET_PARAMETERS =     201;
    public static final int GET_PARAMETERS =     202;
    public static final int REFRESH_PARAMETERS = 203;
    public static final int APPLY_SETTINGS =     204;
    // Focus, Zoom
    public static final int AUTO_FOCUS =                   301;
    public static final int CANCEL_AUTO_FOCUS =            302;
    public static final int SET_AUTO_FOCUS_MOVE_CALLBACK = 303;
    public static final int SET_ZOOM_CHANGE_LISTENER =     304;
    public static final int CANCEL_AUTO_FOCUS_FINISH =     305;
    // Face detection
    public static final int SET_FACE_DETECTION_LISTENER = 461;
    public static final int START_FACE_DETECTION =        462;
    public static final int STOP_FACE_DETECTION =         463;
    // Presentation
    public static final int ENABLE_SHUTTER_SOUND =    501;
    public static final int SET_DISPLAY_ORIENTATION = 502;
    public static final int SET_JPEG_ORIENTATION = 503;
    // Capture
    public static final int CAPTURE_PHOTO = 601;

    public static String stringify(int action) {
        switch (action) {
            case OPEN_CAMERA:
                return "OPEN_CAMERA";
            case RELEASE:
                return "RELEASE";
            case RECONNECT:
                return "RECONNECT";
            case UNLOCK:
                return "UNLOCK";
            case LOCK:
                return "LOCK";
            case SET_PREVIEW_TEXTURE_ASYNC:
                return "SET_PREVIEW_TEXTURE_ASYNC";
            case START_PREVIEW_ASYNC:
                return "START_PREVIEW_ASYNC";
            case STOP_PREVIEW:
                return "STOP_PREVIEW";
            case SET_PREVIEW_CALLBACK_WITH_BUFFER:
                return "SET_PREVIEW_CALLBACK_WITH_BUFFER";
            case ADD_CALLBACK_BUFFER:
                return "ADD_CALLBACK_BUFFER";
            case SET_PREVIEW_DISPLAY_ASYNC:
                return "SET_PREVIEW_DISPLAY_ASYNC";
            case SET_PREVIEW_CALLBACK:
                return "SET_PREVIEW_CALLBACK";
            case SET_ONE_SHOT_PREVIEW_CALLBACK:
                return "SET_ONE_SHOT_PREVIEW_CALLBACK";
            case SET_PARAMETERS:
                return "SET_PARAMETERS";
            case GET_PARAMETERS:
                return "GET_PARAMETERS";
            case REFRESH_PARAMETERS:
                return "REFRESH_PARAMETERS";
            case APPLY_SETTINGS:
                return "APPLY_SETTINGS";
            case AUTO_FOCUS:
                return "AUTO_FOCUS";
            case CANCEL_AUTO_FOCUS:
                return "CANCEL_AUTO_FOCUS";
            case SET_AUTO_FOCUS_MOVE_CALLBACK:
                return "SET_AUTO_FOCUS_MOVE_CALLBACK";
            case SET_ZOOM_CHANGE_LISTENER:
                return "SET_ZOOM_CHANGE_LISTENER";
            case CANCEL_AUTO_FOCUS_FINISH:
                return "CANCEL_AUTO_FOCUS_FINISH";
            case SET_FACE_DETECTION_LISTENER:
                return "SET_FACE_DETECTION_LISTENER";
            case START_FACE_DETECTION:
                return "START_FACE_DETECTION";
            case STOP_FACE_DETECTION:
                return "STOP_FACE_DETECTION";
            case ENABLE_SHUTTER_SOUND:
                return "ENABLE_SHUTTER_SOUND";
            case SET_DISPLAY_ORIENTATION:
                return "SET_DISPLAY_ORIENTATION";
            case CAPTURE_PHOTO:
                return "CAPTURE_PHOTO";
            default:
                return "UNKNOWN(" + action + ")";
        }
    }

    private CameraActions() {
        throw new AssertionError();
    }
}
