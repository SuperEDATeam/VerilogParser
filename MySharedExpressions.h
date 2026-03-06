#pragma once
#ifndef MY_SHARED_EXPRESSIONS_H
#define MY_SHARED_EXPRESSIONS_H

#include <string>
#include <vector>
#include <regex>

struct AssignStatement {
    std::string lhs;
    std::string rhs;
};

// 主优化函数：输入原始 Verilog 字符串，返回优化后的字符串
std::string optimizeSharedExpressions(const std::string& input);

#endif // MY_SHARED_EXPRESSIONS_H
