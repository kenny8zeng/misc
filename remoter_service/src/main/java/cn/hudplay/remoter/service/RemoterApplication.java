package cn.hudplay.remoter.service;

import android.app.Activity;
import android.app.AlarmManager;
import android.app.Application;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.util.Log;

public class RemoterApplication extends Application {

    private static final String TAG = "RemoterApplication";

    private static RemoterApplication mRemoterApp = null;

    private static Context mApplicationContext = null;

    public static final String ALARM_MANAGER_ACTION = "cn.hudplay.remoter.service.action.alarm";

    @Override
    public void onCreate() {

        super.onCreate();
        Log.e(TAG, "remoter application is on create");
        mRemoterApp = this;
        if (mApplicationContext == null) {
            mApplicationContext = getApplicationContext();
        }
        invokeTimerService(getApplicationContext(), ALARM_MANAGER_ACTION);
    }

    @Override
    public void onTrimMemory(int level) {

        super.onTrimMemory(level);
        if (level >= TRIM_MEMORY_MODERATE) {

        }
    }

    public static RemoterApplication getInstance() {

        return mRemoterApp;
    }

    @Override
    public void onTerminate() {

        super.onTerminate();
        cancelAlarmManager(getApplicationContext(), ALARM_MANAGER_ACTION);
    }

    private void invokeTimerService(Context context, String action) {

        AlarmManager alarmManager = (AlarmManager) context.getSystemService(Activity.ALARM_SERVICE);
        Intent intent = new Intent(context, RemoterReceiver.class);
        intent.setAction(action);
        PendingIntent pendingIntent = PendingIntent.getBroadcast(context, 0, intent, PendingIntent.FLAG_UPDATE_CURRENT);
        long now = System.currentTimeMillis();
        alarmManager.setInexactRepeating(AlarmManager.RTC_WAKEUP, now, 60 * 1000/*一分钟*/, pendingIntent);
    }

    private void cancelAlarmManager(Context context, String action) {

        Intent intent = new Intent(context, RemoterReceiver.class);
        intent.setAction(action);
        PendingIntent pendingIntent = PendingIntent.getBroadcast(context, 0, intent, PendingIntent.FLAG_UPDATE_CURRENT);
        AlarmManager alarmManager = (AlarmManager) context.getSystemService(Activity.ALARM_SERVICE);
        alarmManager.cancel(pendingIntent);
    }
}