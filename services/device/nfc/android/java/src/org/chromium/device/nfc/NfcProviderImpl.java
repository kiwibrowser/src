// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.device.nfc;

import org.chromium.device.mojom.Nfc;
import org.chromium.device.mojom.NfcProvider;
import org.chromium.mojo.bindings.InterfaceRequest;
import org.chromium.mojo.system.MojoException;
import org.chromium.services.service_manager.InterfaceFactory;

/**
 * Android implementation of the NfcProvider Mojo interface.
 */
public class NfcProviderImpl implements NfcProvider {
    private static final String TAG = "NfcProviderImpl";
    private NfcDelegate mDelegate;

    public NfcProviderImpl(NfcDelegate delegate) {
        mDelegate = delegate;
    }

    @Override
    public void close() {}

    @Override
    public void onConnectionError(MojoException e) {}

    @Override
    public void getNfcForHost(int hostId, InterfaceRequest<Nfc> request) {
        Nfc.MANAGER.bind(new NfcImpl(hostId, mDelegate), request);
    }

    /**
     * A factory for implementations of the NfcProvider interface.
     */
    public static class Factory implements InterfaceFactory<NfcProvider> {
        private NfcDelegate mDelegate;

        public Factory(NfcDelegate delegate) {
            mDelegate = delegate;
        }

        @Override
        public NfcProvider createImpl() {
            return new NfcProviderImpl(mDelegate);
        }
    }
}
