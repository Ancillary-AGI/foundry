/**
 * Android Input Platform Implementation
 * Touch, sensor, and controller input handling for Android
 */

#include "AndroidPlatform.h"
#include <android/input.h>
#include <android/sensor.h>
#include <android/keycodes.h>
#include <jni.h>
#include <vector>
#include <unordered_map>
#include <memory>

// Input implementation for Android
class AndroidInput {
private:
    ASensorManager* sensorManager = nullptr;
    ASensorEventQueue* sensorEventQueue = nullptr;
    const ASensor* accelerometerSensor = nullptr;
    const ASensor* gyroscopeSensor = nullptr;

    std::unordered_map<int, bool> keyStates_;
    std::unordered_map<int, bool> touchStates_;
    std::vector<AInputEvent*> pendingEvents_;

    struct TouchPoint {
        int id;
        float x, y;
        float pressure;
        bool active;
    };

    std::unordered_map<int, TouchPoint> touchPoints_;
    std::vector<float> accelerometerData_ = {0, 0, 0};
    std::vector<float> gyroscopeData_ = {0, 0, 0};

public:
    AndroidInput() = default;
    ~AndroidInput() { shutdown(); }

    bool initialize() {
        // Initialize sensor manager
        sensorManager = ASensorManager_getInstance();
        if (!sensorManager) {
            return false;
        }

        // Get sensors
        accelerometerSensor = ASensorManager_getDefaultSensor(sensorManager, ASENSOR_TYPE_ACCELEROMETER);
        gyroscopeSensor = ASensorManager_getDefaultSensor(sensorManager, ASENSOR_TYPE_GYROSCOPE);

        // Create sensor event queue
        ALooper* looper = ALooper_prepare(ALOOPER_PREPARE_ALLOW_NON_CALLBACKS);
        sensorEventQueue = ASensorManager_createEventQueue(sensorManager, looper, 0, nullptr, nullptr);

        if (sensorEventQueue) {
            // Enable sensors
            if (accelerometerSensor) {
                ASensorEventQueue_enableSensor(sensorEventQueue, accelerometerSensor);
                ASensorEventQueue_setEventRate(sensorEventQueue, accelerometerSensor, 100000); // 10ms
            }

            if (gyroscopeSensor) {
                ASensorEventQueue_enableSensor(sensorEventQueue, gyroscopeSensor);
                ASensorEventQueue_setEventRate(sensorEventQueue, gyroscopeSensor, 100000); // 10ms
            }
        }

        return true;
    }

    void shutdown() {
        if (sensorEventQueue) {
            if (accelerometerSensor) {
                ASensorEventQueue_disableSensor(sensorEventQueue, accelerometerSensor);
            }
            if (gyroscopeSensor) {
                ASensorEventQueue_disableSensor(sensorEventQueue, gyroscopeSensor);
            }
            ASensorManager_destroyEventQueue(sensorManager, sensorEventQueue);
            sensorEventQueue = nullptr;
        }
    }

    void update() {
        // Process sensor events
        if (sensorEventQueue) {
            ASensorEvent event;
            while (ASensorEventQueue_getEvents(sensorEventQueue, &event, 1) > 0) {
                switch (event.type) {
                case ASENSOR_TYPE_ACCELEROMETER:
                    accelerometerData_[0] = event.acceleration.x;
                    accelerometerData_[1] = event.acceleration.y;
                    accelerometerData_[2] = event.acceleration.z;
                    break;

                case ASENSOR_TYPE_GYROSCOPE:
                    gyroscopeData_[0] = event.vector.x;
                    gyroscopeData_[1] = event.vector.y;
                    gyroscopeData_[2] = event.vector.z;
                    break;
                }
            }
        }
    }

    // Touch input handling
    void handleTouchEvent(AInputEvent* event) {
        int action = AMotionEvent_getAction(event);
        int pointerIndex = (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
        int pointerId = AMotionEvent_getPointerId(event, pointerIndex);

        switch (action & AMOTION_EVENT_ACTION_MASK) {
        case AMOTION_EVENT_ACTION_DOWN:
        case AMOTION_EVENT_ACTION_POINTER_DOWN: {
            TouchPoint touch;
            touch.id = pointerId;
            touch.x = AMotionEvent_getX(event, pointerIndex);
            touch.y = AMotionEvent_getY(event, pointerIndex);
            touch.pressure = AMotionEvent_getPressure(event, pointerIndex);
            touch.active = true;
            touchPoints_[pointerId] = touch;
            break;
        }

        case AMOTION_EVENT_ACTION_MOVE: {
            int pointerCount = AMotionEvent_getPointerCount(event);
            for (int i = 0; i < pointerCount; i++) {
                int id = AMotionEvent_getPointerId(event, i);
                if (touchPoints_.count(id)) {
                    touchPoints_[id].x = AMotionEvent_getX(event, i);
                    touchPoints_[id].y = AMotionEvent_getY(event, i);
                    touchPoints_[id].pressure = AMotionEvent_getPressure(event, i);
                }
            }
            break;
        }

        case AMOTION_EVENT_ACTION_UP:
        case AMOTION_EVENT_ACTION_POINTER_UP: {
            if (touchPoints_.count(pointerId)) {
                touchPoints_[pointerId].active = false;
            }
            break;
        }

        case AMOTION_EVENT_ACTION_CANCEL: {
            // Cancel all touches
            for (auto& pair : touchPoints_) {
                pair.second.active = false;
            }
            break;
        }
        }
    }

    // Key input handling
    void handleKeyEvent(AInputEvent* event) {
        int keyCode = AKeyEvent_getKeyCode(event);
        int action = AKeyEvent_getAction(event);

        keyStates_[keyCode] = (action == AKEY_EVENT_ACTION_DOWN);
    }

    // Getters
    const std::unordered_map<int, TouchPoint>& getTouchPoints() const {
        return touchPoints_;
    }

    const std::vector<float>& getAccelerometerData() const {
        return accelerometerData_;
    }

    const std::vector<float>& getGyroscopeData() const {
        return gyroscopeData_;
    }

    const std::unordered_map<int, bool>& getKeyStates() const {
        return keyStates_;
    }

    bool isKeyPressed(int keyCode) const {
        auto it = keyStates_.find(keyCode);
        return it != keyStates_.end() && it->second;
    }

    bool isTouchActive(int touchId) const {
        auto it = touchPoints_.find(touchId);
        return it != touchPoints_.end() && it->second.active;
    }
};

// Input API functions that can be called from Java
extern "C" {
    JNIEXPORT jlong JNICALL Java_com_foundryengine_game_GameActivity_nativeCreateInput(JNIEnv* env, jobject thiz) {
        AndroidInput* input = new AndroidInput();
        if (input->initialize()) {
            return reinterpret_cast<jlong>(input);
        } else {
            delete input;
            return 0;
        }
    }

    JNIEXPORT void JNICALL Java_com_foundryengine_game_GameActivity_nativeDestroyInput(JNIEnv* env, jobject thiz, jlong inputPtr) {
        AndroidInput* input = reinterpret_cast<AndroidInput*>(inputPtr);
        if (input) {
            delete input;
        }
    }

    JNIEXPORT void JNICALL Java_com_foundryengine_game_GameActivity_nativeUpdateInput(JNIEnv* env, jobject thiz, jlong inputPtr) {
        AndroidInput* input = reinterpret_cast<AndroidInput*>(inputPtr);
        if (input) {
            input->update();
        }
    }

    JNIEXPORT void JNICALL Java_com_foundryengine_game_GameActivity_nativeHandleTouchEvent(JNIEnv* env, jobject thiz, jlong inputPtr, jobject motionEvent) {
        AndroidInput* input = reinterpret_cast<AndroidInput*>(inputPtr);
        if (input && motionEvent) {
            // Convert Java MotionEvent to native AInputEvent
            // This is a simplified implementation
            AInputEvent* event = nullptr; // Would need proper conversion
            if (event) {
                input->handleTouchEvent(event);
            }
        }
    }

    JNIEXPORT void JNICALL Java_com_foundryengine_game_GameActivity_nativeHandleKeyEvent(JNIEnv* env, jobject thiz, jlong inputPtr, jobject keyEvent) {
        AndroidInput* input = reinterpret_cast<AndroidInput*>(inputPtr);
        if (input && keyEvent) {
            // Convert Java KeyEvent to native AInputEvent
            // This is a simplified implementation
            AInputEvent* event = nullptr; // Would need proper conversion
            if (event) {
                input->handleKeyEvent(event);
            }
        }
    }

    JNIEXPORT jboolean JNICALL Java_com_foundryengine_game_GameActivity_nativeIsKeyPressed(JNIEnv* env, jobject thiz, jlong inputPtr, jint keyCode) {
        AndroidInput* input = reinterpret_cast<AndroidInput*>(inputPtr);
        return input && input->isKeyPressed(keyCode);
    }

    JNIEXPORT jboolean JNICALL Java_com_foundryengine_game_GameActivity_nativeIsTouchActive(JNIEnv* env, jobject thiz, jlong inputPtr, jint touchId) {
        AndroidInput* input = reinterpret_cast<AndroidInput*>(inputPtr);
        return input && input->isTouchActive(touchId);
    }

    JNIEXPORT jfloatArray JNICALL Java_com_foundryengine_game_GameActivity_nativeGetAccelerometerData(JNIEnv* env, jobject thiz, jlong inputPtr) {
        AndroidInput* input = reinterpret_cast<AndroidInput*>(inputPtr);
        if (!input) return nullptr;

        const auto& data = input->getAccelerometerData();
        jfloatArray result = env->NewFloatArray(3);
        env->SetFloatArrayRegion(result, 0, 3, data.data());
        return result;
    }

    JNIEXPORT jfloatArray JNICALL Java_com_foundryengine_game_GameActivity_nativeGetGyroscopeData(JNIEnv* env, jobject thiz, jlong inputPtr) {
        AndroidInput* input = reinterpret_cast<AndroidInput*>(inputPtr);
        if (!input) return nullptr;

        const auto& data = input->getGyroscopeData();
        jfloatArray result = env->NewFloatArray(3);
        env->SetFloatArrayRegion(result, 0, 3, data.data());
        return result;
    }
}
