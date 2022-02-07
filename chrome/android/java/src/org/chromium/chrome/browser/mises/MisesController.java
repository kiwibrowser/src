package org.chromium.chrome.browser.mises;

import org.chromium.base.Log;
import org.chromium.base.ThreadUtils;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.chrome.browser.preferences.ChromePreferenceManager;
import org.chromium.chrome.browser.profiles.Profile;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;

public class MisesController {
    private static final String TAG = "MisesController";
    public static final String MISES_EXTENSION_KEY = "nkbihfbeogaeaoehlefnkodbefgpgknn";
	private String mMisesId = "";
	private String mMisesToken = "";
	private String mMisesNickname = "";
	private String mMisesAvatar = "";
	private String mLastShareIcon = "";
	private String mLastShareTitle = "";
	private String mLastShareUrl = "";

	private static MisesController sInstance;

	public interface MisesControllerObserver {
	    void OnMisesUserInfoChanged();
    }

    ArrayList<MisesControllerObserver> observers_ = new ArrayList<>();

	public void AddObserver(MisesControllerObserver observer) {
	    observers_.add(observer);
    }

    public void RemoveObserver(MisesControllerObserver observer) {
	    observers_.remove(observer);
    }

    public static MisesController getInstance() {
        ThreadUtils.assertOnUiThread();
        if (sInstance == null) {
            sInstance = new MisesController();
            String json = ChromePreferenceManager.getInstance().getMisesUserInfo();
            if (json == null || json.isEmpty()) {
                sInstance.mMisesId = "";
                sInstance.mMisesToken = "";
                sInstance.mMisesNickname = "";
                sInstance.mMisesAvatar = "";
            } else {
                try {
                    JSONObject jsonMessage = new JSONObject(json);
                    if (jsonMessage.has("misesId")) {
                        sInstance.mMisesId = jsonMessage.getString("misesId");
                    }
                    if (jsonMessage.has("token")) {
                        sInstance.mMisesToken = jsonMessage.getString("token");
                    }
                    if (jsonMessage.has("nickname")) {
                        sInstance.mMisesNickname = jsonMessage.getString("nickname");
                    }
                    if (jsonMessage.has("avatar")) {
                        sInstance.mMisesAvatar = jsonMessage.getString("avatar");
                    }
                } catch (JSONException e) {
                    Log.e(TAG, "setMisesUserInfo from cache %s error", json);
                }
            }
        }
        return sInstance;
    }

    @CalledByNative
    public static void setMisesUserInfo(String json) {
        MisesController instance = getInstance();
    	if (json == null || json.isEmpty()) {
    	    instance.mMisesId = "";
    	    instance.mMisesToken = "";
    	    instance.mMisesNickname = "";
    	    instance.mMisesAvatar = "";
    	    ChromePreferenceManager.getInstance().setMisesUserInfo("");
        } else {
            try {
                JSONObject jsonMessage = new JSONObject(json);
                if (jsonMessage.has("misesId")) {
                    instance.mMisesId = jsonMessage.getString("misesId");
                }
                if (jsonMessage.has("token")) {
                    instance.mMisesToken = jsonMessage.getString("token");
                }
                if (jsonMessage.has("nickname")) {
                    instance.mMisesNickname = jsonMessage.getString("nickname");
                }
                if (jsonMessage.has("avatar")) {
                    instance.mMisesAvatar = jsonMessage.getString("avatar");
                } else {
                    instance.mMisesAvatar = "";
                }
                ChromePreferenceManager.getInstance().setMisesUserInfo(json);
            } catch (JSONException e) {
                Log.e(TAG, "setMisesUserInfo from plugin %s error", json);
            }
        }

    	for (MisesControllerObserver observer : instance.observers_) {
    	    observer.OnMisesUserInfoChanged();
        }
    }

    public boolean isLogin() {
        return mMisesToken != null && !mMisesToken.isEmpty();
    }

    public String getMisesId() {
        return mMisesId;
    }

    public String getMisesToken() {
        return mMisesToken;
    }

    public String getMisesNickname() {
        return mMisesNickname;
    }

    public String getMisesAvatar() {
        return mMisesAvatar;
    }

    public void setLastShareInfo(String icon, String title, String url) {
	    mLastShareIcon = icon;
	    mLastShareTitle = title;
	    mLastShareUrl = url;
    }

    public String getLastShareIcon() {
        return mLastShareIcon;
    }

    public String getLastShareTitle() {
        return mLastShareTitle;
    }

    public String getLastShareUrl() {
        return mLastShareUrl;
    }

    public void clearLastShareInfo() {
	    mLastShareUrl = "";
	    mLastShareTitle = "";
	    mLastShareIcon = "";
    }
}