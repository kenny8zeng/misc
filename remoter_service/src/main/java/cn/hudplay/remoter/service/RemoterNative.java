package cn.hudplay.remoter.service;

public class RemoterNative {

    static {
        System.loadLibrary("remoter_service");
    }

    public static native void init();
}