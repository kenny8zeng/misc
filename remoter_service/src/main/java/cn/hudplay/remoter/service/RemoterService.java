package cn.hudplay.remoter.service;

import android.app.Service;
import android.content.Intent;
import android.os.IBinder;
import android.util.Log;

public class RemoterService extends Service {

    private final static String TAG = "RemoterService";

    @Override
    public IBinder onBind(Intent intent) {

        return null;
    }

    @Override
    public void onCreate() {

        super.onCreate();
        RemoterNative.init();
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {

        Log.e(TAG, "remoter service is running...");
        return super.onStartCommand(intent, START_STICKY, startId);
    }

    @Override
    public void onDestroy() {

        super.onDestroy();
    }
}