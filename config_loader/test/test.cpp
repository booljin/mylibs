#include <iostream>
#include <stdio.h>

#include "ini_file.h"

using namespace std;

int main()
{
    CIniFile ini_file("test.ini");
#ifdef __DEBUG
    ini_file.debug();
#endif
    if(!ini_file.valid())
    {
        cout << "read file err" << endl;
        return 0;
    }
    char temp[10] = {0};
    ini_file.get_str("", "str", "default", temp, 10);
    printf("[].str = >%s<\n", temp);
    ini_file.get_str("AAA", "str", "default", temp, 10);
    printf("AAA.str = >%s<\n", temp);
    ini_file.get_str("AAA", "f", "default", temp, 10);
    printf("AAA.f = >%s<\n", temp);
    ini_file.get_str("BBB", "str", "default", temp, 10);
    printf("BBB.str = >%s<\n", temp);
    
    printf("[].int = >%i<\n", ini_file.get_int("", "int", -90));
    printf("[].u = >%u<\n", ini_file.get_uint("u", -90));
    printf("[].u64 = >%I64u<\n", ini_file.get_uint64("", "u64", 1234567890123));
    printf("[AAA].f = >%f<\n", ini_file.get_float("AAA", "f", 3.00));
    
    printf("test: %f\n", strtof("3.14", NULL));
    // int t, ta, tb;
    // ini_file.GetInt("AAA","a",0,&ta);
    // ini_file.GetInt("","a",0,&t);
    // ini_file.GetInt("BBB","a",0,&tb);
    // cout << "a, AAA.a, BBB.a = " << t << " " << ta << " " << tb << endl;
    
    // char c[30] = {0};
    // char ca[30] = {0};
    // char cb[30] = {0};
    // ini_file.GetString("", "b", "default", c, 30);
    // ini_file.GetString("AAA", "b", "default", ca, 30);
    // ini_file.GetString("BBB", "b", "default", cb, 30);
    // cout << "b, AAA.b, BBB.b = " << c << " " << ca << " " << cb << endl;
    
    // int u, ua, ub;
    // ini_file.GetInt("AAA","c",0,&ua);
    // ini_file.GetInt("","aa",99,&u);
    // ini_file.GetInt("BBB","aac",99,&ub);
    // cout << "u, AAA.u, BBB.u = " << u << " " << ua << " " << ub << endl;
    
    return 0;
}