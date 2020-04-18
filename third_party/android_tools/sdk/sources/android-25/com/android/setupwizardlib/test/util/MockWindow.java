package com.android.setupwizardlib.test.util;

import android.content.Context;
import android.content.res.Configuration;
import android.graphics.drawable.Drawable;
import android.net.Uri;
import android.os.Bundle;
import android.view.InputQueue;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;

public class MockWindow extends Window {

    public MockWindow(Context context) {
        super(context);
    }

    @Override
    public void takeSurface(SurfaceHolder.Callback2 callback2) {
        throw new UnsupportedOperationException("Unexpected method call on mock");
    }

    @Override
    public void takeInputQueue(InputQueue.Callback callback) {
        throw new UnsupportedOperationException("Unexpected method call on mock");
    }

    @Override
    public boolean isFloating() {
        throw new UnsupportedOperationException("Unexpected method call on mock");
    }

    @Override
    public void setContentView(int i) {
        throw new UnsupportedOperationException("Unexpected method call on mock");
    }

    @Override
    public void setContentView(View view) {
        throw new UnsupportedOperationException("Unexpected method call on mock");
    }

    @Override
    public void setContentView(View view, ViewGroup.LayoutParams layoutParams) {
        throw new UnsupportedOperationException("Unexpected method call on mock");
    }

    @Override
    public void addContentView(View view, ViewGroup.LayoutParams layoutParams) {
        throw new UnsupportedOperationException("Unexpected method call on mock");
    }

    @Override
    public View getCurrentFocus() {
        throw new UnsupportedOperationException("Unexpected method call on mock");
    }

    @Override
    public LayoutInflater getLayoutInflater() {
        throw new UnsupportedOperationException("Unexpected method call on mock");
    }

    @Override
    public void setTitle(CharSequence charSequence) {
        throw new UnsupportedOperationException("Unexpected method call on mock");
    }

    @Override
    public void setTitleColor(int i) {
        throw new UnsupportedOperationException("Unexpected method call on mock");
    }

    @Override
    public void openPanel(int i, KeyEvent keyEvent) {
        throw new UnsupportedOperationException("Unexpected method call on mock");
    }

    @Override
    public void closePanel(int i) {
        throw new UnsupportedOperationException("Unexpected method call on mock");
    }

    @Override
    public void togglePanel(int i, KeyEvent keyEvent) {
        throw new UnsupportedOperationException("Unexpected method call on mock");
    }

    @Override
    public void invalidatePanelMenu(int i) {
        throw new UnsupportedOperationException("Unexpected method call on mock");
    }

    @Override
    public boolean performPanelShortcut(int i, int i1, KeyEvent keyEvent, int i2) {
        throw new UnsupportedOperationException("Unexpected method call on mock");
    }

    @Override
    public boolean performPanelIdentifierAction(int i, int i1, int i2) {
        throw new UnsupportedOperationException("Unexpected method call on mock");
    }

    @Override
    public void closeAllPanels() {
        throw new UnsupportedOperationException("Unexpected method call on mock");
    }

    @Override
    public boolean performContextMenuIdentifierAction(int i, int i1) {
        throw new UnsupportedOperationException("Unexpected method call on mock");
    }

    @Override
    public void onConfigurationChanged(Configuration configuration) {
        throw new UnsupportedOperationException("Unexpected method call on mock");
    }

    @Override
    public void setBackgroundDrawable(Drawable drawable) {
        throw new UnsupportedOperationException("Unexpected method call on mock");
    }

    @Override
    public void setFeatureDrawableResource(int i, int i1) {
        throw new UnsupportedOperationException("Unexpected method call on mock");
    }

    @Override
    public void setFeatureDrawableUri(int i, Uri uri) {
        throw new UnsupportedOperationException("Unexpected method call on mock");
    }

    @Override
    public void setFeatureDrawable(int i, Drawable drawable) {
        throw new UnsupportedOperationException("Unexpected method call on mock");
    }

    @Override
    public void setFeatureDrawableAlpha(int i, int i1) {
        throw new UnsupportedOperationException("Unexpected method call on mock");
    }

    @Override
    public void setFeatureInt(int i, int i1) {
        throw new UnsupportedOperationException("Unexpected method call on mock");
    }

    @Override
    public void takeKeyEvents(boolean b) {
        throw new UnsupportedOperationException("Unexpected method call on mock");
    }

    @Override
    public boolean superDispatchKeyEvent(KeyEvent keyEvent) {
        throw new UnsupportedOperationException("Unexpected method call on mock");
    }

    @Override
    public boolean superDispatchKeyShortcutEvent(KeyEvent keyEvent) {
        throw new UnsupportedOperationException("Unexpected method call on mock");
    }

    @Override
    public boolean superDispatchTouchEvent(MotionEvent motionEvent) {
        throw new UnsupportedOperationException("Unexpected method call on mock");
    }

    @Override
    public boolean superDispatchTrackballEvent(MotionEvent motionEvent) {
        throw new UnsupportedOperationException("Unexpected method call on mock");
    }

    @Override
    public boolean superDispatchGenericMotionEvent(MotionEvent motionEvent) {
        throw new UnsupportedOperationException("Unexpected method call on mock");
    }

    @Override
    public View getDecorView() {
        throw new UnsupportedOperationException("Unexpected method call on mock");
    }

    @Override
    public View peekDecorView() {
        throw new UnsupportedOperationException("Unexpected method call on mock");
    }

    @Override
    public Bundle saveHierarchyState() {
        throw new UnsupportedOperationException("Unexpected method call on mock");
    }

    @Override
    public void restoreHierarchyState(Bundle bundle) {
        throw new UnsupportedOperationException("Unexpected method call on mock");
    }

    @Override
    protected void onActive() {
        throw new UnsupportedOperationException("Unexpected method call on mock");
    }

    @Override
    public void setChildDrawable(int i, Drawable drawable) {
        throw new UnsupportedOperationException("Unexpected method call on mock");
    }

    @Override
    public void setChildInt(int i, int i1) {
        throw new UnsupportedOperationException("Unexpected method call on mock");
    }

    @Override
    public boolean isShortcutKey(int i, KeyEvent keyEvent) {
        throw new UnsupportedOperationException("Unexpected method call on mock");
    }

    @Override
    public void setVolumeControlStream(int i) {
        throw new UnsupportedOperationException("Unexpected method call on mock");
    }

    @Override
    public int getVolumeControlStream() {
        throw new UnsupportedOperationException("Unexpected method call on mock");
    }

    @Override
    public int getStatusBarColor() {
        throw new UnsupportedOperationException("Unexpected method call on mock");
    }

    @Override
    public void setStatusBarColor(int i) {
        throw new UnsupportedOperationException("Unexpected method call on mock");
    }

    @Override
    public int getNavigationBarColor() {
        throw new UnsupportedOperationException("Unexpected method call on mock");
    }

    @Override
    public void setNavigationBarColor(int i) {
        throw new UnsupportedOperationException("Unexpected method call on mock");
    }
}
