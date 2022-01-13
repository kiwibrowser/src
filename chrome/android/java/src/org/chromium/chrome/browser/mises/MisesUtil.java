package org.chromium.chrome.browser.mises;

import android.content.Context;
import android.support.v7.app.AlertDialog;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.TextView;
import org.chromium.chrome.R;

public class MisesUtil {

    public static void showAlertDialog(Context context, String message, View.OnClickListener okListener) {
        AlertDialog.Builder builder = new AlertDialog.Builder(context);
        View view = LayoutInflater.from(context).inflate(R.layout.mises_alert_dialog, null);
        builder.setView(view);
        //builder.setCancelable(false);
        TextView tvContent = (TextView) view.findViewById(R.id.tv_content);
        tvContent.setText(message);
        final AlertDialog dialog = builder.create();
        dialog.show();
        view.findViewById(R.id.btn_confirm).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (okListener != null) {
                    okListener.onClick(v);
                }
                dialog.dismiss();
            }
        });
        view.findViewById(R.id.btn_cancel).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                dialog.dismiss();
            }
        });
    }
} 
