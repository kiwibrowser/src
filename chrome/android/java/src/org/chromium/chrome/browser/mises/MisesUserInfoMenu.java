package org.chromium.chrome.browser.mises;

import android.content.Context;
import android.graphics.drawable.ColorDrawable;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.PopupWindow;
import android.widget.RelativeLayout;
import android.widget.TextView;

import org.chromium.chrome.R;

public class MisesUserInfoMenu extends PopupWindow {

    private Context mContext;

    private View view;
    private View view_user_info, view_login;
    private TextView tvUsername, tvId;
    private ImageView ivAvatar;

    public MisesUserInfoMenu(Context context, String id, String name, String avatar) {

        mContext = context;
        this.view = LayoutInflater.from(mContext).inflate(R.layout.mises_user_info_menu, null);
        view_user_info = view.findViewById(R.id.view_user_info);
        view_login = view.findViewById(R.id.view_login);
        tvUsername = (TextView) view.findViewById(R.id.tv_username);
        tvId = (TextView) view.findViewById(R.id.tv_id);
        ivAvatar = (ImageView) view.findViewById(R.id.iv_avatar);
        boolean isLogin = MisesController.getInstance().isLogin();
        if (isLogin) {
            view_user_info.setVisibility(View.VISIBLE);
            view_login.setVisibility(View.GONE);
            tvUsername.setText(name);
            tvId.setText(id);
        } else {
            view_user_info.setVisibility(View.GONE);
            view_login.setVisibility(View.VISIBLE);
        }

        // 设置外部可点击
        this.setOutsideTouchable(true);

        view.setOnTouchListener(new View.OnTouchListener() {
            @Override
            public boolean onTouch(View v, MotionEvent event) {
                View view = view_login;
                if (view.getVisibility() == View.GONE) {
                    view = view_user_info;
                }
                int width = view.getRight();
                int w = (int) event.getX();
                if (event.getAction() == MotionEvent.ACTION_UP) {
                    if ( w > width) {
                        dismiss();
                    }
                }
                return true;
            }
        });

        /* 设置弹出窗口特征 */
        // 设置视图
        this.setContentView(this.view);
        // 设置弹出窗体的宽和高
        this.setHeight(RelativeLayout.LayoutParams.MATCH_PARENT);
        this.setWidth(RelativeLayout.LayoutParams.MATCH_PARENT);

        // 设置弹出窗体可点击
        this.setFocusable(true);

        this.setAnimationStyle(R.style.LeftMenuStyle);
    }

    public void setOnClickListener(View.OnClickListener itemsOnClick) {
        view.findViewById(R.id.tv_my_data).setOnClickListener(itemsOnClick);
        view.findViewById(R.id.tv_mises_discover).setOnClickListener(itemsOnClick);
        view.findViewById(R.id.tv_wallet).setOnClickListener(itemsOnClick);
        view.findViewById(R.id.tv_nft).setOnClickListener(itemsOnClick);
        view.findViewById(R.id.btn_switch).setOnClickListener(itemsOnClick);
        view.findViewById(R.id.tv_login).setOnClickListener(itemsOnClick);
        view.findViewById(R.id.tv_create_mises).setOnClickListener(itemsOnClick);
        view.findViewById(R.id.tv_restore_mises).setOnClickListener(itemsOnClick);
    }
} 
