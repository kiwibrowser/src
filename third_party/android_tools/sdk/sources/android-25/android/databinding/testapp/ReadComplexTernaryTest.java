package android.databinding.testapp;

import android.test.UiThreadTest;
import android.databinding.testapp.databinding.ReadComplexTernaryBinding;

import android.databinding.testapp.vo.User;

public class ReadComplexTernaryTest extends BaseDataBinderTest<ReadComplexTernaryBinding> {
    public ReadComplexTernaryTest() {
        super(ReadComplexTernaryBinding.class);
    }

    @UiThreadTest
    public void testWhenNull() {
        initBinder();
        User user = new User();
        user.setName("a");
        user.setFullName("a b");
        mBinder.setUser(user);
        mBinder.executePendingBindings();
        assertEquals("?", mBinder.textView.getText().toString());
    }
}
