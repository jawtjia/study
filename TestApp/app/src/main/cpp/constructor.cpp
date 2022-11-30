//
// Created by wei.jia on 11/24/2022.
//

#include "constructor.h"
#include <iostream>
#include <jni.h>

using namespace std;

class Line
{
public:
    int getLength( void );
    Line( int len );             // 简单的构造函数
    Line( const Line &obj);      // 拷贝构造函数
    ~Line();                     // 析构函数

private:
    int *ptr;
};

// 成员函数定义，包括构造函数
Line::Line(int len)
{
    cout << "调用构造函数" << __cplusplus << endl;
    // 为指针分配内存
    ptr = new int;//如果类中有指针并且有分配内存，那么一定要自己重载赋值符号“=”（在赋值对象的时候会被调用）或者重写拷贝构造函数（拷贝构造函数是构造函数的一个重载）。
    *ptr = len;
}

Line::Line(const Line &obj)
{
    cout << "调用拷贝构造函数并为指针 ptr 分配内存" << endl;
    ptr = new int;
    *ptr = *obj.ptr+10; // 拷贝值
}

Line::~Line(void)
{
    cout << "释放内存" << endl;
    delete ptr;
}

int Line::getLength( void )
{
    return *ptr;
}

void display(Line obj)
{
    cout << "line 大小 : " << obj.getLength() <<endl;
}

void display1(Line &obj)
{
    cout << "line 大小 : " << obj.getLength() <<endl;
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_example_testapp_MainActivity_constructor(JNIEnv * env, jobject instance)
{
    Line line1(10);//这里只调用了构造函数，新创建了一个对象。Line line1(Line(10));Line line1 = Line(10);都一样
    //Line line3(line1); //用另一个对象初始化一个对象，调用拷贝构造函数。
    Line line2 = line1;//这里调用了拷贝构造函数，将line1复制出新的对象赋值给line2【即line2（对象的引用）指向拷贝函数复制出来的对象（在堆中）】
    display(line1);//先调用line1的拷贝构造函数，将line2复制出新的对象赋值给display函数形参，函数调用完后释放（析构函数）line1复制出来的对象（类似于局部变量）。
    display(line2);//先调用line2的拷贝构造函数，将line2复制出新的对象赋值给display函数形参，函数调用完后释放（析构函数）line2复制出来的对象（类似于局部变量）。
    display(line1);//还是将line1作为参数传入拷贝构造函数所以一直都是10+10=20
    display(line1);
    display(line2);//line2是由line1调用拷贝构造函数创建的，所以它的值是20，这里有将line2本身作为参数调用拷贝构造函数，所以再加10，结构为30。
    display(line2);

    Line line11(10);//这里只调用了构造函数
    Line line21 = line11;//这里调用了拷贝构造函数，将line1复制出新的对象赋值给line2【即line2（对象的引用）指向拷贝函数复制出来的对象（在堆中）】
    display1(line11);//由于是引用，产生新的对象，直接使用line11，不调用拷贝函数。
    display1(line21);//由于是引用，产生新的对象，直接使用line21，不调用拷贝函数。
    display1(line11);//由于是引用，产生新的对象，直接使用line21，不调用拷贝函数。
    display1(line11);
    display1(line21);
    display1(line21);

    Line line12(10);//调用构造函数，产生新的对象
    Line &line22 = line12;//不产生新的对象
    display1(line12);//引用不产生新的对象
    display1(line22);//引用不产生新的对象
    display(line12);//赋值产生新的对象
    display(line22);//赋值产生新的对象

    Line linep1(10);
    Line *linep2 = &linep1;
    display1(linep1);
    display1(*linep2);
    display(linep1);
    display(*linep2);

    //while(1);
    return 0;//退出进程，释放line1和line2本体对象。
}

//总结：
//1.拷贝函数作用是在赋值的时候调用产生一个新的对象（不用调用构造函数），如果参数是引用的话就不会产生一个新的对象。
//2.C++中的引用就是变量的别名（对象和基本数据类型也是一样的，只是变量的名字），两个是等价的，一样的用法。Java中所说的引用就是变量名，Java中说的引用类似是C中的指针，指向堆中的变量。
//3.拷贝构造函数是构造函数的一个重载。如果不自定义的话，系统会自动默认一个简单的拷贝构造函数（简单的将成员变量复制一份，所以当存在指针内存分配时，很可能会造成内存泄露，需要自行重载赋值符号或者拷贝构造函数）。