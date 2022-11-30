//
// Created by wei.jia on 11/25/2022.
//

#include "virtualclass.h"
#include <iostream>
#include <jni.h>

using namespace std;

// 基类
class Shape
{
public:
    // 提供接口框架的纯虚函数
    virtual int getArea() = 0;
    void setWidth(int w)
    {
        width = w;
    }
    void setHeight(int h)
    {
        height = h;
    }
protected:
    int width;
    int height;
};

// 派生类
class Rectangle: public Shape
{
public:
    int getArea()
    {
        return (width * height);
    }
};
class Triangle: public Shape
{
public:
    int getArea()
    {
        return (width * height)/2;
    }
};

extern "C"
JNIEXPORT jint JNICALL
Java_com_example_testapp_MainActivity_virtualTest(
        JNIEnv *env,
        jobject instance,
        jint a,
        jint b) {
    Rectangle Rect;
    Triangle  Tri;
    jint c;

    Rect.setWidth(a);
    Rect.setHeight(b);
    c = Rect.getArea();
    return c;
}