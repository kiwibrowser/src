package android.databinding.testapp;

import android.databinding.OnRebindCallback;
import android.databinding.ViewDataBinding;
import android.databinding.testapp.databinding.InvalidateAllLayoutBinding;
import android.databinding.testapp.vo.NotBindableVo;

import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;

public class InvalidateAllTest extends BaseDataBinderTest<InvalidateAllLayoutBinding> {

    public InvalidateAllTest() {
        super(InvalidateAllLayoutBinding.class);
    }

    public void testRefreshViaInvalidateAll() throws InterruptedException {
        final Semaphore semaphore = new Semaphore(1);
        semaphore.acquire();
        final NotBindableVo vo = new NotBindableVo("foo");
        initBinder(new Runnable() {
            @Override
            public void run() {
                mBinder.setVo(vo);
                mBinder.addOnRebindCallback(new OnRebindCallback() {
                    @Override
                    public void onBound(ViewDataBinding binding) {
                        super.onBound(binding);
                        semaphore.release();
                    }
                });
            }
        });
        assertTrue(semaphore.tryAcquire(2, TimeUnit.SECONDS));

        assertEquals("foo", mBinder.textView.getText().toString());
        vo.setStringValue("bar");
        mBinder.invalidateAll();

        assertTrue(semaphore.tryAcquire(2, TimeUnit.SECONDS));
        assertEquals("bar", mBinder.textView.getText().toString());

    }
}
