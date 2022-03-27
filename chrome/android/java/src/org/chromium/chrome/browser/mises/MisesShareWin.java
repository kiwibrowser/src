package org.chromium.chrome.browser.mises;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.net.TrafficStats;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Handler;
import android.support.annotation.NonNull;
import android.support.v4.app.DialogFragment;
import android.util.Log;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.PopupWindow;
import android.widget.RelativeLayout;
import android.widget.TextView;
import android.widget.Toast;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.ChromeTabbedActivity;
import org.chromium.chrome.browser.content.ContentUtils;
import org.chromium.chrome.browser.tabmodel.TabCreatorManager;
import org.chromium.chrome.browser.tabmodel.TabModel;
import org.chromium.chrome.browser.widget.LoadingView;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.ByteArrayOutputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.MalformedURLException;
import java.net.URL;

import com.bumptech.glide.Glide;
import com.bumptech.glide.load.DataSource;
import com.bumptech.glide.load.engine.GlideException;
import com.bumptech.glide.load.resource.bitmap.CircleCrop;
import com.bumptech.glide.request.RequestListener;
import com.bumptech.glide.request.RequestOptions;
import com.bumptech.glide.request.target.Target;

import java.io.ByteArrayOutputStream;

import javax.annotation.Nullable;

public class MisesShareWin extends DialogFragment {

    private static final String TAG = "MisesShareWin";

    private Context mContext;

    private FrameLayout view;

    private Button btn_share, btn_cancel;
    private TextView tv_title, tv_url;
    private ImageView image;
    private String mIcon, mTitle, mUrl;
    private ImageResult mImageResult;
    private EditText comment;
    private LoadingView mLoadingView;
    private static final int THREAD_ID = 10000;

    private class ImageResult {
        public String mContentType = "image/png";
        public byte[] mImageData;
    }

    private class MisesShareTask extends AsyncTask<ImageResult, Void, Integer> {
        private String mToken;
        private String mText;
        private String mMisesImageUrl = "";
        public MisesShareTask(String token, String text) {
            mToken = token;
            mText = text;
        }

        private int uploadImageToMises(ImageResult imageResult) {
            int resCode = -1;
            if (imageResult == null || imageResult.mImageData == null || imageResult.mImageData.length == 0)
                return resCode;
            String end = "\r\n";
            String twoHyphens = "--";
            String boundary = "MyBoundary" + System.currentTimeMillis();
            HttpURLConnection urlConnection = null;
            try {
                URL url = new URL("https://apiv2.mises.site/api/v1/upload");
                urlConnection = (HttpURLConnection) url.openConnection();
                urlConnection.setConnectTimeout(20000);
                urlConnection.setDoOutput(true);
                urlConnection.setDoInput(true);
                urlConnection.setUseCaches(false);
                urlConnection.setRequestMethod("POST");
                String userAgent = ContentUtils.getBrowserUserAgent();
                urlConnection.setRequestProperty("User-Agent", userAgent);
                urlConnection.setRequestProperty("Connection", "Keep-alive");
                urlConnection.setRequestProperty("Charset", "UTF-8");
                urlConnection.setRequestProperty("Content-Type", "multipart/form-data; boundary=" + boundary);
                urlConnection.setRequestProperty("Authorization", "Bearer " + mToken);

                DataOutputStream dos = new DataOutputStream(urlConnection.getOutputStream());
                dos.writeBytes(twoHyphens + boundary + end);
                dos.writeBytes("Content-Disposition: form-data; name=\"file_type\"" + end + end);
                dos.writeBytes("image" + end);

                dos.writeBytes(twoHyphens + boundary + end);
                dos.writeBytes("Content-Disposition: form-data; name=\"file\";");
                int nPos = imageResult.mContentType.indexOf("/");
                String ext = ".png";
                if (nPos != -1) {
                    ext = "." + imageResult.mContentType.substring(nPos + 1);
                }
                dos.writeBytes("filename=\"" + System.currentTimeMillis() + ext + "\"" + end);
                dos.writeBytes("Content-Type: " + imageResult.mContentType + end + end);
                dos.write(imageResult.mImageData);
                dos.writeBytes(end);
                String endStr = twoHyphens + boundary + twoHyphens + end;
                dos.write(endStr.getBytes());

                resCode = urlConnection.getResponseCode();
                Log.e(TAG, "upload image to mises " + resCode);

                if (resCode == 200) {
                    InputStream is = urlConnection.getInputStream();
                    ByteArrayOutputStream bo = new ByteArrayOutputStream();
                    int i = is.read();
                    while (i != -1) {
                        bo.write(i);
                        i = is.read();
                    }
                    String resJson = bo.toString();
                    Log.e(TAG, "upload image to mises " + resJson);
                    JSONObject resJsonObject = new JSONObject(resJson);
                    int code = -1;
                    if (resJsonObject.has("code")) {
                        code = resJsonObject.getInt("code");
                    }
                    if (code == 0) {
                        if (resJsonObject.has("data")) {
                            JSONObject dataObj = resJsonObject.getJSONObject("data");
                            if (dataObj.has("path"))
                                mMisesImageUrl = dataObj.getString("path");
                        }
                    }
                } else {
                    InputStream is = urlConnection.getErrorStream();
                    ByteArrayOutputStream bo = new ByteArrayOutputStream();
                    int i = is.read();
                    while (i != -1) {
                        bo.write(i);
                        i = is.read();
                    }
                    String err = bo.toString();
                    Log.e(TAG, "upload image to mises " + err);
                }
            } catch (JSONException e) {
                Log.e(TAG, "Upload image error " + e.toString());
            } catch (MalformedURLException e) {
                Log.e(TAG, "Upload image error " + e.toString());
            } catch (IOException e) {
                Log.e(TAG, "Upload image error " + e.toString());
            } catch (IllegalStateException e) {
                Log.e(TAG, "Upload image error " + e.toString());
            } finally {
                if (urlConnection != null) urlConnection.disconnect();
            }
            return resCode;
        }

        private int PostToMises(String attachUrl) {
            int resCode = -1;
            // if (attachUrl == null || attachUrl.isEmpty())
            //     return resCode;
            HttpURLConnection urlConnection = null;
            try {
                URL url = new URL("https://apiv2.mises.site/api/v1/status");
                urlConnection = (HttpURLConnection) url.openConnection();
                urlConnection.setConnectTimeout(20000);
                urlConnection.setDoOutput(true);
                urlConnection.setDoInput(true);
                urlConnection.setUseCaches(false);
                urlConnection.setRequestMethod("POST");
                String userAgent = ContentUtils.getBrowserUserAgent();
                urlConnection.setRequestProperty("User-Agent", userAgent);
                urlConnection.setRequestProperty("Authorization", "Bearer " + mToken);
                urlConnection.setRequestProperty("Connection", "Keep-alive");
                urlConnection.setRequestProperty("Charset", "UTF-8");
                urlConnection.setRequestProperty("Content-Type", "application/json;charset = utf-8");

                OutputStream outputStream = urlConnection.getOutputStream();
                JSONObject root = new JSONObject();
                root.put("status_type", "link");
                root.put("from_type","forward");
                root.put("content", mText);
                JSONObject linkObject = new JSONObject();
                linkObject.put("title", mTitle);
                URL uri = new URL(mUrl);
                linkObject.put("host", uri.getHost());
                linkObject.put("link", mUrl);
                linkObject.put("attachment_path", attachUrl);
                root.put("link_meta", linkObject);
                String param = root.toString();
                Log.e("mises", "share post to mises : " + param);
                // param = param.replace("\\", "");
                outputStream.write(param.getBytes("UTF-8"));

                resCode = urlConnection.getResponseCode();
                Log.e(TAG, "Share to mises " + resCode);
                if (resCode == 200) {
                    InputStream is = urlConnection.getInputStream();
                    ByteArrayOutputStream bo = new ByteArrayOutputStream();
                    int i = is.read();
                    while (i != -1) {
                        bo.write(i);
                        i = is.read();
                    }
                    String resJson = bo.toString();
                    JSONObject resJsonObject = new JSONObject(resJson);
                    int code = -1;
                    if (resJsonObject.has("code")) {
                        code = resJsonObject.getInt("code");
                    }
                    return code;
                } else {
                    InputStream is = urlConnection.getErrorStream();
                    ByteArrayOutputStream bo = new ByteArrayOutputStream();
                    int i = is.read();
                    while (i != -1) {
                        bo.write(i);
                        i = is.read();
                    }
                    String err = bo.toString();
                    Log.e(TAG, "Share to mises " + err);
                }
            } catch (JSONException e) {
                Log.e(TAG, "Share to mises error " + e.toString());
            } catch (MalformedURLException e) {
                Log.e(TAG, "Share to mises error " + e.toString());
            } catch (IOException e) {
                Log.e(TAG, "Share to mises error " + e.toString());
            } catch (IllegalStateException e) {
                Log.e(TAG, "Share to mises error " + e.toString());
            } finally {
                if (urlConnection != null) urlConnection.disconnect();
            }
            return resCode;
        }

        @Override
        protected Integer doInBackground(ImageResult... imageResults) {
            int res = -1;
            if (imageResults != null && imageResults.length > 0) {
                res = uploadImageToMises(imageResults[0]);
                if (res == 200 && !mMisesImageUrl.isEmpty())
                    res =  PostToMises(mMisesImageUrl);
            } else {
                Log.e("mises", "share image is null");
                res =  PostToMises("");   
            }
            return res;
        }

        @Override
        protected void onPostExecute(Integer res) {
            Log.e(TAG, "mises share action " + res.toString());
            mLoadingView.hideLoadingUI();
            if (res == 0) {
                dismiss();
                Toast.makeText(mContext, "Share success", Toast.LENGTH_SHORT).show();
            } else if (res == 403) {
                MisesController.getInstance().setLastShareInfo(mIcon, mTitle, mUrl);
                dismiss();
                MisesUtil.showAlertDialog(mContext, mContext.getString(R.string.lbl_auth_tip), new View.OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        if (!(mContext instanceof ChromeTabbedActivity))
                            return;
                        ChromeTabbedActivity chromeTabbedActivity = (ChromeTabbedActivity) mContext;
                        TabCreatorManager.TabCreator tabCreator = chromeTabbedActivity.getTabCreator(false);
                        if (tabCreator != null) {
                            tabCreator.openSinglePage("chrome-extension://nkbihfbeogaeaoehlefnkodbefgpgknn/popup.html");
                        }
                    }
                });
            } else {
                Toast.makeText(mContext, "Share failed", Toast.LENGTH_SHORT).show();
            }
        }
    }

    public MisesShareWin() {
    }

    public static MisesShareWin newInstance(String icon, String title, String url) {
        MisesShareWin f = new MisesShareWin();

        // Supply num input as an argument.
        Bundle args = new Bundle();
        args.putString("icon", icon);
        args.putString("title", title);
        args.putString("url", url);
        f.setArguments(args);

        return f;
    }

    @android.support.annotation.Nullable
    @Override
    public View onCreateView(@NonNull LayoutInflater inflater, @android.support.annotation.Nullable ViewGroup container, @android.support.annotation.Nullable Bundle savedInstanceState) {
        this.view = (FrameLayout) inflater.inflate(R.layout.mises_share_dialog, container, false);
        image = view.findViewById(R.id.icon);
        tv_title = view.findViewById(R.id.title);
        tv_url = view.findViewById(R.id.url);
        btn_share = (Button) view.findViewById(R.id.btn_share);
        btn_cancel = (Button) view.findViewById(R.id.btn_cancel);
        comment = (EditText) view.findViewById(R.id.comment);
        mContext = getContext();

        mIcon = getArguments().getString("icon");
        mTitle = getArguments().getString("title");
        mUrl = getArguments().getString("url");
        tv_title.setText(mTitle);
        tv_url.setText(mUrl);
        // 取消按钮
        btn_cancel.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                // 销毁弹出框
                dismiss();
            }
        });
        // 设置按钮监听
        btn_share.setEnabled(false);
        btn_share.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                mLoadingView.showLoadingUI();
                MisesShareTask task = new MisesShareTask(MisesController.getInstance().getMisesToken(), comment.getText().toString().trim());
                task.execute(mImageResult);
            }
        });

        mLoadingView = new LoadingView(mContext);
        FrameLayout.LayoutParams lp = new FrameLayout.LayoutParams(FrameLayout.LayoutParams.WRAP_CONTENT, FrameLayout.LayoutParams.WRAP_CONTENT);
        lp.gravity = Gravity.CENTER;
        mLoadingView.setLayoutParams(lp);
        mLoadingView.setVisibility(View.GONE);
        view.addView(mLoadingView);
        mLoadingView.showLoadingUI();
        Glide.with(mContext).asBitmap().load(mIcon)
                .listener(new RequestListener<Bitmap>() {
                    @Override
                    public boolean onLoadFailed(@Nullable GlideException e, Object model, Target<Bitmap> target, boolean isFirstResource) {
                        Log.e("mises", " MisesShareWin load pic failed" + e.toString() );
                        Toast.makeText(mContext, "Network error", Toast.LENGTH_SHORT).show();
                        mLoadingView.hideLoadingUI();
                        return false;
                    }

                    @Override
                    public boolean onResourceReady(Bitmap resource, Object model, Target<Bitmap> target, DataSource dataSource, boolean isFirstResource) {
                        mLoadingView.hideLoadingUI();
                        if (resource != null) {
                            ByteArrayOutputStream obs = new ByteArrayOutputStream();
                            resource.compress(Bitmap.CompressFormat.PNG, 50, obs);
                            mImageResult = new ImageResult();
                            mImageResult.mImageData = obs.toByteArray();
                            btn_share.setEnabled(true);
                        }
                        return false;
                    }
                })
                .into(image);
        return view;
    }

    @Override
    public void onStart() {
        WindowManager.LayoutParams params = getDialog().getWindow().getAttributes();
        params.width = ViewGroup.LayoutParams.MATCH_PARENT;
        getDialog().getWindow().setAttributes((WindowManager.LayoutParams) params);
        super.onStart();
    }
}
