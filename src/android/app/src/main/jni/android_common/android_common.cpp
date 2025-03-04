// Copyright 2022 Citra Emulator Project
// Copyright 2024 Borked3DS Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "jni/android_common/android_common.h"

#include <string>
#include <string_view>

#include <jni.h>

#include "common/string_util.h"

std::string GetJString(JNIEnv* env, jstring jstr) {
    if (!jstr) {
        return {};
    }

    const jchar* jchars = env->GetStringChars(jstr, nullptr);
    const jsize length = env->GetStringLength(jstr);
    const std::u16string_view string_view(reinterpret_cast<const char16_t*>(jchars), length);
    const std::string converted_string = Common::UTF16ToUTF8(string_view);
    env->ReleaseStringChars(jstr, jchars);

    return converted_string;
}

jstring ToJString(JNIEnv* env, std::string_view str) {
    const std::u16string converted_string = Common::UTF8ToUTF16(str);
    return env->NewString(reinterpret_cast<const jchar*>(converted_string.data()),
                          static_cast<jint>(converted_string.size()));
}

jobjectArray ToJStringArray(JNIEnv* env, const std::vector<std::string>& strs) {
    jobjectArray array =
        env->NewObjectArray(strs.size(), env->FindClass("java/lang/String"), env->NewStringUTF(""));
    for (int i = 0; i < strs.size(); ++i) {
        env->SetObjectArrayElement(array, i, ToJString(env, strs[i]));
    }
    return array;
}
