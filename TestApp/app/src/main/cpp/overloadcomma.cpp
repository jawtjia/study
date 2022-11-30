//
// Created by wei.jia on 11/24/2022.
//

#include <iostream>
#include <vector>
#include "overloadcomma.h"
#include <jni.h>

using namespace std;
class Complex {
private:
    double i;
    double j;
public:
    Complex(int = 0, int = 0);
    void display();
    Complex& operator ++();//前缀自增
    Complex operator ++(int);//后缀自增，参数需要加int
};

Complex::Complex(int a, int b) {
    i = a;
    j = b;
}

void Complex::display() {
    cout << "i="<< i <<"\tj="<< j << endl;
}

Complex& Complex::operator ++() {
    ++i;
    ++j;
    return *this;
}

Complex Complex::operator ++(int) {
    Complex temp = *this;
    ++* this;
    return temp;
}

class A
{
private:
    int a;
public:
    A();
    A(int n);
    A operator+(const A & obj);
    A operator+(const int b);
    friend A operator+(const int b, A obj);
    void display();
} ;

A::A()
{
    a=0;
}
A::A(int n)//构造函数
{
    a=n;
}
A A::operator +(const A& obj)//重载+号用于 对象相加
{
    return this->a+obj.a;
}
A A::operator+(const int b)//重载+号用于  对象与数相加
{
    return A(a+b);
}
A operator+(const int b,  A obj)
{
    return obj+b;//友元函数调用第二个重载+的成员函数  相当于 obj.operator+(b);
}
void A::display()
{
    cout<<a<<endl;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_testapp_MainActivity_overloadPlusplus(JNIEnv *env,jobject instance)
{
    Complex comnum1(2, 2), comnum2, comnum3;
    cout << "自增计算前:" << endl;
    cout << "comnum1:";
    comnum1.display();
    cout << "comnum2:";
    comnum2.display();
    cout << "comnum3:";
    comnum3.display();
    cout << endl;

    cout << "前缀自增计算后：" << endl;
    comnum2 = ++comnum1;
    cout << "comnum1:";
    comnum1.display();
    cout << "comnum2:";
    comnum2.display();
    cout << endl;

    cout << "后缀自增计算后:" << endl;
    comnum3 = comnum1++;
    cout << "comnum1:";
    comnum1.display();
    cout << "comnum3:";
    comnum3.display();

    cout << "前缀递增加引用是为了连续自加++(++a)" << endl;
    Complex comcum4;
    ++(++comcum4);
    cout << "comcum4:";
    comcum4.display();

    cout << "不加引用连续自加(a++)++" << endl;
    Complex comcum5;
    (comcum5++)++;
    cout << "comcum5:";
    comcum5.display();

    A a1(1);
    A a2(2);
    A a3,a4,a5;
    a1.display();
    a2.display();
    int m=1;
    a3=a1+a2;//可以交换顺序，相当月a3=a1.operator+(a2);
    a3.display();
    a4=a1+m;//因为加了个友元函数所以也可以交换顺序了。
    a4.display();
    a5=m+a1+a2+m;
    a5.display();

    cout << "OVERLOAD_PLUSPLUS_TEST_DONE" << endl << endl;
}

// 假设一个实际的类
class Obj {
    static int i, j;
public:
    void f() const { cout << i++ << endl; }
    void g() const { cout << j++ << endl; }
};

// 静态成员定义
int Obj::i = 10;
int Obj::j = 12;

// 为上面的类实现一个容器
class ObjContainer {
    vector<Obj*> a;
public:
    void add(Obj* obj)
    {
        a.push_back(obj);  // 调用向量的标准方法
    }
    friend class SmartPointer;
};

// 实现智能指针，用于访问类 Obj 的成员
class SmartPointer {
    ObjContainer oc;
    int index;
public:
    SmartPointer(ObjContainer& objc)
    {
        oc = objc;
        index = 0;
    }
    // 返回值表示列表结束
    bool operator++() // 前缀版本
    {
        if(index >= oc.a.size() - 1) return false;
        if(oc.a[++index] == 0) return false;
        return true;
    }
    bool operator++(int) // 后缀版本
    {
        return operator++();
    }
    // 重载运算符 ->
    Obj* operator->() const
    {
        if(!oc.a[index])
        {
            cout << "Zero value";
            return (Obj*)0;
        }
        return oc.a[index];
    }
};

extern "C"
JNIEXPORT void JNICALL
Java_com_example_testapp_MainActivity_overloadPointComma(JNIEnv *env,jobject instance)
{
    const int sz = 10;
    Obj o[sz];
    ObjContainer oc;
    for(int i = 0; i < sz; i++)
    {
        oc.add(&o[i]);
    }
    SmartPointer sp(oc); // 创建一个迭代器
    do {
        sp->f(); // 智能指针调用
        sp->g();
    } while(sp++);

    cout << "OVERLOAD_POINT_COMMA_TEST_DONE" << endl << endl;
}

extern "C"
JNIEXPORT jstring JNICALL
Java_com_example_testapp_MainActivity_overloadTest(JNIEnv *env,jobject instance)
{
    std::string h = "I am overload test fun";
    return env->NewStringUTF(h.c_str());
}