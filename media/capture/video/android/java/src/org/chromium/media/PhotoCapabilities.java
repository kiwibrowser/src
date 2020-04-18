// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.media;

import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;

/**
 * Set of PhotoCapabilities read from the different VideoCapture Devices.
 **/
@JNINamespace("media")
class PhotoCapabilities {
    public final int maxIso;
    public final int minIso;
    public final int currentIso;
    public final int stepIso;
    public final int maxHeight;
    public final int minHeight;
    public final int currentHeight;
    public final int stepHeight;
    public final int maxWidth;
    public final int minWidth;
    public final int currentWidth;
    public final int stepWidth;
    public final double maxZoom;
    public final double minZoom;
    public final double currentZoom;
    public final double stepZoom;
    public final int focusMode;
    public final int[] focusModes;
    public final int exposureMode;
    public final int[] exposureModes;
    public final double maxExposureCompensation;
    public final double minExposureCompensation;
    public final double currentExposureCompensation;
    public final double stepExposureCompensation;
    public final int whiteBalanceMode;
    public final int[] whiteBalanceModes;
    public final int[] fillLightModes;
    public final boolean supportsTorch;
    public final boolean torch;
    public final boolean redEyeReduction;
    public final int maxColorTemperature;
    public final int minColorTemperature;
    public final int currentColorTemperature;
    public final int stepColorTemperature;

    PhotoCapabilities(int maxIso, int minIso, int currentIso, int stepIso, int maxHeight,
            int minHeight, int currentHeight, int stepHeight, int maxWidth, int minWidth,
            int currentWidth, int stepWidth, double maxZoom, double minZoom, double currentZoom,
            double stepZoom, int focusMode, int[] focusModes, int exposureMode, int[] exposureModes,
            double maxExposureCompensation, double minExposureCompensation,
            double currentExposureCompensation, double stepExposureCompensation,
            int whiteBalanceMode, int[] whiteBalanceModes, int[] fillLightModes,
            boolean supportsTorch, boolean torch, boolean redEyeReduction, int maxColorTemperature,
            int minColorTemperature, int currentColorTemperature, int stepColorTemperature) {
        this.maxIso = maxIso;
        this.minIso = minIso;
        this.currentIso = currentIso;
        this.stepIso = stepIso;
        this.maxHeight = maxHeight;
        this.minHeight = minHeight;
        this.currentHeight = currentHeight;
        this.stepHeight = stepHeight;
        this.maxWidth = maxWidth;
        this.minWidth = minWidth;
        this.currentWidth = currentWidth;
        this.stepWidth = stepWidth;
        this.maxZoom = maxZoom;
        this.minZoom = minZoom;
        this.currentZoom = currentZoom;
        this.stepZoom = stepZoom;
        this.focusMode = focusMode;
        this.focusModes = focusModes;
        this.exposureMode = exposureMode;
        this.exposureModes = exposureModes;
        this.maxExposureCompensation = maxExposureCompensation;
        this.minExposureCompensation = minExposureCompensation;
        this.currentExposureCompensation = currentExposureCompensation;
        this.stepExposureCompensation = stepExposureCompensation;
        this.whiteBalanceMode = whiteBalanceMode;
        this.whiteBalanceModes = whiteBalanceModes;
        this.fillLightModes = fillLightModes;
        this.supportsTorch = supportsTorch;
        this.torch = torch;
        this.redEyeReduction = redEyeReduction;
        this.maxColorTemperature = maxColorTemperature;
        this.minColorTemperature = minColorTemperature;
        this.currentColorTemperature = currentColorTemperature;
        this.stepColorTemperature = stepColorTemperature;
    }

    @CalledByNative
    public int getMinIso() {
        return minIso;
    }

    @CalledByNative
    public int getMaxIso() {
        return maxIso;
    }

    @CalledByNative
    public int getCurrentIso() {
        return currentIso;
    }

    @CalledByNative
    public int getStepIso() {
        return stepIso;
    }

    @CalledByNative
    public int getMinHeight() {
        return minHeight;
    }

    @CalledByNative
    public int getMaxHeight() {
        return maxHeight;
    }

    @CalledByNative
    public int getCurrentHeight() {
        return currentHeight;
    }

    @CalledByNative
    public int getStepHeight() {
        return stepHeight;
    }

    @CalledByNative
    public int getMinWidth() {
        return minWidth;
    }

    @CalledByNative
    public int getMaxWidth() {
        return maxWidth;
    }

    @CalledByNative
    public int getCurrentWidth() {
        return currentWidth;
    }

    @CalledByNative
    public int getStepWidth() {
        return stepWidth;
    }

    @CalledByNative
    public double getMinZoom() {
        return minZoom;
    }

    @CalledByNative
    public double getMaxZoom() {
        return maxZoom;
    }

    @CalledByNative
    public double getCurrentZoom() {
        return currentZoom;
    }

    @CalledByNative
    public double getStepZoom() {
        return stepZoom;
    }

    @CalledByNative
    public int getFocusMode() {
        return focusMode;
    }

    @CalledByNative
    public int[] getFocusModes() {
        return focusModes != null ? focusModes.clone() : new int[0];
    }

    @CalledByNative
    public int getExposureMode() {
        return exposureMode;
    }

    @CalledByNative
    public int[] getExposureModes() {
        return exposureModes != null ? exposureModes.clone() : new int[0];
    }

    @CalledByNative
    public double getMinExposureCompensation() {
        return minExposureCompensation;
    }

    @CalledByNative
    public double getMaxExposureCompensation() {
        return maxExposureCompensation;
    }

    @CalledByNative
    public double getCurrentExposureCompensation() {
        return currentExposureCompensation;
    }

    @CalledByNative
    public double getStepExposureCompensation() {
        return stepExposureCompensation;
    }

    @CalledByNative
    public int getWhiteBalanceMode() {
        return whiteBalanceMode;
    }

    @CalledByNative
    public int[] getWhiteBalanceModes() {
        return whiteBalanceModes != null ? whiteBalanceModes.clone() : new int[0];
    }

    @CalledByNative
    public int[] getFillLightModes() {
        return fillLightModes != null ? fillLightModes.clone() : new int[0];
    }

    @CalledByNative
    public boolean getSupportsTorch() {
        return supportsTorch;
    }

    @CalledByNative
    public boolean getTorch() {
        return torch;
    }

    @CalledByNative
    public boolean getRedEyeReduction() {
        return redEyeReduction;
    }

    @CalledByNative
    public int getMinColorTemperature() {
        return minColorTemperature;
    }

    @CalledByNative
    public int getMaxColorTemperature() {
        return maxColorTemperature;
    }

    @CalledByNative
    public int getCurrentColorTemperature() {
        return currentColorTemperature;
    }

    @CalledByNative
    public int getStepColorTemperature() {
        return stepColorTemperature;
    }

    public static class Builder {
        public int maxIso;
        public int minIso;
        public int currentIso;
        public int stepIso;
        public int maxHeight;
        public int minHeight;
        public int currentHeight;
        public int stepHeight;
        public int maxWidth;
        public int minWidth;
        public int currentWidth;
        public int stepWidth;
        public double maxZoom;
        public double minZoom;
        public double currentZoom;
        public double stepZoom;
        public int focusMode;
        public int[] focusModes;
        public int exposureMode;
        public int[] exposureModes;
        public double maxExposureCompensation;
        public double minExposureCompensation;
        public double currentExposureCompensation;
        public double stepExposureCompensation;
        public int whiteBalanceMode;
        public int[] whiteBalanceModes;
        public int[] fillLightModes;
        public boolean supportsTorch;
        public boolean torch;
        public boolean redEyeReduction;
        public int maxColorTemperature;
        public int minColorTemperature;
        public int currentColorTemperature;
        public int stepColorTemperature;

        public Builder() {}

        public Builder setMaxIso(int maxIso) {
            this.maxIso = maxIso;
            return this;
        }

        public Builder setMinIso(int minIso) {
            this.minIso = minIso;
            return this;
        }

        public Builder setCurrentIso(int currentIso) {
            this.currentIso = currentIso;
            return this;
        }

        public Builder setStepIso(int stepIso) {
            this.stepIso = stepIso;
            return this;
        }

        public Builder setMaxHeight(int maxHeight) {
            this.maxHeight = maxHeight;
            return this;
        }

        public Builder setMinHeight(int minHeight) {
            this.minHeight = minHeight;
            return this;
        }

        public Builder setCurrentHeight(int currentHeight) {
            this.currentHeight = currentHeight;
            return this;
        }

        public Builder setStepHeight(int stepHeight) {
            this.stepHeight = stepHeight;
            return this;
        }

        public Builder setMaxWidth(int maxWidth) {
            this.maxWidth = maxWidth;
            return this;
        }

        public Builder setMinWidth(int minWidth) {
            this.minWidth = minWidth;
            return this;
        }

        public Builder setCurrentWidth(int currentWidth) {
            this.currentWidth = currentWidth;
            return this;
        }

        public Builder setStepWidth(int stepWidth) {
            this.stepWidth = stepWidth;
            return this;
        }

        public Builder setMaxZoom(double maxZoom) {
            this.maxZoom = maxZoom;
            return this;
        }

        public Builder setMinZoom(double minZoom) {
            this.minZoom = minZoom;
            return this;
        }

        public Builder setCurrentZoom(double currentZoom) {
            this.currentZoom = currentZoom;
            return this;
        }

        public Builder setStepZoom(double stepZoom) {
            this.stepZoom = stepZoom;
            return this;
        }

        public Builder setFocusMode(int focusMode) {
            this.focusMode = focusMode;
            return this;
        }

        public Builder setFocusModes(int[] focusModes) {
            this.focusModes = focusModes.clone();
            return this;
        }

        public Builder setExposureMode(int exposureMode) {
            this.exposureMode = exposureMode;
            return this;
        }

        public Builder setExposureModes(int[] exposureModes) {
            this.exposureModes = exposureModes.clone();
            return this;
        }

        public Builder setMaxExposureCompensation(double maxExposureCompensation) {
            this.maxExposureCompensation = maxExposureCompensation;
            return this;
        }

        public Builder setMinExposureCompensation(double minExposureCompensation) {
            this.minExposureCompensation = minExposureCompensation;
            return this;
        }

        public Builder setCurrentExposureCompensation(double currentExposureCompensation) {
            this.currentExposureCompensation = currentExposureCompensation;
            return this;
        }

        public Builder setStepExposureCompensation(double stepExposureCompensation) {
            this.stepExposureCompensation = stepExposureCompensation;
            return this;
        }

        public Builder setWhiteBalanceMode(int whiteBalanceMode) {
            this.whiteBalanceMode = whiteBalanceMode;
            return this;
        }

        public Builder setWhiteBalanceModes(int[] whiteBalanceModes) {
            this.whiteBalanceModes = whiteBalanceModes.clone();
            return this;
        }

        public Builder setFillLightModes(int[] fillLightModes) {
            this.fillLightModes = fillLightModes.clone();
            return this;
        }

        public Builder setSupportsTorch(boolean supportsTorch) {
            this.supportsTorch = supportsTorch;
            return this;
        }

        public Builder setTorch(boolean torch) {
            this.torch = torch;
            return this;
        }

        public Builder setRedEyeReduction(boolean redEyeReduction) {
            this.redEyeReduction = redEyeReduction;
            return this;
        }

        public Builder setMaxColorTemperature(int maxColorTemperature) {
            this.maxColorTemperature = maxColorTemperature;
            return this;
        }

        public Builder setMinColorTemperature(int minColorTemperature) {
            this.minColorTemperature = minColorTemperature;
            return this;
        }

        public Builder setCurrentColorTemperature(int currentColorTemperature) {
            this.currentColorTemperature = currentColorTemperature;
            return this;
        }

        public Builder setStepColorTemperature(int stepColorTemperature) {
            this.stepColorTemperature = stepColorTemperature;
            return this;
        }

        public PhotoCapabilities build() {
            return new PhotoCapabilities(maxIso, minIso, currentIso, stepIso, maxHeight, minHeight,
                    currentHeight, stepHeight, maxWidth, minWidth, currentWidth, stepWidth, maxZoom,
                    minZoom, currentZoom, stepZoom, focusMode, focusModes, exposureMode,
                    exposureModes, maxExposureCompensation, minExposureCompensation,
                    currentExposureCompensation, stepExposureCompensation, whiteBalanceMode,
                    whiteBalanceModes, fillLightModes, supportsTorch, torch, redEyeReduction,
                    maxColorTemperature, minColorTemperature, currentColorTemperature,
                    stepColorTemperature);
        }
    }
}
