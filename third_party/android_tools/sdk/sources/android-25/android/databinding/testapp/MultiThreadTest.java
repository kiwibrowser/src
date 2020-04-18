package android.databinding.testapp;

import android.databinding.testapp.databinding.MultiThreadLayoutBinding;
import android.test.UiThreadTest;

import java.util.concurrent.CountDownLatch;

public class MultiThreadTest extends BaseDataBinderTest<MultiThreadLayoutBinding> {
    public MultiThreadTest() {
        super(MultiThreadLayoutBinding.class);
    }

    public void testSetOnBackgroundThread() throws Throwable {
        initBinder();
        mBinder.setText("a");
        assertEquals("a", mBinder.getText());
        Thread.sleep(500);
        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                assertEquals("a", mBinder.myTextView.getText().toString());
            }
        });
        mBinder.setText("b");
        Thread.sleep(500);
        assertEquals("b", mBinder.getText());
        runTestOnUiThread(new Runnable() {
            @Override
            public void run() {
                assertEquals("b", mBinder.myTextView.getText().toString());
            }
        });
    }
}
