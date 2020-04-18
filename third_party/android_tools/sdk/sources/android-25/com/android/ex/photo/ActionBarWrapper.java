package com.android.ex.photo;

import android.app.ActionBar;
import android.graphics.drawable.Drawable;

/**
 * Wrapper around {@link ActionBar}.
 */
public class ActionBarWrapper implements ActionBarInterface {

    private final ActionBar mActionBar;

    private class MenuVisiblityListenerWrapper implements ActionBar.OnMenuVisibilityListener {

        private final ActionBarInterface.OnMenuVisibilityListener mWrapped;

        public MenuVisiblityListenerWrapper(ActionBarInterface.OnMenuVisibilityListener wrapped) {
            mWrapped = wrapped;
        }

        @Override
        public void onMenuVisibilityChanged(boolean isVisible) {
            mWrapped.onMenuVisibilityChanged(isVisible);
        }
    }

    public ActionBarWrapper(ActionBar actionBar) {
        mActionBar = actionBar;
    }

    @Override
    public void setDisplayHomeAsUpEnabled(boolean showHomeAsUp) {
        mActionBar.setDisplayHomeAsUpEnabled(showHomeAsUp);
    }

    @Override
    public void addOnMenuVisibilityListener(OnMenuVisibilityListener listener) {
        mActionBar.addOnMenuVisibilityListener(new MenuVisiblityListenerWrapper(listener));
    }

    @Override
    public void setDisplayOptionsShowTitle() {
        mActionBar.setDisplayOptions(ActionBar.DISPLAY_SHOW_TITLE, ActionBar.DISPLAY_SHOW_TITLE);
    }

    @Override
    public CharSequence getTitle() {
       return mActionBar.getTitle();
    }

    @Override
    public void setTitle(CharSequence title) {
        mActionBar.setTitle(title);
    }

    @Override
    public void setSubtitle(CharSequence subtitle) {
        mActionBar.setSubtitle(subtitle);
    }

    @Override
    public void show() {
        mActionBar.show();
    }

    @Override
    public void hide() {
        mActionBar.hide();
    }

    @Override
    public void setLogo(Drawable logo) {
        mActionBar.setLogo(logo);
    }

}
