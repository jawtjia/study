//
// Created by wei.jia on 11/24/2022.
//

#include <jni.h>
#include <string>
#include <iostream>
#include "cpplearn.h"

extern "C"
JNIEXPORT jint JNICALL
Java_com_example_testapp_MainActivity_sumFromJNI(
        JNIEnv *env,
        jobject instance,
        jint a,
        jint b) {
    return a+b;
}
