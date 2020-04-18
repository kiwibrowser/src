package android.databinding.testapp.vo;

import android.databinding.BaseObservable;
import android.databinding.Bindable;
import android.databinding.testapp.BR;

public class User extends BaseObservable {
    @Bindable
    private User friend;
    @Bindable
    private String name;
    @Bindable
    private String fullName;

    public User getFriend() {
        return friend;
    }

    public void setFriend(User friend) {
        this.friend = friend;
        notifyPropertyChanged(BR.friend);
    }

    public String getName() {
        return name;
    }

    public void setName(String name) {
        this.name = name;
        notifyPropertyChanged(BR.name);
    }

    public String getFullName() {
        return fullName;
    }

    public void setFullName(String fullName) {
        this.fullName = fullName;
        notifyPropertyChanged(BR.fullName);
    }
}
