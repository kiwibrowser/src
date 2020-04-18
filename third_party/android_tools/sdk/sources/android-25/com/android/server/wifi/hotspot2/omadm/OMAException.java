package com.android.server.wifi.hotspot2.omadm;

import java.io.IOException;

public class OMAException extends IOException {
    public OMAException(String message) {
        super(message);
    }
}
