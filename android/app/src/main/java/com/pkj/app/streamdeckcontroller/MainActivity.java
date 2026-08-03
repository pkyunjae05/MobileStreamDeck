package com.pkj.app.streamdeckcontroller;

import android.app.Activity;
import android.content.Intent;
import android.hardware.usb.UsbAccessory;
import android.hardware.usb.UsbManager;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ImageButton;
import android.widget.Toast;

import com.pkj.app.streamdeckcontroller.tcp.tcpCommunicator;
import com.pkj.app.streamdeckcontroller.usb.usbCommunicator;

public class MainActivity extends Activity {
    private static final String TAG = "MainActivity";
    private static final int TCP_PORT = 9999;
    private EditText ipAddressInput;

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
        bindConnectButton();
        handleUsbIntent(getIntent());
    }

    private void bindActionButtons() {
        ImageButton youtubeButton = findViewById(R.id.btn_action_youtube);
        if (youtubeButton != null) {
            youtubeButton.setOnClickListener(actionClickListener);
        }

        // ImageButton spotifyButton = findViewById(R.id.btn_action_spotify);
        // if (spotifyButton != null) {
        //     spotifyButton.setOnClickListener(actionClickListener);
        // }
    }

    // [ADD] 하단 IP 입력 + 연결 버튼 바인딩.
    //       버튼 클릭 시 입력된 IP로 tcpCommunicator를 통해 PC(TcpManager)에 접속을 시도한다.
    private void bindConnectButton() {
        ipAddressInput = findViewById(R.id.et_ip_address);
        Button connectButton = findViewById(R.id.btn_connect);
        if (connectButton == null) return;

        connectButton.setOnClickListener(v -> {
            String ip = ipAddressInput.getText().toString().trim();
            if (ip.isEmpty()) {
                Toast.makeText(this, "IP 주소를 입력하세요.", Toast.LENGTH_SHORT).show();
                return;
            }

            Toast.makeText(this, ip + " 로 연결 시도 중...", Toast.LENGTH_SHORT).show();
            Log.d(TAG, "TCP 연결 시도: " + ip + ":" + TCP_PORT);

            tcpCommunicator.getInstance().connect(ip, TCP_PORT, new tcpCommunicator.ConnectionCallback() {
                @Override
                public void onConnected() {
                    runOnUiThread(() ->
                            Toast.makeText(MainActivity.this, "연결 성공: " + ip, Toast.LENGTH_SHORT).show());
                }

                @Override
                public void onConnectFailed(String reason) {
                    runOnUiThread(() ->
                            Toast.makeText(MainActivity.this, "연결 실패: " + reason, Toast.LENGTH_SHORT).show());
                }
            });
        });
    }

    private void sendActionCommand(String command) {
        usbCommunicator aoaManager = usbCommunicator.getInstance();
        tcpCommunicator tcpManager = tcpCommunicator.getInstance();

        if (aoaManager.isConnected()) {
            aoaManager.sendCommand(command);
        } else if (tcpManager.isConnected()) {
            tcpManager.sendCommand(command);
        } else {
            Log.w(TAG, "PC와 연결되어 있지 않아 명령을 전송할 수 없습니다: " + command);
            Toast.makeText(this, "PC와 연결되어 있지 않습니다.", Toast.LENGTH_SHORT).show();
            return;
        }

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

                usbCommunicator.getInstance().openAccessory(this, accessory);
            }
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        Log.d(TAG, "MainActivity 종료됨 (onDestroy)");

        tcpCommunicator.getInstance().closeConnection();
    }
}
