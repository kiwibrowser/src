package org.chromium.chrome.browser.init;



import static android.app.Activity.RESULT_OK;

import android.app.Activity;
import android.content.Intent;
import android.util.Log;

import com.google.android.play.core.appupdate.AppUpdateInfo;
import com.google.android.play.core.appupdate.AppUpdateManager;
import com.google.android.play.core.appupdate.AppUpdateManagerFactory;
import com.google.android.play.core.appupdate.AppUpdateOptions;
import com.google.android.play.core.install.InstallStateUpdatedListener;
import com.google.android.play.core.install.model.AppUpdateType;
import com.google.android.play.core.install.model.InstallStatus;
import com.google.android.play.core.install.model.UpdateAvailability;
import com.google.android.play.core.tasks.Task;

import org.chromium.chrome.browser.mises.MisesUtil;

import org.chromium.chrome.R;

public class InAppUpdater {
    final static String TAG = "InAppUpdate";
    final static int MISES_INAPP_UPDATE_REQUEST_CODE = 8001;
    final static int DAYS_FOR_FLEXIBLE_UPDATE = 7;
    private AppUpdateManager appUpdateManager;
    private InstallStateUpdatedListener listener;
    private int updateType = -1;
    public InAppUpdater() {

    }
    public void startCheck(final Activity act) {
        try {
            appUpdateManager = AppUpdateManagerFactory.create(act);

            // Returns an intent object that you use to check for an update.
            Task<AppUpdateInfo> appUpdateInfoTask = appUpdateManager.getAppUpdateInfo();

            // Checks that the platform will allow the specified type of update.
            appUpdateInfoTask.addOnSuccessListener(appUpdateInfo -> {
                if (appUpdateInfo.updateAvailability() == UpdateAvailability.UPDATE_AVAILABLE) {
                    // Request the update.
                    this.startDownload(appUpdateInfo, act);

                } else if (appUpdateInfo.updateAvailability() == UpdateAvailability.DEVELOPER_TRIGGERED_UPDATE_IN_PROGRESS){
		    Log.i(TAG,"update was in progress");
                    if (appUpdateInfo.installStatus() == InstallStatus.DOWNLOADED) {
			popupCompleteUpdate(act);
		    } else {
		        this.startDownload(appUpdateInfo, act);
		    }
		} else {
                    Log.i(TAG,"no update " + String.valueOf(appUpdateInfo.updateAvailability()) );
                }
            }).addOnCompleteListener(appUpdateInfo -> {
                Log.i(TAG,"complete");
            }).addOnFailureListener(ex -> {
                Log.e(TAG,"fail " + ex.toString());
            });
        }catch (Exception ex) {
            Log.e(TAG,"Check flow failed! exception: " + ex.toString());
            appUpdateManager = null;
        }

    }
    private void startDownload(final AppUpdateInfo appUpdateInfo, final Activity act) {
        if (appUpdateManager != null) {
            try {
                AppUpdateOptions option;
                if (appUpdateInfo.updatePriority() >= 4 /* high priority */
                        && appUpdateInfo.isUpdateTypeAllowed(AppUpdateType.IMMEDIATE)) {
                    option = AppUpdateOptions.newBuilder(AppUpdateType.IMMEDIATE)
                            .setAllowAssetPackDeletion(true)
                            .build();
		    updateType = AppUpdateType.IMMEDIATE;
                    Log.i(TAG,"update IMMEDIATE");
                } else if (appUpdateInfo.updatePriority() >= 2 
			//&& appUpdateInfo.clientVersionStalenessDays() != null
		        //&& appUpdateInfo.clientVersionStalenessDays() >= DAYS_FOR_FLEXIBLE_UPDATE
                        && appUpdateInfo.isUpdateTypeAllowed(AppUpdateType.FLEXIBLE)) {
                    option = AppUpdateOptions.newBuilder(AppUpdateType.FLEXIBLE)
                            .setAllowAssetPackDeletion(true)
                            .build();
                    // Create a listener to track request state updates.
                    listener = state -> {
                        // (Optional) Provide a download progress bar.
                        if (state.installStatus() == InstallStatus.DOWNLOADING) {
                            long bytesDownloaded = state.bytesDownloaded();
                            long totalBytesToDownload = state.totalBytesToDownload();
                            // Implement progress bar.
                            Log.i(TAG,"update DOWNLOADING " + String.valueOf(bytesDownloaded) + "/" +  String.valueOf(totalBytesToDownload) );
                        }
                        if (state.installStatus() == InstallStatus.DOWNLOADED) {
                            // After the update is downloaded, show a notification
                            // and request user confirmation to restart the app.
                            popupCompleteUpdate(act);
                            if (listener != null && appUpdateManager != null) {
                                appUpdateManager.unregisterListener(listener);
                                listener = null;
                            }
                        }
                        // Log state or install the update.
                    };
                    appUpdateManager.registerListener(listener);
		    updateType = AppUpdateType.FLEXIBLE;
                    Log.i(TAG,"update FLEXIBLE");
                } else {
                    Log.i(TAG,"update SKIPPED");
                    return;
                }
                appUpdateManager.startUpdateFlowForResult(
                        // Pass the intent that is returned by 'getAppUpdateInfo()'.
                        appUpdateInfo,
                        // The current activity making the update request.
                        act,
                        option,
                        // Include a request code to later monitor this update request.
                        MISES_INAPP_UPDATE_REQUEST_CODE);
            }catch (Exception ex) {
                Log.e(TAG,"Update flow failed! exception: " + ex.toString());
            }
        }

    }
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        if (requestCode == MISES_INAPP_UPDATE_REQUEST_CODE) {
            if (resultCode != RESULT_OK) {
                Log.e(TAG,"Update flow failed! Result code: " + resultCode);
                // If the update is cancelled or fails,
                // you can request to start the update again.
            }
        }
    }
    public void onResume(final Activity act) {
        if (appUpdateManager != null && updateType != -1) {

            appUpdateManager
                    .getAppUpdateInfo()
                    .addOnSuccessListener(
                            appUpdateInfo -> {
                                if (updateType == AppUpdateType.IMMEDIATE && appUpdateInfo.updateAvailability()
                                        == UpdateAvailability.DEVELOPER_TRIGGERED_UPDATE_IN_PROGRESS) {
                                    // If an in-app update is already running, resume the update.
                                    Log.i(TAG,"resume update");
                                    try {
                                        if (appUpdateManager != null) {
                                            appUpdateManager.startUpdateFlowForResult(
                                                    appUpdateInfo,
                                                    AppUpdateType.IMMEDIATE,
                                                    act,
                                                    MISES_INAPP_UPDATE_REQUEST_CODE);
                                        }

                                    }catch (Exception ex) {
                                        Log.e(TAG,"Update flow failed! exception: " + ex.toString());
                                    }
                                }
                                if (updateType == AppUpdateType.FLEXIBLE && appUpdateInfo.installStatus() == InstallStatus.DOWNLOADED) {
                                    popupCompleteUpdate(act);
                                }
                            });
        }


    }

    // Displays the snackbar notification and call to action.
    private void popupCompleteUpdate(final Activity act) {
        Log.i(TAG,"popupCompleteUpdate");
	MisesUtil.showAlertDialog(act, act.getString(R.string.lbl_update_download_complete_tip), v1 -> {
                if (appUpdateManager != null) {
            		appUpdateManager.completeUpdate();
        	}
	});

    }
}
