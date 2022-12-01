#include <iostream>
#include <map>

using namespace std;

typedef float (*calculator)(float,float);

float add(float num1,float num2){return num1+num2;}
float mns(float num1,float num2){return num1-num2;}
float tms(float num1,float num2){return num1*num2;}
float div(float num1,float num2){return num1/num2;}

map<char,calculator> fun;

int main()
{
    char op;
    float num1, num2;
    cout << "输入运算符：+、-、*、/ : ";
    cin >> op;
 
    cout << "输入第一个数: ";
    cin >> num1;
    cout << "输入第二个数: ";
    cin >> num2;
    fun['+'] = add;
    fun['-'] = mns;
    fun['*'] = tms;
    fun['/'] = div;
    try
    {
        if((op == '/')&&(num2 == 0)) throw "ZeroDivisionError";
        else if(fun.count(op)) cout << fun[op](num1,num2) << endl;
        else cout << "Error!  请输入正确运算符。";
    }
    catch(char const* e){cout << e <<endl;}
 
    return 0;
}