package com.pkj.app.streamdeckcontroller.tcp;

import android.util.Log;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.nio.charset.StandardCharsets;
import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.TimeUnit;

public class tcpCommunicator {
    private static final String TAG = "tcpCommunicator";
    private static final tcpCommunicator instance = new tcpCommunicator();

    private static final int CONNECT_TIMEOUT_MS = 3000;

    public interface ConnectionCallback {
        void onConnected();
        void onConnectFailed(String reason);
    }

    private Socket socket;
    private InputStream inputStream;
    private OutputStream outputStream;

    private Thread connectThread;
    private Thread readThread;
    private Thread writeThread;
    private Thread handshakeThread;

    private volatile boolean isConnected = false;
    private volatile boolean isHandshakeCompleted = false;

    private final BlockingQueue<String> sendQueue = new ArrayBlockingQueue<>(32);

    private tcpCommunicator() {}

    public static tcpCommunicator getInstance() {
        return instance;
    }

    public void connect(String host, int port, ConnectionCallback callback) {
        if (isConnected) {
            Log.w(TAG, "Already connected, ignoring duplicate connect() call");
            if (callback != null) callback.onConnected();
            return;
        }

        if (connectThread != null && connectThread.isAlive()) {
            Log.w(TAG, "Connect already in progress, ignoring duplicate request");
            return;
        }

        connectThread = new Thread(() -> {
            try {
                Log.d(TAG, "Connecting to " + host + ":" + port + " ...");

                Socket s = new Socket();
                s.connect(new InetSocketAddress(host, port), CONNECT_TIMEOUT_MS);

                socket = s;
                inputStream = s.getInputStream();
                outputStream = s.getOutputStream();
                isConnected = true;
                isHandshakeCompleted = false;
                sendQueue.clear();

                Log.d(TAG, "TCP Connection Opened Successfully: " + host + ":" + port);

                startReadThread();
                startWriteThread();
                startHandshakeThread();

                if (callback != null) callback.onConnected();
            } catch (IOException e) {
                Log.e(TAG, "TCP connect failed", e);
                isConnected = false;
                if (callback != null) callback.onConnectFailed(e.getMessage());
            }
        });
        connectThread.start();
    }

    private void startHandshakeThread() {
        handshakeThread = new Thread(() -> {
            while (isConnected && !isHandshakeCompleted) {
                sendCommand("HELLO_ANDROID");
                try {
                    Thread.sleep(1000);
                } catch (InterruptedException e) {
                    break;
                }
            }
        });
        handshakeThread.start();
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
                    } else if (bytesRead < 0) {
                        Log.w(TAG, "Read thread: peer closed the connection");
                        closeConnection();
                        break;
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

                    outputStream.write(message.getBytes(StandardCharsets.UTF_8));
                    outputStream.flush();
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
        if (!sendQueue.offer(message)) {
            Log.w(TAG, "Send queue full, dropping message: " + message);
        }
    }

    public void closeConnection() {
        isConnected = false;
        isHandshakeCompleted = false;

        if (handshakeThread != null) handshakeThread.interrupt();
        if (writeThread != null) writeThread.interrupt();
        if (readThread != null) readThread.interrupt();

        try {
            if (inputStream != null) inputStream.close();
            if (outputStream != null) outputStream.close();
            if (socket != null) socket.close();
            Log.d(TAG, "TCP Connection Closed");
        } catch (IOException e) {
            Log.e(TAG, "Error closing connection", e);
        }
    }

    public boolean isConnected() {
        return isConnected;
    }
}
