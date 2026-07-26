package com.pkj.app.streamdeckcontroller.usb;

public interface AoaConnectionListener {
    void onConnectionChanged(boolean isConnected);
    void onMessageReceived(String message);
    void onError(String errorMsg);
}