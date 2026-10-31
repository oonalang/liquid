#pragma once
#include <jni.h>
#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

extern JavaVM* VM;

namespace RememberedLicense {

inline std::vector<uint8_t> Pack(const std::vector<uint8_t>& iv, const std::vector<uint8_t>& encrypted) {
    if (iv.size() != 12 || encrypted.size() < 16 || encrypted.size() > 4096) return {};
    std::vector<uint8_t> payload;
    payload.reserve(1 + iv.size() + encrypted.size());
    payload.push_back(1);
    payload.insert(payload.end(), iv.begin(), iv.end());
    payload.insert(payload.end(), encrypted.begin(), encrypted.end());
    return payload;
}

inline bool Unpack(const std::vector<uint8_t>& payload,
                   std::vector<uint8_t>& iv, std::vector<uint8_t>& encrypted) {
    if (payload.size() < 29 || payload.size() > 4109 || payload[0] != 1) return false;
    iv.assign(payload.begin() + 1, payload.begin() + 13);
    encrypted.assign(payload.begin() + 13, payload.end());
    return true;
}

#if defined(__ANDROID__) && !defined(STUDIO_STANDALONE_PREVIEW)

template <typename VMType>
inline auto AttachThread(VMType* vm, JNIEnv** env, int)
    -> decltype(vm->AttachCurrentThread(env, nullptr)) {
    return vm->AttachCurrentThread(env, nullptr);
}

template <typename VMType>
inline auto AttachThread(VMType* vm, JNIEnv** env, long)
    -> decltype(vm->AttachCurrentThread(reinterpret_cast<void**>(env), nullptr)) {
    return vm->AttachCurrentThread(reinterpret_cast<void**>(env), nullptr);
}

struct Session {
    JNIEnv* env = nullptr;
    bool attached = false;
    bool frame = false;
    Session() {
        if (!::VM) return;
        const jint result = ::VM->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6);
        if (result == JNI_EDETACHED) {
            if (AttachThread(::VM, &env, 0) != JNI_OK) { env = nullptr; return; }
            attached = true;
        } else if (result != JNI_OK) {
            env = nullptr;
        }
        if (env && env->PushLocalFrame(96) == 0) frame = true;
        else env = nullptr;
    }
    ~Session() {
        if (env && frame) {
            if (env->ExceptionCheck()) env->ExceptionClear();
            env->PopLocalFrame(nullptr);
        }
        if (attached && ::VM) ::VM->DetachCurrentThread();
    }
    explicit operator bool() const { return env && frame; }
};

inline bool Good(JNIEnv* env) {
    if (!env || !env->ExceptionCheck()) return env != nullptr;
    env->ExceptionClear();
    return false;
}

inline jstring Text(JNIEnv* env, const char* value) {
    return env->NewStringUTF(value);
}

inline std::string NativeString(JNIEnv* env, jstring value) {
    if (!value || !Good(env)) return {};
    const char* chars = env->GetStringUTFChars(value, nullptr);
    if (!chars || !Good(env)) return {};
    std::string result(chars);
    env->ReleaseStringUTFChars(value, chars);
    return result;
}

inline std::vector<uint8_t> Bytes(JNIEnv* env, jbyteArray value) {
    if (!value || !Good(env)) return {};
    const jsize length = env->GetArrayLength(value);
    if (length <= 0 || length > 4096) return {};
    std::vector<uint8_t> result(static_cast<size_t>(length));
    env->GetByteArrayRegion(value, 0, length, reinterpret_cast<jbyte*>(result.data()));
    if (!Good(env)) return {};
    return result;
}

inline jbyteArray JavaBytes(JNIEnv* env, const uint8_t* bytes, size_t length) {
    if (!bytes || !length || length > 4096) return nullptr;
    jbyteArray value = env->NewByteArray(static_cast<jsize>(length));
    if (!value || !Good(env)) return nullptr;
    env->SetByteArrayRegion(value, 0, static_cast<jsize>(length), reinterpret_cast<const jbyte*>(bytes));
    return Good(env) ? value : nullptr;
}

inline jobject App(JNIEnv* env) {
    jclass cls = env->FindClass("android/app/ActivityThread");
    if (!cls || !Good(env)) return nullptr;
    jmethodID getApp = env->GetStaticMethodID(cls, "currentApplication", "()Landroid/app/Application;");
    if (!getApp || !Good(env)) return nullptr;
    jobject app = env->CallStaticObjectMethod(cls, getApp);
    return Good(env) ? app : nullptr;
}

inline jobject Preferences(JNIEnv* env) {
    jobject app = App(env);
    if (!app) return nullptr;
    jclass cls = env->GetObjectClass(app);
    jmethodID getter = cls ? env->GetMethodID(cls, "getSharedPreferences", "(Ljava/lang/String;I)Landroid/content/SharedPreferences;") : nullptr;
    if (!getter || !Good(env)) return nullptr;
    jobject preferences = env->CallObjectMethod(app, getter, Text(env, "ethnir_noir_license"), 0);
    return Good(env) ? preferences : nullptr;
}

inline std::string ReadPreference(JNIEnv* env, jobject preferences) {
    if (!preferences) return {};
    jclass cls = env->GetObjectClass(preferences);
    jmethodID getter = cls ? env->GetMethodID(cls, "getString", "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;") : nullptr;
    if (!getter || !Good(env)) return {};
    jstring value = static_cast<jstring>(env->CallObjectMethod(preferences, getter,
        Text(env, "remembered_license_v1"), nullptr));
    return Good(env) ? NativeString(env, value) : std::string();
}

inline bool WritePreference(JNIEnv* env, jobject preferences, const std::string* value) {
    if (!preferences) return false;
    jclass cls = env->GetObjectClass(preferences);
    jmethodID edit = cls ? env->GetMethodID(cls, "edit", "()Landroid/content/SharedPreferences$Editor;") : nullptr;
    if (!edit || !Good(env)) return false;
    jobject editor = env->CallObjectMethod(preferences, edit);
    if (!editor || !Good(env)) return false;
    jclass editClass = env->GetObjectClass(editor);
    if (!editClass || !Good(env)) return false;
    jmethodID operation = env->GetMethodID(editClass, value ? "putString" : "remove",
        value ? "(Ljava/lang/String;Ljava/lang/String;)Landroid/content/SharedPreferences$Editor;"
              : "(Ljava/lang/String;)Landroid/content/SharedPreferences$Editor;");
    jmethodID commit = env->GetMethodID(editClass, "commit", "()Z");
    if (!operation || !commit || !Good(env)) return false;
    jstring name = Text(env, "remembered_license_v1");
    if (!name || !Good(env)) return false;
    if (value) {
        jstring data = Text(env, value->c_str());
        if (!data || !Good(env)) return false;
        env->CallObjectMethod(editor, operation, name, data);
    } else {
        env->CallObjectMethod(editor, operation, name);
    }
    if (!Good(env)) return false;
    return env->CallBooleanMethod(editor, commit) == JNI_TRUE && Good(env);
}

inline jobject Key(JNIEnv* env, bool create) {
    jclass storeClass = env->FindClass("java/security/KeyStore");
    if (!storeClass || !Good(env)) return nullptr;
    jmethodID instance = env->GetStaticMethodID(storeClass, "getInstance", "(Ljava/lang/String;)Ljava/security/KeyStore;");
    jmethodID load = env->GetMethodID(storeClass, "load", "(Ljava/io/InputStream;[C)V");
    jmethodID getKey = env->GetMethodID(storeClass, "getKey", "(Ljava/lang/String;[C)Ljava/security/Key;");
    if (!instance || !load || !getKey || !Good(env)) return nullptr;
    jobject store = env->CallStaticObjectMethod(storeClass, instance, Text(env, "AndroidKeyStore"));
    if (!store || !Good(env)) return nullptr;
    env->CallVoidMethod(store, load, nullptr, nullptr);
    if (!Good(env)) return nullptr;
    jstring alias = Text(env, "ethnir.noir.license.v1");
    jobject key = env->CallObjectMethod(store, getKey, alias, nullptr);
    if (!Good(env)) return nullptr;
    if (key || !create) return key;

    jclass builderClass = env->FindClass("android/security/keystore/KeyGenParameterSpec$Builder");
    if (!builderClass || !Good(env)) return nullptr;
    jmethodID ctor = env->GetMethodID(builderClass, "<init>", "(Ljava/lang/String;I)V");
    jmethodID blockModes = env->GetMethodID(builderClass, "setBlockModes",
        "([Ljava/lang/String;)Landroid/security/keystore/KeyGenParameterSpec$Builder;");
    jmethodID padding = env->GetMethodID(builderClass, "setEncryptionPaddings",
        "([Ljava/lang/String;)Landroid/security/keystore/KeyGenParameterSpec$Builder;");
    jmethodID build = env->GetMethodID(builderClass, "build", "()Landroid/security/keystore/KeyGenParameterSpec;");
    if (!ctor || !blockModes || !padding || !build || !Good(env)) return nullptr;
    jobject specBuilder = env->NewObject(builderClass, ctor, alias, 3);
    jclass strClass = env->FindClass("java/lang/String");
    if (!specBuilder || !strClass || !Good(env)) return nullptr;
    jobjectArray modes = env->NewObjectArray(1, strClass, Text(env, "GCM"));
    jobjectArray paddings = env->NewObjectArray(1, strClass, Text(env, "NoPadding"));
    if (!modes || !paddings || !Good(env)) return nullptr;
    env->CallObjectMethod(specBuilder, blockModes, modes);
    if (!Good(env)) return nullptr;
    env->CallObjectMethod(specBuilder, padding, paddings);
    if (!Good(env)) return nullptr;
    jobject spec = env->CallObjectMethod(specBuilder, build);
    if (!spec || !Good(env)) return nullptr;

    jclass generatorClass = env->FindClass("javax/crypto/KeyGenerator");
    if (!generatorClass || !Good(env)) return nullptr;
    jmethodID createGenerator = env->GetStaticMethodID(generatorClass, "getInstance",
        "(Ljava/lang/String;Ljava/lang/String;)Ljavax/crypto/KeyGenerator;");
    jmethodID initialize = env->GetMethodID(generatorClass, "init", "(Ljava/security/spec/AlgorithmParameterSpec;)V");
    jmethodID generate = env->GetMethodID(generatorClass, "generateKey", "()Ljavax/crypto/SecretKey;");
    if (!createGenerator || !initialize || !generate || !Good(env)) return nullptr;
    jobject generator = env->CallStaticObjectMethod(generatorClass, createGenerator,
        Text(env, "AES"), Text(env, "AndroidKeyStore"));
    if (!generator || !Good(env)) return nullptr;
    env->CallVoidMethod(generator, initialize, spec);
    if (!Good(env)) return nullptr;
    key = env->CallObjectMethod(generator, generate);
    return Good(env) ? key : nullptr;
}

inline jobject Cipher(JNIEnv* env) {
    jclass cls = env->FindClass("javax/crypto/Cipher");
    if (!cls || !Good(env)) return nullptr;
    jmethodID method = env->GetStaticMethodID(cls, "getInstance", "(Ljava/lang/String;)Ljavax/crypto/Cipher;");
    if (!method || !Good(env)) return nullptr;
    jobject cipher = env->CallStaticObjectMethod(cls, method, Text(env, "AES/GCM/NoPadding"));
    return Good(env) ? cipher : nullptr;
}

inline std::string Encode(JNIEnv* env, const std::vector<uint8_t>& bytes) {
    jclass cls = env->FindClass("android/util/Base64");
    if (!cls || !Good(env)) return {};
    jmethodID encode = env->GetStaticMethodID(cls, "encodeToString", "([BI)Ljava/lang/String;");
    if (!encode || !Good(env)) return {};
    jbyteArray data = JavaBytes(env, bytes.data(), bytes.size());
    if (!data) return {};
    jstring encoded = static_cast<jstring>(env->CallStaticObjectMethod(cls, encode, data, 2));
    return Good(env) ? NativeString(env, encoded) : std::string();
}

inline std::vector<uint8_t> Decode(JNIEnv* env, const std::string& data) {
    if (data.empty() || data.size() > 5500) return {};
    jclass cls = env->FindClass("android/util/Base64");
    if (!cls || !Good(env)) return {};
    jmethodID decode = env->GetStaticMethodID(cls, "decode", "(Ljava/lang/String;I)[B");
    if (!decode || !Good(env)) return {};
    jbyteArray decoded = static_cast<jbyteArray>(env->CallStaticObjectMethod(cls, decode,
        Text(env, data.c_str()), 2));
    return Good(env) ? Bytes(env, decoded) : std::vector<uint8_t>();
}

inline bool Save(const std::string& license) {
    if (license.empty() || license.size() > 255 || license.find('\0') != std::string::npos) return false;
    Session scope;
    if (!scope) return false;
    JNIEnv* env = scope.env;
    jobject preferences = Preferences(env);
    jobject key = Key(env, true);
    jobject cipher = Cipher(env);
    if (!preferences || !key || !cipher) return false;
    jclass cls = env->GetObjectClass(cipher);
    jmethodID init = env->GetMethodID(cls, "init", "(ILjava/security/Key;)V");
    jmethodID getIv = env->GetMethodID(cls, "getIV", "()[B");
    jmethodID finish = env->GetMethodID(cls, "doFinal", "([B)[B");
    if (!init || !getIv || !finish || !Good(env)) return false;
    env->CallVoidMethod(cipher, init, 1, key);
    if (!Good(env)) return false;
    jbyteArray input = JavaBytes(env, reinterpret_cast<const uint8_t*>(license.data()), license.size());
    if (!input) return false;
    jbyteArray encrypted = static_cast<jbyteArray>(env->CallObjectMethod(cipher, finish, input));
    jbyteArray iv = static_cast<jbyteArray>(env->CallObjectMethod(cipher, getIv));
    if (!Good(env)) return false;
    const auto packed = Pack(Bytes(env, iv), Bytes(env, encrypted));
    if (packed.empty()) return false;
    const std::string encoded = Encode(env, packed);
    return !encoded.empty() && WritePreference(env, preferences, &encoded);
}

inline bool Load(std::string& license) {
    license.clear();
    Session scope;
    if (!scope) return false;
    JNIEnv* env = scope.env;
    jobject preferences = Preferences(env);
    const std::string encoded = ReadPreference(env, preferences);
    if (encoded.empty()) return false;
    std::vector<uint8_t> iv, encrypted;
    if (!Unpack(Decode(env, encoded), iv, encrypted)) return false;
    jobject key = Key(env, false);
    jobject cipher = Cipher(env);
    if (!key || !cipher) return false;
    jclass cls = env->GetObjectClass(cipher);
    jmethodID init = env->GetMethodID(cls, "init",
        "(ILjava/security/Key;Ljava/security/spec/AlgorithmParameterSpec;)V");
    jmethodID finish = env->GetMethodID(cls, "doFinal", "([B)[B");
    jclass specClass = env->FindClass("javax/crypto/spec/GCMParameterSpec");
    jmethodID ctor = specClass ? env->GetMethodID(specClass, "<init>", "(I[B)V") : nullptr;
    if (!init || !finish || !ctor || !Good(env)) return false;
    jbyteArray javaIv = JavaBytes(env, iv.data(), iv.size());
    if (!javaIv) return false;
    jobject spec = env->NewObject(specClass, ctor, 128, javaIv);
    if (!spec || !Good(env)) return false;
    env->CallVoidMethod(cipher, init, 2, key, spec);
    if (!Good(env)) return false;
    jbyteArray bytes = JavaBytes(env, encrypted.data(), encrypted.size());
    if (!bytes) return false;
    jbyteArray plaintext = static_cast<jbyteArray>(env->CallObjectMethod(cipher, finish, bytes));
    if (!Good(env)) return false;
    auto decoded = Bytes(env, plaintext);
    if (decoded.empty() || decoded.size() > 255 ||
        std::find(decoded.begin(), decoded.end(), uint8_t(0)) != decoded.end()) return false;
    license.assign(decoded.begin(), decoded.end());
    std::fill(decoded.begin(), decoded.end(), 0);
    return true;
}

inline bool Clear() {
    Session scope;
    if (!scope) return false;
    jobject preferences = Preferences(scope.env);
    return WritePreference(scope.env, preferences, nullptr);
}

#else
inline bool Save(const std::string&) { return false; }
inline bool Load(std::string&) { return false; }
inline bool Clear() { return false; }
#endif
}
