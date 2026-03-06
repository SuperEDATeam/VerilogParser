// VerilogDotConverter.h
#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <map>
#include <vector>
#include <string>
#include "MyParser.h"
#include "MyNetlist.h"
#include <string>     // for std::string

class VerilogDotConverter {
public:
    VerilogDotConverter();
    VerilogDotConverter(Lexer myLexer,Parser myParser,MyMidModuleMap myMidModuleMap,MyDesign* myDesign) {
		lexer_ = &myLexer;
		parser_ = &myParser;
        midmap_ = myMidModuleMap;
		des_ = myDesign;
    }
    ~VerilogDotConverter();

    // 解析并显示网表，生成Dot文件
    std::string getNetlist(const std::string& filePath);

    // 打印中间模块映射信息
    std::string getMapString() const;

	// 将Dot文件转换为PNG图片
    static void convertDotToPng(const std::string& dotFilePath, const std::string& pngFilePath);

	// 打开PNG文件
    static void openPngFile(const std::string& pngFilePath);

	// 将Verilog转换为BLIF格式
	void convertVerilogToBlif(const std::string& blifFilePath);

private:
    std::string fileinput_;
    Lexer* lexer_ = nullptr;
    Parser* parser_ = nullptr;
    MyMidModuleMap midmap_;
    MyDesign* des_ = nullptr; // 用户设计 elaborator 类型
};