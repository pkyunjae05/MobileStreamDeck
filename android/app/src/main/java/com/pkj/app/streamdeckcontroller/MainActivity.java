package com.pkj.app.streamdeckcontroller;

import android.app.Activity;
import android.content.Intent;
import android.hardware.usb.UsbAccessory;
import android.hardware.usb.UsbManager;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.ImageButton;
import android.widget.Toast;

import com.pkj.app.streamdeckcontroller.usb.AoaCommunicationManager;

public class MainActivity extends Activity {
    private static final String TAG = "MainActivity";

    // [ADD] 버튼이 늘어나도 이 리스너 하나로 전부 처리.
    //       클릭된 View의 tag(=전송할 명령 문자열)를 그대로 읽어 sendCommand()에 전달.
    private final View.OnClickListener actionClickListener = v -> {
        Object tag = v.getTag();
        if (!(tag instanceof String)) {
            Log.w(TAG, "클릭된 뷰에 tag(명령 문자열)가 설정되어 있지 않습니다: " + v);
            return;
        }
        String command = (String) tag;
        sendActionCommand(command);
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        Log.d(TAG, "MainActivity 생성됨 (onCreate)");

        bindActionButtons();
        handleUsbIntent(getIntent());
    }

    // [ADD] 액션 버튼들에 공용 리스너 연결.
    //       버튼이 늘어나면 findViewById 한 줄만 추가하면 됨.
    private void bindActionButtons() {
        ImageButton youtubeButton = findViewById(R.id.btn_action_youtube);
        if (youtubeButton != null) {
            youtubeButton.setOnClickListener(actionClickListener);
        }

        // [향후 확장 예시]
        // ImageButton spotifyButton = findViewById(R.id.btn_action_spotify);
        // if (spotifyButton != null) {
        //     spotifyButton.setOnClickListener(actionClickListener);
        // }
    }

    // [ADD] 연결 상태를 확인한 뒤 명령을 전송하고, 결과를 사용자에게 즉시 피드백.
    private void sendActionCommand(String command) {
        AoaCommunicationManager manager = AoaCommunicationManager.getInstance();

        if (!manager.isConnected()) {
            Log.w(TAG, "PC와 연결되어 있지 않아 명령을 전송할 수 없습니다: " + command);
            Toast.makeText(this, "PC와 연결되어 있지 않습니다.", Toast.LENGTH_SHORT).show();
            return;
        }

        manager.sendCommand(command);
        Log.d(TAG, "명령 전송 요청: " + command);
        Toast.makeText(this, command + " 전송됨", Toast.LENGTH_SHORT).show();
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
