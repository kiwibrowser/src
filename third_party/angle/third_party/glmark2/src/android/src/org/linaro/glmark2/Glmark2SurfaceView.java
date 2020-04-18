package org.linaro.glmark2;

import java.io.File;

import android.graphics.PixelFormat;
import android.opengl.GLSurfaceView;
import android.app.Activity;
import android.util.Log;

import javax.microedition.khronos.egl.EGL10;
import javax.microedition.khronos.egl.EGLDisplay;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

class Glmark2SurfaceView extends GLSurfaceView {

    public static final String LOG_TAG = "glmark2";

    public Glmark2SurfaceView(Activity activity) {
        super(activity);
        mActivity = activity;

        setEGLContextClientVersion(2);

        setEGLConfigChooser(getConfigChooser());

        setRenderer(new Glmark2Renderer(this));
    }

    private EGLConfigChooser getConfigChooser() {
        String args = mActivity.getIntent().getStringExtra("args");

        if (args == null)
            args = "";

        String[] argv = args.split(" ");

        /* Find the visual-config option argument */
        String configString = new String();
        boolean keepNext = false;
        for (String arg : argv) {
            if (keepNext) {
                configString = arg;
                break;
            }

            if (arg.equals("--visual-config"))
                keepNext = true;
        }

        /* Parse the config string parameters */
        String[] configParams = configString.split(":");
        GLVisualConfig targetConfig = new GLVisualConfig(5, 6, 5, 0, 16, 0, 1);

        for (String param : configParams) {
            String[] paramKeyValue = param.split("=");
            if (paramKeyValue.length < 2)
                continue;

            if (paramKeyValue[0].equals("red") || paramKeyValue[0].equals("r"))
                targetConfig.red = Integer.parseInt(paramKeyValue[1]);
            else if (paramKeyValue[0].equals("green") || paramKeyValue[0].equals("g"))
                targetConfig.green = Integer.parseInt(paramKeyValue[1]);
            else if (paramKeyValue[0].equals("blue") || paramKeyValue[0].equals("b"))
                targetConfig.blue = Integer.parseInt(paramKeyValue[1]);
            else if (paramKeyValue[0].equals("alpha") || paramKeyValue[0].equals("a"))
                targetConfig.alpha = Integer.parseInt(paramKeyValue[1]);
            else if (paramKeyValue[0].equals("depth") || paramKeyValue[0].equals("d"))
                targetConfig.depth = Integer.parseInt(paramKeyValue[1]);
            else if (paramKeyValue[0].equals("stencil") || paramKeyValue[0].equals("s"))
                targetConfig.stencil = Integer.parseInt(paramKeyValue[1]);
            else if (paramKeyValue[0].equals("buffer") || paramKeyValue[0].equals("buf"))
                targetConfig.buffer = Integer.parseInt(paramKeyValue[1]);
        }

        return new Glmark2ConfigChooser(targetConfig);
    }

    /**
     * EGLConfigChooser that quits with an error dialog when a suitable config
     * cannot be found.
     */
    private class Glmark2ConfigChooser implements EGLConfigChooser {
        private int[] mAttribList;

        public Glmark2ConfigChooser(GLVisualConfig targetConfig)
        {
            mAttribList = new int[] {
                    EGL10.EGL_RED_SIZE, targetConfig.red,
                    EGL10.EGL_GREEN_SIZE, targetConfig.green,
                    EGL10.EGL_BLUE_SIZE, targetConfig.blue,
                    EGL10.EGL_ALPHA_SIZE, targetConfig.alpha,
                    EGL10.EGL_DEPTH_SIZE, targetConfig.depth,
                    EGL10.EGL_STENCIL_SIZE, targetConfig.stencil,
                    EGL10.EGL_BUFFER_SIZE, targetConfig.buffer,
                    EGL10.EGL_RENDERABLE_TYPE, 4, /* 4 = EGL_OPENGL_ES2_BIT */
                    EGL10.EGL_NONE };

            mTargetConfig = targetConfig;
       }

        @Override
        public EGLConfig chooseConfig(EGL10 egl, EGLDisplay display) {
            try {
                return chooseConfigInternal(egl, display);
            }
            catch (Exception e) {
                /* Log an error message */
                Log.e(LOG_TAG, "No suitable EGLConfig for GLES2.0 found. Please check that proper GLES2.0 drivers are installed.");
                /* Display an informative (and lethal for the app) dialog */
                mActivity.runOnUiThread(new Runnable() {
                    public void run() {
                        mActivity.showDialog(Glmark2Activity.DIALOG_EGLCONFIG_FAIL_ID);
                    }
                });

                /* Wait here until the app process gets killed... */
                synchronized (this) {
                    try { this.wait(); } catch (Exception ex) { }
                }
            }
            return null;
        }

        private EGLConfig chooseConfigInternal(EGL10 egl, EGLDisplay display) {
            /* Get the number of available configs matching the attributes */
            int[] num_config = new int[1];
            if (!egl.eglChooseConfig(display, mAttribList, null, 0, num_config)) {
                throw new IllegalArgumentException("eglChooseConfig failed");
            }

            int numConfigs = num_config[0];

            if (numConfigs <= 0) {
                throw new IllegalArgumentException("No matching configs found");
            }

            /* Get the matching configs */
            EGLConfig[] configs = new EGLConfig[numConfigs];
            if (!egl.eglChooseConfig(display, mAttribList, configs, numConfigs,
                                     num_config))
            {
                throw new IllegalArgumentException("eglChooseConfig#2 failed");
            }

            /* Find the best matching config. */
            int bestScore = Integer.MIN_VALUE;
            EGLConfig bestConfig = configs[0];

            for (EGLConfig config : configs) {
                GLVisualConfig vc = new GLVisualConfig();
                vc.red = findConfigAttrib(egl, display, config,
                                          EGL10.EGL_RED_SIZE, 0);
                vc.green = findConfigAttrib(egl, display, config,
                                            EGL10.EGL_GREEN_SIZE, 0);
                vc.blue = findConfigAttrib(egl, display, config,
                                           EGL10.EGL_BLUE_SIZE, 0);
                vc.alpha = findConfigAttrib(egl, display, config,
                                            EGL10.EGL_ALPHA_SIZE, 0);
                vc.depth = findConfigAttrib(egl, display, config,
                                            EGL10.EGL_DEPTH_SIZE, 0);
                vc.stencil = findConfigAttrib(egl, display, config,
                                            EGL10.EGL_STENCIL_SIZE, 0);
                vc.buffer = findConfigAttrib(egl, display, config,
                                             EGL10.EGL_BUFFER_SIZE, 0);

                int score = Glmark2Native.scoreConfig(vc, mTargetConfig);

                if (score > bestScore) {
                    bestScore = score;
                    bestConfig = config;
                }
            }

            return bestConfig;
        }

        private int findConfigAttrib(EGL10 egl, EGLDisplay display, EGLConfig config,
                                     int attribute, int defaultValue)
        {
            int[] value = new int[] { defaultValue };
            egl.eglGetConfigAttrib(display, config, attribute, value);
            return value[0];
        }

        protected GLVisualConfig mTargetConfig;
    }

    public Activity getActivity() {
        return mActivity;
    }

    private Activity mActivity;

}

class Glmark2Renderer implements GLSurfaceView.Renderer {
    public Glmark2Renderer(Glmark2SurfaceView view) {
        mView = view;
    }

    public void onDrawFrame(GL10 gl) {
        if (!Glmark2Native.render())
            mView.getActivity().finish();
    }

    public void onSurfaceChanged(GL10 gl, int width, int height) {
        Glmark2Native.resize(width, height);
    }

    public void onSurfaceCreated(GL10 gl, EGLConfig config) {
        String args = mView.getActivity().getIntent().getStringExtra("args");
        File f = new File(mView.getActivity().getFilesDir(), "last_run.log");
        Glmark2Native.init(mView.getActivity().getAssets(), args, f.getAbsolutePath());
    }

    private Glmark2SurfaceView mView;
}
