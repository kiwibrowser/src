package org.chromium.chrome.browser.mises;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.net.TrafficStats;
import android.os.AsyncTask;
import android.os.Handler;
import android.util.Log;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.PopupWindow;
import android.widget.RelativeLayout;
import android.widget.TextView;
import android.widget.Toast;

import org.chromium.chrome.R;
import org.chromium.chrome.browser.content.ContentUtils;
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
import java.net.URLEncoder;

public class MisesShareWin extends PopupWindow {

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
    private View pop_layout;
    private Handler mHandler = new Handler();
    private static final int THREAD_ID = 10000;

    private class ImageResult {
        public String mContentType = "image/png";
        public byte[] mImageData;
    }
    private class FetchImageTask extends AsyncTask<String, Void, ImageResult> {
        public FetchImageTask() {

        }

        @Override
        protected ImageResult doInBackground(String... strings) {
            if (strings.length == 0 || strings[0] == null || strings[0].isEmpty())
                return null;
            HttpURLConnection urlConnection = null;
            try {
                URL url = new URL(strings[0]);
                urlConnection = (HttpURLConnection) url.openConnection();
                urlConnection.setConnectTimeout(20000);
                urlConnection.setRequestMethod("GET");
                if (urlConnection.getResponseCode() == 200) {
                    Log.i(TAG, "get icon data ok");
                    InputStream is = urlConnection.getInputStream();
                    ByteArrayOutputStream bo = new ByteArrayOutputStream();
                    int i = is.read();
                    while (i != -1) {
                        bo.write(i);
                        i = is.read();
                    }
                    String contentType = urlConnection.getHeaderField("Content-Type");
                    ImageResult result = new ImageResult();
                    if (contentType != null && !contentType.isEmpty())
                        result.mContentType = contentType;
                    result.mImageData = bo.toByteArray();
                    return result;
                }
            } catch (MalformedURLException e) {
                Log.w(TAG, "fetch image error " + e.toString());
            } catch (IOException e) {
                Log.w(TAG, "fetch image error " + e.toString());
            } catch (IllegalStateException e) {
                Log.w(TAG, "fetch image error " + e.toString());
            } finally {
                if (urlConnection != null) urlConnection.disconnect();
            }
            return null;
        }

        @Override
        protected void onPostExecute(ImageResult result) {
            if (result != null && result.mImageData != null && result.mImageData.length > 0) {
                mImageResult = result;
                Bitmap bitmap = BitmapFactory.decodeByteArray(mImageResult.mImageData, 0, mImageResult.mImageData.length);
                if (bitmap != null)
                    image.setImageBitmap(bitmap);
            }
            mLoadingView.hideLoadingUI();
        }
    }

    private class MisesShareTask extends AsyncTask<ImageResult, Void, Integer> {
        private String mToken;
        private String mText;
        public MisesShareTask(String token, String text) {
            mToken = token;
            mText = text;
        }

        private String uploadImageToMises(ImageResult imageResult) {
            if (imageResult == null || imageResult.mImageData == null || imageResult.mImageData.length == 0)
                return "";
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

                int resCode = urlConnection.getResponseCode();
                Log.d(TAG, "upload image to mises " + resCode);

                if (resCode == 200) {
                    InputStream is = urlConnection.getInputStream();
                    ByteArrayOutputStream bo = new ByteArrayOutputStream();
                    int i = is.read();
                    while (i != -1) {
                        bo.write(i);
                        i = is.read();
                    }
                    String resJson = bo.toString();
                    Log.d(TAG, "upload image to mises " + resJson);
                    JSONObject resJsonObject = new JSONObject(resJson);
                    int code = -1;
                    if (resJsonObject.has("code")) {
                        code = resJsonObject.getInt("code");
                    }
                    if (code == 0) {
                        if (resJsonObject.has("data")) {
                            JSONObject dataObj = resJsonObject.getJSONObject("data");
                            if (dataObj.has("path"))
                                return dataObj.getString("path");
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
                Log.w(TAG, "Upload image error " + e.toString());
            } catch (MalformedURLException e) {
                Log.w(TAG, "Upload image error " + e.toString());
            } catch (IOException e) {
                Log.w(TAG, "Upload image error " + e.toString());
            } catch (IllegalStateException e) {
                Log.w(TAG, "Upload image error " + e.toString());
            } finally {
                if (urlConnection != null) urlConnection.disconnect();
            }
            return "";
        }

        private int PostToMises(String attachUrl) {
            if (attachUrl == null || attachUrl.isEmpty())
                return -1;
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
                urlConnection.setRequestProperty("Content-Type", "application/json");

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
                param = param.replace("\\", "");
                outputStream.write(param.getBytes());

                int resCode = urlConnection.getResponseCode();
                Log.d(TAG, "Share to mises " + resCode);
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
            return -1;
        }

        @Override
        protected Integer doInBackground(ImageResult... imageResults) {
            if (imageResults != null && imageResults.length > 0) {
                String attachUrl = uploadImageToMises(imageResults[0]);
                return PostToMises(attachUrl);
            }
            return -1;
        }

        @Override
        protected void onPostExecute(Integer res) {
            Log.d(TAG, "mises share action " + res.toString());
            mLoadingView.hideLoadingUI();
            if (res == 0) {
                dismiss();
                Toast.makeText(mContext, "Share success", Toast.LENGTH_SHORT).show();
            } else if (res == 403) {
                Toast.makeText(mContext, "Share failed, token expired.", Toast.LENGTH_SHORT).show();
            } else {
                Toast.makeText(mContext, "Share failed", Toast.LENGTH_SHORT).show();
            }
        }
    }

    public MisesShareWin(Context context, String icon, String title, String url) {
        mIcon = icon;
        mTitle = title;
        mUrl = url;
        mContext = context;
        this.view = (FrameLayout) LayoutInflater.from(mContext).inflate(R.layout.mises_share_dialog, null);
        pop_layout = view.findViewById(R.id.pop_layout);
        image = view.findViewById(R.id.icon);
        tv_title = view.findViewById(R.id.title);
        tv_url = view.findViewById(R.id.url);
        btn_share = (Button) view.findViewById(R.id.btn_share);
        btn_cancel = (Button) view.findViewById(R.id.btn_cancel);
        comment = (EditText) view.findViewById(R.id.comment);
        tv_title.setText(title);
        tv_url.setText(url);
        // 取消按钮
        btn_cancel.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                // 销毁弹出框
                dismiss();
            }
        });
        // 设置按钮监听
        btn_share.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                mLoadingView.showLoadingUI();
                MisesShareTask task = new MisesShareTask(MisesController.getInstance().getMisesToken(), comment.getText().toString().trim());
                task.execute(mImageResult);
            }
        });

        // 设置外部可点击
        this.setOutsideTouchable(false);

        /* 设置弹出窗口特征 */
        // 设置视图
        this.setContentView(this.view);
        // 设置弹出窗体的宽和高
        this.setHeight(RelativeLayout.LayoutParams.MATCH_PARENT);
        this.setWidth(RelativeLayout.LayoutParams.MATCH_PARENT);

        // 设置弹出窗体可点击
        this.setFocusable(true);

//        view.setOnTouchListener(new View.OnTouchListener() {
//            @Override
//            public boolean onTouch(View v, MotionEvent event) {
//                int x = (int) event.getX();
//                int y = (int) event.getY();
//                if (event.getAction() == MotionEvent.ACTION_UP) {
//                    if (x < pop_layout.getLeft() || x > pop_layout.getRight()
//                            || y < pop_layout.getTop() || y > pop_layout.getBottom()) {
//                        dismiss();
//                    }
//                }
//                return true;
//            }
//        });
        mLoadingView = new LoadingView(mContext);
        FrameLayout.LayoutParams lp = new FrameLayout.LayoutParams(FrameLayout.LayoutParams.WRAP_CONTENT, FrameLayout.LayoutParams.WRAP_CONTENT);
        lp.gravity = Gravity.CENTER;
        mLoadingView.setLayoutParams(lp);
        mLoadingView.setVisibility(View.GONE);
        view.addView(mLoadingView);

        TrafficStats.setThreadStatsTag(THREAD_ID);

        mHandler.postDelayed(new Runnable() {
            @Override
            public void run() {
                mLoadingView.showLoadingUI();
                FetchImageTask fetchImageTask = new FetchImageTask();
                fetchImageTask.execute(mIcon);
            }
        }, 500);
    }
} 
