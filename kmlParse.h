/**
* kmlParse.cpp : 实现操作方法
* Description：利用Tiny解析kml文件，及与实时GPS数据的处理
* Lead author：蔡寰宇
* Deputy author：陈宽
* Deputy author：李聚升
* Version：1.3 2013.8.19 9:00
*/

#include<iostream>
#include"tinyxml.h"
#include<string>
#include<sstream>
#include<math.h>
#include<fstream>//文件操作

using namespace std;