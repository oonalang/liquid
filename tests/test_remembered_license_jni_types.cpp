#ifndef __ANDROID__
#define __ANDROID__ 1
#endif
#include "../src/main/jni/UI/RememberedLicense.h"
#include <cassert>

struct AndroidJNIStyleVM {
    bool called = false;
    jint AttachCurrentThread(JNIEnv** env, void*) {
        called = true;
        *env = reinterpret_cast<JNIEnv*>(0x1);
        return JNI_OK;
    }
};
struct HostJNIStyleVM {
    bool called = false;
    jint AttachCurrentThread(void** env, void*) {
        called = true;
        *env = reinterpret_cast<void*>(0x1);
        return JNI_OK;
    }
};
int main() {
    AndroidJNIStyleVM android;
    HostJNIStyleVM host;
    JNIEnv* env = nullptr;
    assert(RememberedLicense::AttachThread(&android, &env, 0) == JNI_OK);
    assert(android.called && env);
    env = nullptr;
    assert(RememberedLicense::AttachThread(&host, &env, 0) == JNI_OK);
    assert(host.called && env);
}
