package com.foundryengine.game;

import android.app.Activity;
import android.content.pm.ActivityInfo;
import android.content.pm.PackageManager;
import android.content.res.AssetManager;
import android.os.Bundle;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.Toast;

import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

/**
 * Main Android Game Activity
 * Interfaces with FoundryEngine native C++ code
 */
public class GameActivity extends Activity implements SurfaceHolder.Callback {
    private static final String TAG = "GameActivity";
    private static final int PERMISSION_REQUEST_CODE = 100;

    // Native methods
    private native void nativeOnCreate();
    private native void nativeOnDestroy();
    private native void nativeOnSurfaceCreated(Surface surface);
    private native void nativeOnSurfaceChanged(int width, int height);
    private native void nativeOnSurfaceDestroyed();
    private native void nativeOnTouchEvent(int action, float x, float y, int pointerId);
    private native void nativeOnKeyEvent(int keyCode, int action);
    private native void nativeOnGamepadConnected(int deviceId);
    private native void nativeOnGamepadDisconnected(int deviceId);
    private native void nativeOnPause();
    private native void nativeOnResume();
    private native void nativeOnLowMemory();

    // UDP Networking native methods
    private native long nativeCreateUDPNetworking();
    private native long nativeCreateUDPConnection();
    private native boolean nativeUDPConnect(long connectionPtr, String address, int port);
    private native void nativeUDPDisconnect(long connectionPtr);
    private native boolean nativeUDPSendPacket(long connectionPtr, byte[] data, boolean reliable);
    private native byte[] nativeUDPReceivePacket(long connectionPtr);
    private native boolean nativeUDPIsConnected(long connectionPtr);
    private native String nativeGetUDPStatistics();
    private native long nativeCreateUDPServerSocket(int port);

    // UI components
    private SurfaceView surfaceView;
    private SurfaceHolder surfaceHolder;

    // Game state
    private boolean gameInitialized = false;
    private boolean surfaceCreated = false;

    // Required permissions
    private static final String[] REQUIRED_PERMISSIONS = {
        android.Manifest.permission.WRITE_EXTERNAL_STORAGE,
        android.Manifest.permission.READ_EXTERNAL_STORAGE,
        android.Manifest.permission.INTERNET,
        android.Manifest.permission.ACCESS_NETWORK_STATE,
        android.Manifest.permission.VIBRATE
    };

    static {
        try {
            // Load native library with full path for security
            System.load(System.getProperty("java.library.path") + "/libfoundryengine.so");
        } catch (UnsatisfiedLinkError e) {
            // Fallback to loadLibrary if full path fails
            System.loadLibrary("foundryengine");
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        Log.d(TAG, "onCreate called");

        // Set up full screen
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        getWindow().setFlags(
            WindowManager.LayoutParams.FLAG_FULLSCREEN,
            WindowManager.LayoutParams.FLAG_FULLSCREEN
        );
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        // Force landscape orientation
        setRequestedOrientation(ActivityInfo.SCREEN_INFO_DEFAULT);

        // Request permissions
        requestPermissions();

        // Create surface view
        surfaceView = new SurfaceView(this);
        surfaceHolder = surfaceView.getHolder();
        surfaceHolder.addCallback(this);

        setContentView(surfaceView);

        // Initialize native engine
        nativeOnCreate();
        gameInitialized = true;

        Log.d(TAG, "GameActivity created successfully");
    }

    @Override
    protected void onDestroy() {
        Log.d(TAG, "onDestroy called");

        if (gameInitialized) {
            try {
                nativeOnDestroy();
            } catch (Exception e) {
                Log.e(TAG, "Error in nativeOnDestroy", e);
            }
            gameInitialized = false;
        }

        super.onDestroy();
    }

    @Override
    protected void onPause() {
        Log.d(TAG, "onPause called");

        if (gameInitialized) {
            try {
                nativeOnPause();
            } catch (Exception e) {
                Log.e(TAG, "Error in nativeOnPause", e);
            }
        }

        super.onPause();
    }

    @Override
    protected void onResume() {
        Log.d(TAG, "onResume called");

        super.onResume();

        if (gameInitialized) {
            nativeOnResume();
        }
    }

    @Override
    public void onLowMemory() {
        Log.w(TAG, "onLowMemory called");

        if (gameInitialized) {
            nativeOnLowMemory();
        }

        super.onLowMemory();
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        Log.d(TAG, "surfaceCreated called");

        surfaceCreated = true;
        Surface surface = holder.getSurface();

        if (gameInitialized && surface != null && surface.isValid()) {
            nativeOnSurfaceCreated(surface);
        }
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        Log.d(TAG, "surfaceChanged: " + width + "x" + height);

        if (gameInitialized && surfaceCreated) {
            nativeOnSurfaceChanged(width, height);
        }
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        Log.d(TAG, "surfaceDestroyed called");

        surfaceCreated = false;

        if (gameInitialized) {
            nativeOnSurfaceDestroyed();
        }
    }

    // Touch event handling
    @Override
    public boolean onTouchEvent(android.view.MotionEvent event) {
        if (!gameInitialized) return false;

        int action = event.getActionMasked();
        int pointerIndex = event.getActionIndex();
        int pointerId = event.getPointerId(pointerIndex);

        float x = event.getX(pointerIndex);
        float y = event.getY(pointerIndex);

        nativeOnTouchEvent(action, x, y, pointerId);

        // Convert Android touch events to native format
        switch (action) {
            case android.view.MotionEvent.ACTION_DOWN:
            case android.view.MotionEvent.ACTION_POINTER_DOWN:
                // Touch started
                break;
            case android.view.MotionEvent.ACTION_MOVE:
                // Touch moved
                break;
            case android.view.MotionEvent.ACTION_UP:
            case android.view.MotionEvent.ACTION_POINTER_UP:
                // Touch ended
                break;
            case android.view.MotionEvent.ACTION_CANCEL:
                // Touch cancelled
                break;
            default:
                break;
        }

        return true;
    }

    // Key event handling
    @Override
    public boolean onKeyDown(int keyCode, android.view.KeyEvent event) {
        if (gameInitialized) {
            nativeOnKeyEvent(keyCode, 0); // ACTION_DOWN
        }
        return super.onKeyDown(keyCode, event);
    }

    @Override
    public boolean onKeyUp(int keyCode, android.view.KeyEvent event) {
        if (gameInitialized) {
            nativeOnKeyEvent(keyCode, 1); // ACTION_UP
        }
        return super.onKeyUp(keyCode, event);
    }

    // Permission handling
    private void requestPermissions() {
        boolean allGranted = true;

        for (String permission : REQUIRED_PERMISSIONS) {
            if (ContextCompat.checkSelfPermission(this, permission)
                != PackageManager.PERMISSION_GRANTED) {
                allGranted = false;
                break;
            }
        }

        if (!allGranted) {
            ActivityCompat.requestPermissions(
                this,
                REQUIRED_PERMISSIONS,
                PERMISSION_REQUEST_CODE
            );
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);

        if (requestCode == PERMISSION_REQUEST_CODE) {
            // Validate arrays are not null and have matching lengths
            if (permissions == null || grantResults == null || permissions.length != grantResults.length) {
                Log.e(TAG, "Invalid permission result arrays");
                return;
            }
            
            boolean allGranted = true;

            for (int result : grantResults) {
                if (result != PackageManager.PERMISSION_GRANTED) {
                    allGranted = false;
                    break;
                }
            }

            if (!allGranted) {
                Toast.makeText(this, "Some permissions were denied. Game may not function properly.",
                    Toast.LENGTH_LONG).show();
            }
        }
    }

    // Asset management
    public AssetManager getAssetManager() {
        return getAssets();
    }

    // File utilities for native code
    public static byte[] readFile(String path) {
        // Validate path to prevent directory traversal
        if (path == null || path.contains("..") || path.contains("//")) {
            Log.e(TAG, "Invalid file path: " + path);
            return new byte[0];
        }
        
        try {
            java.io.File file = new java.io.File(path);
            String canonicalPath = file.getCanonicalPath();
            
            // Ensure the canonical path is within allowed directories
            if (!canonicalPath.startsWith("/data/data/") && !canonicalPath.startsWith("/sdcard/")) {
                Log.e(TAG, "Path outside allowed directories: " + canonicalPath);
                return new byte[0];
            }
            
            java.io.InputStream inputStream = new java.io.FileInputStream(canonicalPath);
            java.io.ByteArrayOutputStream outputStream = new java.io.ByteArrayOutputStream();

            byte[] buffer = new byte[1024];
            int length;
            while ((length = inputStream.read(buffer)) != -1) {
                outputStream.write(buffer, 0, length);
            }

            inputStream.close();
            return outputStream.toByteArray();
        } catch (java.io.IOException e) {
            Log.e(TAG, "Failed to read file: " + path, e);
            return new byte[0];
        }
    }

    public static void writeFile(String path, byte[] data) {
        try {
            java.io.FileOutputStream outputStream = new java.io.FileOutputStream(path);
            outputStream.write(data);
            outputStream.close();
        } catch (java.io.IOException e) {
            Log.e(TAG, "Failed to write file: " + path, e);
        }
    }

    public static void deleteFile(String path) {
        java.io.File file = new java.io.File(path);
        if (file.exists()) {
            file.delete();
        }
    }

    public static String[] listFiles(String directory) {
        java.io.File dir = new java.io.File(directory);
        if (dir.exists() && dir.isDirectory()) {
            return dir.list();
        }
        return new String[0];
    }

    public static void createDirectory(String path) {
        java.io.File dir = new java.io.File(path);
        if (!dir.exists()) {
            dir.mkdirs();
        }
    }

    public static boolean exists(String path) {
        return new java.io.File(path).exists();
    }

    // JNI interface for native code to call Java methods
    private void onGamepadConnected(int deviceId) {
        nativeOnGamepadConnected(deviceId);
    }

    private void onGamepadDisconnected(int deviceId) {
        nativeOnGamepadDisconnected(deviceId);
    }

    // Error handling
    private void showError(String message) {
        runOnUiThread(() -> {
            Toast.makeText(this, message, Toast.LENGTH_LONG).show();
        });
    }

    // Performance monitoring
    public void logPerformance(String tag, String message) {
        Log.d("Performance_" + tag, message);
    }

    // Memory management
    public void onTrimMemory(int level) {
        super.onTrimMemory(level);

        switch (level) {
            case TRIM_MEMORY_RUNNING_MODERATE:
                Log.w(TAG, "Memory running moderate");
                break;
            case TRIM_MEMORY_RUNNING_LOW:
                Log.w(TAG, "Memory running low");
                break;
            case TRIM_MEMORY_RUNNING_CRITICAL:
                Log.e(TAG, "Memory running critical");
                break;
            case TRIM_MEMORY_BACKGROUND:
                Log.i(TAG, "Memory background");
                break;
            case TRIM_MEMORY_MODERATE:
                Log.i(TAG, "Memory moderate");
                break;
            case TRIM_MEMORY_COMPLETE:
                Log.i(TAG, "Memory complete");
                break;
            default:
                Log.w(TAG, "Unknown memory trim level: " + level);
                break;
        }

        if (gameInitialized) {
            nativeOnLowMemory();
        }
    }
}
