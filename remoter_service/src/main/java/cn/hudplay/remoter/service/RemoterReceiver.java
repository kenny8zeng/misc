package cn.hudplay.remoter.service;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.util.Log;
import android.widget.Toast;

public class RemoterReceiver extends BroadcastReceiver {

    private static final String TAG = "RemoterReceiver";

    private Context mContext = null;

    @Override
    public void onReceive(Context context, Intent intent) {

        mContext = context;
        String action = intent.getAction();
        Log.e(TAG, "action:" + action);
        if (action.equals(Intent.ACTION_BOOT_COMPLETED)) {
            startRemoterService();
        } else if (action.equals("start.remoter.service")) {
            startRemoterService();
        } else if (action.equals("stop.remoter.service")) {
            stopRemoterService();
        } else if (action.equals("android.intent.action.SIM_STATE_CHANGED")) {
            startRemoterService();
        } else if (action.equals("com.android.intent.action.restart.launcher")) {
            startLauncher();
        } else if (action.equals(RemoterApplication.ALARM_MANAGER_ACTION)) {
            startRemoterService();
        }
    }

    private void startRemoterService() {

        if (mContext != null) {
            Intent remoterServiceIntent = new Intent(mContext, RemoterService.class);
            mContext.startService(remoterServiceIntent);
        }
    }

    private void stopRemoterService() {

        if (mContext != null) {
            Intent remoterServiceIntent = new Intent(mContext, RemoterService.class);
            mContext.stopService(remoterServiceIntent);
        }
    }

    private void startLauncher() {

        try {
            Intent actionIntent = new Intent();
            actionIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            actionIntent.addFlags(Intent.FLAG_ACTIVITY_SINGLE_TOP);
            actionIntent.setAction("com.android.activity.action.launcher");
            actionIntent.addCategory("com.android.activity.category.launcher");
            if (mContext != null) {
                mContext.startActivity(actionIntent);
                Toast.makeText(mContext, "正在重启HUD PLAY", Toast.LENGTH_LONG).show();
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
}
