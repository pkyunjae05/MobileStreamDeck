package com.pkj.app.streamdeckcontroller;

import android.app.Activity;
import android.content.Intent;
import android.hardware.usb.UsbAccessory;
import android.hardware.usb.UsbManager;
import android.os.Bundle;
import android.util.Log;

import com.pkj.app.streamdeckcontroller.usb.AoaCommunicationManager;

public class MainActivity extends Activity {
    private static final String TAG = "MainActivity";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        // setContentView(R.layout.activity_main); // 필요시 레이아웃 연결
        Log.d(TAG, "MainActivity 생성됨 (onCreate)");

        handleUsbIntent(getIntent());
    }

    @Override
    protected void onNewIntent(Intent intent) {
        super.onNewIntent(intent);
        setIntent(intent);
        Log.d(TAG, "새로운 Intent 수신됨 (onNewIntent)");

        handleUsbIntent(intent);
    }

    private void handleUsbIntent(Intent intent) {
        if (intent != null) Log.d(TAG, "onReceive: " + intent.getAction());

        if (intent != null && UsbManager.ACTION_USB_ACCESSORY_ATTACHED.equals(intent.getAction())) {
            UsbAccessory accessory = intent.getParcelableExtra(UsbManager.EXTRA_ACCESSORY);
            if (accessory != null) {
                Log.d(TAG, "PC(AOA 장치) 감지됨: " + accessory.getModel());

                // 싱글톤 매니저를 통해 Accessory Open!
                AoaCommunicationManager.getInstance().openAccessory(this, accessory);
            }
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        Log.d(TAG, "MainActivity 종료됨 (onDestroy)");
    }
}