#pragma once
#include "Includes/obfuscate.h"
#include "json.hpp"
#include "ImGui/imgui.h"
#include <curl/curl.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/md5.h>
#include <jni.h>
#include <string>
#include <sstream>
#include <ctime>
#include <cstdio>
#include <string>
#include <functional>
#include <cstring>
#include <limits>
#include <cstdint>
#include "Includes/EthnirAuthReply.h"
#include "Includes/AndroidCurlTrust.h"

extern JavaVM* VM;

using json = nlohmann::json;
std::string g_Token, g_Auth;
bool bValid = false;
std::string EXP = " ";
std::string mod_status = " ";
std::string DeviceID;
std::string DeviceModel = " ";
std::string DeviceBrand; 

// Copy Java UTF strings before releasing JNI references. Keep the same
// key+AndroidID+MODEL+BRAND fingerprint used by the original panel.
static std::string ShaiCopyJString(JNIEnv* env, jstring value) {
    if (!env || !value || env->ExceptionCheck()) return {};
    const char* chars = env->GetStringUTFChars(value, nullptr);
    if (!chars) return {};
    const std::string copy(chars);
    env->ReleaseStringUTFChars(value, chars);
    return copy;
}

std::string GetAndroidID(JNIEnv* env, jobject context) {
    if (!env || !context) return {};
    jclass ctxClass = env->FindClass("android/content/Context");
    jclass secureClass = env->FindClass("android/provider/Settings$Secure");
    std::string id;
    if (ctxClass && secureClass && !env->ExceptionCheck()) {
        jmethodID getResolver = env->GetMethodID(ctxClass, "getContentResolver", "()Landroid/content/ContentResolver;");
        jmethodID getString = env->GetStaticMethodID(secureClass, "getString", "(Landroid/content/ContentResolver;Ljava/lang/String;)Ljava/lang/String;");
        if (getResolver && getString && !env->ExceptionCheck()) {
            jobject resolver = env->CallObjectMethod(context, getResolver);
            jstring key = env->NewStringUTF("android_id");
            if (resolver && key && !env->ExceptionCheck()) {
                jstring value = static_cast<jstring>(env->CallStaticObjectMethod(secureClass, getString, resolver, key));
                id = ShaiCopyJString(env, value);
                if (value) env->DeleteLocalRef(value);
            }
            if (key) env->DeleteLocalRef(key);
            if (resolver) env->DeleteLocalRef(resolver);
        }
    }
    if (secureClass) env->DeleteLocalRef(secureClass);
    if (ctxClass) env->DeleteLocalRef(ctxClass);
    return id;
}

static std::string GetBuildText(JNIEnv* env, const char* fieldName) {
    if (!env) return {};
    jclass cls = env->FindClass("android/os/Build");
    std::string text;
    if (cls && !env->ExceptionCheck()) {
        jfieldID field = env->GetStaticFieldID(cls, fieldName, "Ljava/lang/String;");
        if (field && !env->ExceptionCheck()) {
            jstring value = static_cast<jstring>(env->GetStaticObjectField(cls, field));
            text = ShaiCopyJString(env, value);
            if (value) env->DeleteLocalRef(value);
        }
    }
    if (cls) env->DeleteLocalRef(cls);
    return text;
}
std::string GetDeviceModel(JNIEnv *env) { return GetBuildText(env, "MODEL"); }
std::string GetDeviceBrand(JNIEnv *env) { return GetBuildText(env, "BRAND"); }

std::string GetDeviceUniqueIdentifier(JNIEnv* env, const char* fingerprint) {
    if (!env || !fingerprint) return {};
    jclass cls = env->FindClass("java/util/UUID");
    std::string uuid;
    if (cls && !env->ExceptionCheck()) {
        jmethodID fromBytes = env->GetStaticMethodID(cls, "nameUUIDFromBytes", "([B)Ljava/util/UUID;");
        jmethodID asString = env->GetMethodID(cls, "toString", "()Ljava/lang/String;");
        if (fromBytes && asString && !env->ExceptionCheck()) {
            const size_t len = std::strlen(fingerprint);
            if (len <= static_cast<size_t>(std::numeric_limits<jsize>::max())) {
                jbyteArray arr = env->NewByteArray(static_cast<jsize>(len));
                if (arr && !env->ExceptionCheck()) {
                    env->SetByteArrayRegion(arr, 0, static_cast<jsize>(len), reinterpret_cast<const jbyte*>(fingerprint));
                    jobject obj = env->CallStaticObjectMethod(cls, fromBytes, arr);
                    if (obj && !env->ExceptionCheck()) {
                        jstring value = static_cast<jstring>(env->CallObjectMethod(obj, asString));
                        uuid = ShaiCopyJString(env, value);
                        if (value) env->DeleteLocalRef(value);
                    }
                    if (obj) env->DeleteLocalRef(obj);
                }
                if (arr) env->DeleteLocalRef(arr);
            }
        }
    }
    if (cls) env->DeleteLocalRef(cls);
    return uuid;
}

std::string CalcMD5(std::string s) {
    std::string result;
    unsigned char hash[MD5_DIGEST_LENGTH];
    char tmp[4];
    MD5_CTX md5;
    MD5_Init(&md5);
    MD5_Update(&md5, s.c_str(), s.length());
    MD5_Final(hash, &md5);
    for (unsigned char i : hash) {
        sprintf(tmp, "%02x", i);
        result += tmp;
    }
    return result;
}

struct MemoryStruct {
	char *memory;
	size_t size;
};

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    struct MemoryStruct* mem = static_cast<MemoryStruct*>(userp);
    if (size != 0 && nmemb > SIZE_MAX / size) return 0;
    const size_t bytes = size * nmemb;
    constexpr size_t kMaxResponseBytes = 64 * 1024;
    if (!mem || mem->size > kMaxResponseBytes || bytes > kMaxResponseBytes - mem->size) return 0;
    char* resized = static_cast<char*>(realloc(mem->memory, mem->size + bytes + 1));
    if (!resized) return 0;
    mem->memory = resized;
    memcpy(mem->memory + mem->size, contents, bytes);
    mem->size += bytes;
    mem->memory[mem->size] = 0;
    return bytes;
}

int ShowSoftKeyboardInput() {
	jint result;
	jint flags = 0;
	
	JNIEnv *env;
	VM->AttachCurrentThread(&env, NULL);
	
	jclass looperClass = env->FindClass(OBFUSCATE("android/os/Looper"));
	auto prepareMethod = env->GetStaticMethodID(looperClass, OBFUSCATE("prepare"), OBFUSCATE("()V"));
	env->CallStaticVoidMethod(looperClass, prepareMethod);
	
	jclass activityThreadClass = env->FindClass(OBFUSCATE("android/app/ActivityThread"));
	jfieldID sCurrentActivityThreadField = env->GetStaticFieldID(activityThreadClass, OBFUSCATE("sCurrentActivityThread"), OBFUSCATE("Landroid/app/ActivityThread;"));
	jobject sCurrentActivityThread = env->GetStaticObjectField(activityThreadClass, sCurrentActivityThreadField);
	
	jfieldID mInitialApplicationField = env->GetFieldID(activityThreadClass, OBFUSCATE("mInitialApplication"), OBFUSCATE("Landroid/app/Application;"));
	jobject mInitialApplication = env->GetObjectField(sCurrentActivityThread, mInitialApplicationField);
	
	jclass contextClass = env->FindClass(OBFUSCATE("android/content/Context"));
	jfieldID fieldINPUT_METHOD_SERVICE = env->GetStaticFieldID(contextClass, OBFUSCATE("INPUT_METHOD_SERVICE"), OBFUSCATE("Ljava/lang/String;"));
	jobject INPUT_METHOD_SERVICE = env->GetStaticObjectField(contextClass, fieldINPUT_METHOD_SERVICE);
	jmethodID getSystemServiceMethod = env->GetMethodID(contextClass, OBFUSCATE("getSystemService"), OBFUSCATE("(Ljava/lang/String;)Ljava/lang/Object;"));
	jobject callObjectMethod = env->CallObjectMethod(mInitialApplication, getSystemServiceMethod, INPUT_METHOD_SERVICE);
	
	jclass classInputMethodManager = env->FindClass(OBFUSCATE("android/view/inputmethod/InputMethodManager"));
    jmethodID toggleSoftInputId = env->GetMethodID(classInputMethodManager, OBFUSCATE("toggleSoftInput"), OBFUSCATE("(II)V"));
	
	if (result) {
		env->CallVoidMethod(callObjectMethod, toggleSoftInputId, 2, flags);
	} else {
		env->CallVoidMethod(callObjectMethod, toggleSoftInputId, flags, flags);
	}
	
	env->DeleteLocalRef(classInputMethodManager);
	env->DeleteLocalRef(callObjectMethod);
	env->DeleteLocalRef(contextClass);
    env->DeleteLocalRef(mInitialApplication);
    env->DeleteLocalRef(activityThreadClass);
	VM->DetachCurrentThread();
	
	return result;
}

int PollUnicodeChars() {
	JNIEnv *env;
	VM->AttachCurrentThread(&env, NULL);
	
	jclass looperClass = env->FindClass(OBFUSCATE("android/os/Looper"));
	auto prepareMethod = env->GetStaticMethodID(looperClass, OBFUSCATE("prepare"), OBFUSCATE("()V"));
	env->CallStaticVoidMethod(looperClass, prepareMethod);
	
	jclass activityThreadClass = env->FindClass(OBFUSCATE("android/app/ActivityThread"));
	jfieldID sCurrentActivityThreadField = env->GetStaticFieldID(activityThreadClass, OBFUSCATE("sCurrentActivityThread"), OBFUSCATE("Landroid/app/ActivityThread;"));
	jobject sCurrentActivityThread = env->GetStaticObjectField(activityThreadClass, sCurrentActivityThreadField);
	
	jfieldID mInitialApplicationField = env->GetFieldID(activityThreadClass, OBFUSCATE("mInitialApplication"), OBFUSCATE("Landroid/app/Application;"));
	jobject mInitialApplication = env->GetObjectField(sCurrentActivityThread, mInitialApplicationField);
	
	jclass keyEventClass = env->FindClass(OBFUSCATE("android/view/KeyEvent"));
	jmethodID getUnicodeCharMethod = env->GetMethodID(keyEventClass, OBFUSCATE("getUnicodeChar"), OBFUSCATE("(I)I"));
	
	ImGuiIO& io = ImGui::GetIO();
    
	int return_key = env->CallIntMethod(keyEventClass, getUnicodeCharMethod);
	
	env->DeleteLocalRef(keyEventClass);
	env->DeleteLocalRef(mInitialApplication);
    env->DeleteLocalRef(activityThreadClass);
	VM->DetachCurrentThread();
	
	return return_key;
}


std::string getClipboard() {


    std::string result;
    JNIEnv *env;

    VM->AttachCurrentThread(&env, NULL);

    auto looperClass = env->FindClass(OBFUSCATE("android/os/Looper"));
    auto prepareMethod = env->GetStaticMethodID(looperClass, OBFUSCATE("prepare"),
                         OBFUSCATE("()V"));
    env->CallStaticVoidMethod(looperClass, prepareMethod);

    jclass activityThreadClass = env->FindClass(OBFUSCATE("android/app/ActivityThread"));
    jfieldID sCurrentActivityThreadField = env->GetStaticFieldID(activityThreadClass, OBFUSCATE(
            "sCurrentActivityThread"), OBFUSCATE("Landroid/app/ActivityThread;"));
    jobject sCurrentActivityThread = env->GetStaticObjectField(activityThreadClass,
                                     sCurrentActivityThreadField);

    jfieldID mInitialApplicationField = env->GetFieldID(activityThreadClass,
                                        OBFUSCATE("mInitialApplication"),
                                        OBFUSCATE("Landroid/app/Application;"));
    jobject mInitialApplication = env->GetObjectField(sCurrentActivityThread,
                                  mInitialApplicationField);

    auto contextClass = env->FindClass(OBFUSCATE("android/content/Context"));
    auto getSystemServiceMethod = env->GetMethodID(contextClass, OBFUSCATE("getSystemService"),
                                  OBFUSCATE(
                                      "(Ljava/lang/String;)Ljava/lang/Object;"));

    auto str = env->NewStringUTF(OBFUSCATE("clipboard"));
    auto clipboardManager = env->CallObjectMethod(mInitialApplication, getSystemServiceMethod, str);
    env->DeleteLocalRef(str);

    jclass ClipboardManagerClass = env->FindClass(OBFUSCATE("android/content/ClipboardManager"));
    auto getText = env->GetMethodID(ClipboardManagerClass, OBFUSCATE("getText"),
                                    OBFUSCATE("()Ljava/lang/CharSequence;"));

    jclass CharSequenceClass = env->FindClass(OBFUSCATE("java/lang/CharSequence"));
    auto toStringMethod = env->GetMethodID(CharSequenceClass, OBFUSCATE("toString"),
                                           OBFUSCATE("()Ljava/lang/String;"));

    auto text = env->CallObjectMethod(clipboardManager, getText);
    if (text) {
        str = (jstring) env->CallObjectMethod(text, toStringMethod);
        result = env->GetStringUTFChars(str, 0);
        env->DeleteLocalRef(str);
        env->DeleteLocalRef(text);
    }

    env->DeleteLocalRef(CharSequenceClass);
    env->DeleteLocalRef(ClipboardManagerClass);
    env->DeleteLocalRef(clipboardManager);
    env->DeleteLocalRef(contextClass);
    env->DeleteLocalRef(mInitialApplication);
    env->DeleteLocalRef(activityThreadClass);
    VM->DetachCurrentThread();
    return result.c_str();
}

// Same signed-license protocol as IMGUI3: POST game, user_key, serial;
// accept a session only when server payload and signature verify.
std::string Login(const char* user_key) {
    bValid = false;
    g_Token.clear();
    g_Auth.clear();
    if (!user_key || !*user_key) return "Enter your license key.";
    if (!VM) return "Android runtime is unavailable.";

    JNIEnv* env = nullptr;
    bool attachedHere = false;
    const jint state = VM->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6);
    if (state == JNI_EDETACHED) {
        if (VM->AttachCurrentThread(&env, nullptr) != JNI_OK || !env)
            return "Unable to access Android device information.";
        attachedHere = true;
    } else if (state != JNI_OK || !env) {
        return "Unable to access Android device information.";
    }

    // Use the application's own Android ID rather than a different app's ID.
    std::string serial;
    jclass activityThread = env->FindClass("android/app/ActivityThread");
    jobject app = nullptr;
    if (activityThread && !env->ExceptionCheck()) {
        jmethodID currentApp = env->GetStaticMethodID(activityThread, "currentApplication", "()Landroid/app/Application;");
        if (currentApp && !env->ExceptionCheck())
            app = env->CallStaticObjectMethod(activityThread, currentApp);
        if (env->ExceptionCheck()) env->ExceptionClear();
        if (!app) {
            jfieldID currentThreadField = env->GetStaticFieldID(activityThread, "sCurrentActivityThread", "Landroid/app/ActivityThread;");
            if (currentThreadField && !env->ExceptionCheck()) {
                jobject thread = env->GetStaticObjectField(activityThread, currentThreadField);
                if (thread && !env->ExceptionCheck()) {
                    jfieldID appField = env->GetFieldID(activityThread, "mInitialApplication", "Landroid/app/Application;");
                    if (appField && !env->ExceptionCheck()) app = env->GetObjectField(thread, appField);
                }
                if (thread) env->DeleteLocalRef(thread);
            }
            if (env->ExceptionCheck()) env->ExceptionClear();
        }
    }
    if (env->ExceptionCheck()) env->ExceptionClear();
    if (app) {
        const std::string androidId = GetAndroidID(env, app);
        const std::string model = GetDeviceModel(env);
        const std::string brand = GetDeviceBrand(env);
        if (!env->ExceptionCheck() && !androidId.empty() && !model.empty() && !brand.empty()) {
            DeviceID = androidId;
            DeviceModel = model;
            DeviceBrand = brand;
            const std::string fingerprint = std::string(user_key) + androidId + model + brand;
            serial = GetDeviceUniqueIdentifier(env, fingerprint.c_str());
        }
        env->DeleteLocalRef(app);
    }
    if (activityThread) env->DeleteLocalRef(activityThread);
    if (env->ExceptionCheck()) env->ExceptionClear();
    if (attachedHere) VM->DetachCurrentThread();
    if (serial.empty()) return "Device ID unavailable. Open the game before signing in.";

    CURL* curl = curl_easy_init();
    if (!curl) return "Could not initialize the license connection.";
    char* escapedKey = curl_easy_escape(curl, user_key, 0);
    char* escapedSerial = curl_easy_escape(curl, serial.c_str(), 0);
    if (!escapedKey || !escapedSerial) {
        if (escapedKey) curl_free(escapedKey);
        if (escapedSerial) curl_free(escapedSerial);
        curl_easy_cleanup(curl);
        return "Could not encode the license request.";
    }
    const std::string post = std::string("game=") + imgui3_license::kGame +
        "&user_key=" + escapedKey + "&serial=" + escapedSerial;
    curl_free(escapedKey);
    curl_free(escapedSerial);

    MemoryStruct response{};
    response.memory = static_cast<char*>(calloc(1, 1));
    if (!response.memory) {
        curl_easy_cleanup(curl);
        return "Not enough memory for the license request.";
    }
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded");
    headers = curl_slist_append(headers, "Accept: application/json");
    char networkError[CURL_ERROR_SIZE] = {};
    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, networkError);
    curl_easy_setopt(curl, CURLOPT_URL, imgui3_license::kEndpoint);
    // Do not forward a license key to an unexpected redirect destination.
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 0L);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(post.size()));
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 20L);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
#if defined(__ANDROID__)
    curl_easy_setopt(curl, CURLOPT_SSL_CTX_FUNCTION, ethnir_tls::InstallAndroidTrust);
#endif

    const CURLcode error = curl_easy_perform(curl);
    long status = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
    std::string message;
    if (error != CURLE_OK) {
        // Keep the numeric CURLcode, HTTP status and libcurl's diagnostic;
        // never print the URL query, license key or device identifier.
        const char* reason = networkError[0] ? networkError : curl_easy_strerror(error);
        if (error == CURLE_PEER_FAILED_VERIFICATION || error == CURLE_SSL_CACERT ||
            error == CURLE_SSL_CERTPROBLEM) {
            message = "SSL verification failed (curl " + std::to_string(static_cast<int>(error)) +
                      "): " + reason;
        } else {
            message = "License connection failed (curl " + std::to_string(static_cast<int>(error)) +
                      ", HTTP " + std::to_string(status) + "): " + reason;
        }
    } else if (status >= 300 && status < 400) {
        message = "License server redirected (HTTP " + std::to_string(status) +
                  "). Check the /connect route.";
    } else if (status != 200) {
        message = "License server returned HTTP " + std::to_string(status) + ".";
    } else {
        const ethnir_auth::Response verdict = ethnir_auth::ParseSignedReply(
            std::string(response.memory, response.size), user_key, serial);
        if (verdict.accepted) {
            bValid = true;
            g_Token = verdict.payload;
            std::tm when{};
            localtime_r(&verdict.expires, &when);
            char date[32] = {};
            strftime(date, sizeof(date), "%Y-%m-%d", &when);
            EXP = date;
        } else {
            message = verdict.error;
        }
    }
    if (headers) curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    free(response.memory);
    return bValid ? "OK" : (message.empty() ? "Login failed." : message);
}
