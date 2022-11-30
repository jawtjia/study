//
// Created by wei.jia on 11/29/2022.
//

#include "const.h"
#include <jni.h>

extern "C"
JNIEXPORT jint JNICALL
Java_com_example_testapp_MainActivity_constTest(
        JNIEnv * env,
        jobject instance,
        jint i
        )
{
    int test1 = i;
    int test2 = 20;
    const int test3 = 30;
    int *const a = &test1; //a是指针，const直接修饰这个指针，表示这个指针本身（指针存里面存的地址，即指向）不能变。
    const int *b = &test1; //b是指针，const修饰的是int *b，表示b指针指向的内容不能通过*b来改变（可以直接改变test1的值来改变），但是指针本身（指向）是可以改变的
    const int *const c = &test1;//c是指针，本身（指向）不能改变，指向的内容不能通过指针*b来改变（可以直接改变test1的值来改变）。
    const int& d = test1;//与int const& e = test1;等价，d不能在后续使用中用来改变它绑定的值（可以直接改变它绑定的值test1来改变d引用的值）
    int const& e = test1;
    int& f = test1;//初始化的时候不能使用test3,d,e，但在后续赋值中可以（改变引用实际是改变它绑定的那个值，只要它绑定的那个值允许被改变就可以在后续赋值即可）。
    //printf("const test: a=%p,%d,b=%p,%d,c=%p,%d,d=%p,%d,e=%p,%d,f=%p,%d\n",a,*a,b,*b,c,*c,&d,d,&e,e,&f,f);
    *a = 3;
    b = &test2;
    f = test3;
    //c = &test2; //编译错误
    //*c = 4; //编译错误
    //a = &test2; //编译错误
    //*b = 22; //编译错误
    //printf("const test: a=%p,%d,b=%p,%d,c=%p,%d,d=%p,%d,e=%p,%d,f=%p,%d\n",a,*a,b,*b,c,*c,&d,d,&e,e,&f,f);
    //test1 = 10;
    //printf("const test: a=%p,%d,b=%p,%d,c=%p,%d,d=%p,%d,e=%p,%d,f=%p,%d\n",a,*a,b,*b,c,*c,&d,d,&e,e,&f,f);
    return test1;
}