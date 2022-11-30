//
// Created by wei.jia on 11/24/2022.
//

#include "smartpoint.h"
#include <iostream>
#include <string>
#include <memory>
#include <jni.h>

class Monster
{
    std::weak_ptr<Monster> m_father;
    std::weak_ptr<Monster> m_son;

public:
    void setFather(std::shared_ptr<Monster>& father);
    void setSon(std::shared_ptr<Monster>& son);
    Monster(){std::cout << "new Monster" << std::endl;};
    ~Monster(){std::cout << "A monster die!" << std::endl;}
};

void Monster::setFather(std::shared_ptr<Monster>& father)
{
    m_father = father; //如果m_father是shared_ptr 有互相调用时函数结束将不会自动释放内存
    std::cout << "set Father" << std::endl;
};

void Monster::setSon(std::shared_ptr<Monster>& son)
{
    m_son = son;//如果m_son是shared_ptr 有互相调用时函数结束将不会自动释放内存，因为
    std::cout << "set Son" << std::endl;
};

void runGame()
{
    Monster *m(new Monster());
    std::shared_ptr<Monster> father(new Monster());//can not be "std::shared_ptr<Monster> father = new Monster();
    std::shared_ptr<Monster> son(new Monster());//can not be "std::shared_ptr<Monster> son = new Monster();
    father->setSon(son);
    son->setFather(father);
    delete m;
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_example_testapp_MainActivity_smartPointTest(JNIEnv *env, jobject instance)
{
    runGame();
    return 5;
}

