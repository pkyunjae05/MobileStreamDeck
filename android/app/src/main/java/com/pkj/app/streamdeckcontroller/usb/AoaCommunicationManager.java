package com.pkj.app.streamdeckcontroller.usb;

import android.content.Context;
import android.hardware.usb.UsbAccessory;
import android.hardware.usb.UsbManager;
import android.os.ParcelFileDescriptor;
import android.util.Log;

import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.TimeUnit;

public class AoaCommunicationManager {
    private static final String TAG = "AoaCommunicationManager";
    private static final AoaCommunicationManager instance = new AoaCommunicationManager();

    private UsbManager usbManager;
    private ParcelFileDescriptor fileDescriptor;
    private FileInputStream inputStream;
    private FileOutputStream outputStream;
    private Thread readThread;
    private Thread writeThread;
    private Thread handshakeThread;
    private volatile boolean isConnected = false;
    private volatile boolean isHandshakeCompleted = false;

    // [FIX] 기존 sendCommand()는 호출될 때마다 새 Thread를 생성해서 write()를 실행했음.
    //       HELLO_ANDROID 반복 전송 스레드와 실제 write 스레드가 동시에 여러 개 뜰 수 있어
    //       synchronized 키워드가 사실상 무의미했고, write 순서 보장도 안 됐음.
    //       단일 writer 스레드 + 큐 구조로 교체해 전송을 직렬화함.
    private final BlockingQueue<String> sendQueue = new ArrayBlockingQueue<>(32);

    private AoaCommunicationManager() {}

    public static AoaCommunicationManager getInstance() {
        return instance;
    }

    public void openAccessory(Context context, UsbAccessory accessory) {
        usbManager = (UsbManager) context.getSystemService(Context.USB_SERVICE);
        if (usbManager == null) return;

        fileDescriptor = usbManager.openAccessory(accessory);
        if (fileDescriptor != null) {
            java.io.FileDescriptor fd = fileDescriptor.getFileDescriptor();
            inputStream = new FileInputStream(fd);
            outputStream = new FileOutputStream(fd);
            isConnected = true;
            isHandshakeCompleted = false;
            sendQueue.clear();

            Log.d(TAG, "AOA Connection Opened Successfully");

            startReadThread();
            startWriteThread();

            handshakeThread = new Thread(() -> {
                while (isConnected && !isHandshakeCompleted) {
                    sendCommand("HELLO_ANDROID");
                    try {
                        // [FIX] PC 쪽 재열거~claim_interface 완료까지 로그 상 약 5.7초가 걸렸음.
                        //       500ms 간격으로 계속 write()를 시도하면 PC가 아직 읽지 않는 상태에서
                        //       송신 버퍼가 계속 쌓일 수 있어 간격을 늘림.
                        Thread.sleep(1000);
                    } catch (InterruptedException e) {
                        break;
                    }
                }
            });
            handshakeThread.start();
        } else {
            Log.e(TAG, "AOA Connection Failed: FileDescriptor is null");
        }
    }

    private void startReadThread() {
        if (readThread != null && readThread.isAlive()) {
            return;
        }

        readThread = new Thread(() -> {
            byte[] buffer = new byte[1024];
            Log.d(TAG, "Read thread started. Waiting for PC data...");

            while (isConnected && !Thread.currentThread().isInterrupted()) {
                try {
                    int bytesRead = inputStream.read(buffer);
                    if (bytesRead > 0) {
                        String receivedMessage = new String(buffer, 0, bytesRead, StandardCharsets.UTF_8).trim();
                        Log.d(TAG, "PC로부터 데이터 수신됨: " + receivedMessage);
                        handleReceivedData(receivedMessage);
                    }
                } catch (IOException e) {
                    Log.e(TAG, "Read thread exception", e);
                    closeConnection();
                    break;
                }
            }
        });
        readThread.start();
    }

    // [ADD] 단일 writer 스레드: 큐에서 순차적으로 꺼내 write. sendCommand()마다 스레드 생성하지 않음.
    private void startWriteThread() {
        if (writeThread != null && writeThread.isAlive()) {
            return;
        }

        writeThread = new Thread(() -> {
            while (isConnected) {
                try {
                    String message = sendQueue.poll(500, TimeUnit.MILLISECONDS);
                    if (message == null) continue;
                    if (outputStream == null) continue;

                    long start = System.currentTimeMillis();
                    outputStream.write(message.getBytes(StandardCharsets.UTF_8));
                    outputStream.flush();
                    long elapsed = System.currentTimeMillis() - start;

                    // [ADD] write()가 비정상적으로 오래 걸리면(호스트가 아직 안 읽는 상태)
                    //       원인 파악을 위해 반드시 로그로 남김.
                    if (elapsed > 300) {
                        Log.w(TAG, "write() took " + elapsed + "ms - host may not be reading yet");
                    }
                    Log.d(TAG, "Command sent to PC: " + message);
                } catch (InterruptedException ignored) {
                    break;
                } catch (IOException e) {
                    Log.e(TAG, "Failed to send command, closing connection", e);
                    closeConnection();
                    break;
                }
            }
        });
        writeThread.start();
    }

    private void handleReceivedData(String message) {
        Log.d(TAG, "[handleReceivedData] message: " + message);
        if (message.equals("HELLO_PC_FROM_APP")) {
            isHandshakeCompleted = true;
        }
    }

    public void sendCommand(String message) {
        if (!isConnected) {
            Log.w(TAG, "Cannot send command: Not connected");
            return;
        }
        // [FIX] 큐가 가득 차면 버리고 경고만 남김(예전처럼 write용 스레드가 무한 누적되는 것 방지)
        if (!sendQueue.offer(message)) {
            Log.w(TAG, "Send queue full, dropping message: " + message);
        }
    }

    public void closeConnection() {
        isConnected = false;
        if (handshakeThread != null) handshakeThread.interrupt();
        if (writeThread != null) writeThread.interrupt();
        if (readThread != null) readThread.interrupt();
        try {
            if (inputStream != null) inputStream.close();
            if (outputStream != null) outputStream.close();
            if (fileDescriptor != null) fileDescriptor.close();
            Log.d(TAG, "AOA Connection Closed");
        } catch (IOException e) {
            Log.e(TAG, "Error closing connection", e);
        }
    }
}
