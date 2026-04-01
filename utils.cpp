#include "utils.h"
#include <stdio.h>
#include <stdarg.h>


inline std::string vstringf(const char* fmt, va_list ap)
{
    // For the common case of strings shorter than 128, save a heap
    // allocation by using a stack allocated buffer.
    const int kBufSize = 128;
    char buf[kBufSize];
    buf[0] = '\0';
    va_list apc;
    va_copy(apc, ap);
    int n = vsnprintf(buf, kBufSize, fmt, apc);
    va_end(apc);
    if (n < kBufSize)
        return std::string(buf);

    std::string string;
    char* str = NULL;
#if defined(_WIN32 )|| defined(__CYGWIN__)
    int sz = 2 * kBufSize, rc;
    while (1) {
        va_copy(apc, ap);
        str = (char*)realloc(str, sz);
        rc = vsnprintf(str, sz, fmt, apc);
        va_end(apc);
        if (rc >= 0 && rc < sz)
            break;
        sz *= 2;
    }
    if (str != NULL) {
        string = str;
        free(str);
    }
    return string;
#else
    if (vasprintf(&str, fmt, ap) < 0)
        str = NULL;
    if (str != NULL) {
        string = str;
        free(str);
    }
    return string;
#endif
}

std::string stringf(const char* fmt, ...)
{
    std::string string;
    va_list ap;

    va_start(ap, fmt);
    string = vstringf(fmt, ap);
    va_end(ap);

    return string;
}


// Return the pieces of a label joined by a '|' separator
std::string join_label_pieces(std::vector<std::string> pieces)
{
    std::string ret = "";
    bool first_piece = true;

    for (auto& piece : pieces) {
        if (!first_piece)
            ret += "|";
        ret += piece;
        first_piece = false;
    }

    return ret;
}

//将字符串按运算符划分为左右两个子字符串

std::string trim(const std::string& str) {
    // 去掉前导空格
    auto start = std::find_if(str.begin(), str.end(), [](unsigned char c) {
        return !std::isspace(c);
        });

    // 去掉尾部空格
    auto end = std::find_if(str.rbegin(), str.rend(), [](unsigned char c) {
        return !std::isspace(c);
        }).base();

        // 如果字符串全是空格，返回空字符串
        if (start >= end) {
            return "";
        }

        // 返回去掉前后空格的子字符串
        return std::string(start, end);
}

//bool splitByOperator(const string& str, string& left, string& right, char& op) {
//
//    string operators = "=+-*/&|";
//
//    bool re;
//    // 遍历字符串，查找运算符
//    for (size_t i = 0; i < str.size(); ++i) {
//        if (operators.find(str[i]) != string::npos) {
//            // 找到运算符，分割字符串
//            op = str[i];                    //运算符
//            left = trim(str.substr(0, i));       // 左子字符串
//            right = trim(str.substr(i + 1));     // 右子字符串
//            return true;
//        }
//    }
//
//    left = str;
//    right = "";
//    return false;
//}

bool splitByOperator(const std::string& strold, std::string& left, std::string& right, std::string& op) {
    string str;
    str=stripOuterParentheses(strold);
    // 单字符和双字符运算符列表
    const string lowest_priority_ops = "=";       // 赋值
    const string low_priority_ops = "||";         // 逻辑或
    const string mid_low_priority_ops = "&&";     // 逻辑与
    const string mid_priority_ops = "|";          // 按位或
    const string mid_high_priority_ops = "^&";    // 按位异或、按位与
    const string high_priority_ops[] = { "==", "!=", ">=", "<=" };  // 比较运算符
    const string highest_priority_ops = "+-*/";   // 算术运算符

    size_t ops_size = sizeof(high_priority_ops) / sizeof(high_priority_ops[0]); // 获取数组长度

    int min_priority = INT_MAX;
    int split_pos = -1;
    string min_op;
    int depth = 0;  // 记录括号深度

    for (size_t i = 0; i < str.size(); ++i) {
        char c = str[i];

        // 处理括号，忽略括号内部的运算符
        if (c == '(') {
            depth++;
            continue;
        }
        if (c == ')') {
            depth--;
            continue;
        }

        // 仅在括号外寻找运算符
        if (depth == 0) {
            string current_op;

            // 检查是否是双字符运算符
            if (i + 1 < str.size()) {
                string two_chars = str.substr(i, 2);
                if (two_chars == "==" || two_chars == "!=" || two_chars == "<=" || two_chars == ">=" ||
                    two_chars == "&&" || two_chars == "||") {
                    current_op = two_chars;
                    i = i + 1;
                }
            }

            // 检查是否是单字符运算符
            if (current_op.empty()) {
                current_op = string(1, c);
            }

            // 处理运算符优先级
            int priority = INT_MAX;
            if (lowest_priority_ops.find(current_op) != string::npos) priority = 1;
            else if (low_priority_ops.find(current_op) != string::npos) priority = 2;
            else if (mid_low_priority_ops.find(current_op) != string::npos) priority = 3;
            else if (mid_priority_ops.find(current_op) != string::npos) priority = 4;
            else if (mid_high_priority_ops.find(current_op) != string::npos) priority = 5;
            else if (std::find(high_priority_ops, high_priority_ops + ops_size, current_op) != high_priority_ops + ops_size) priority = 6;
            else if (highest_priority_ops.find(current_op) != string::npos) priority = 7;

            // 避免将一元 `-` 或 `+` 误认为二元运算符
            if ((c == '-' || c == '+') && (i == 0 || str[i - 1] == '(' || str[i - 1] == ' ')) {
                continue; // 作为一元运算符，跳过
            }

            // 更新最低优先级运算符
            if (priority < min_priority) {
                min_priority = priority;
                min_op = current_op;
                if (current_op.size() == 1) {
                    split_pos = i;
                }
                else { split_pos = i - 1; }
            }
        }
    }

    // 进行拆分
    if (split_pos != -1) {
        op = min_op;
        left = trim(str.substr(0, split_pos));
        right = trim(str.substr(split_pos + min_op.length()));
        return true;
    }

    // 如果没有找到运算符，则返回整个字符串
    left = trim(str);
    right = "";
    return false;
}
    // 遍历字符串，查找运算符
    //for (size_t i = 0; i < str.size(); ++i) {
    //    // 检查双字符运算符
    //    if (i + 1 < str.size()) {
    //        std::string potentialOp = str.substr(i, 2); // 取当前字符和下一个字符
    //        for (const std::string& opStr : doubleCharOperators) {
    //            if (potentialOp == opStr) {
    //                // 找到双字符运算符
    //                op = potentialOp;
    //                left = trim(str.substr(0, i));         // 左子字符串
    //                right = trim(str.substr(i + 2));       // 右子字符串
    //                return true;
    //            }
    //        }
    //    }

    //    // 检查单字符运算符
    //    if (singleCharOperators.find(str[i]) != std::string::npos) {
    //        // 找到单字符运算符
    //        op = str[i];
    //        left = trim(str.substr(0, i));         // 左子字符串
    //        right = trim(str.substr(i + 1));       // 右子字符串
    //        return true;
    //    }
    //}


// 去除字符串中的空格
string removeSpaces(const string& input) {
    std::string result;
    for (char ch : input) {
        if (!std::isspace(ch)) { // 如果不是空格字符，则加入结果字符串
            result += ch;
        }
    }
    return result;
}



// 判断运算类型
OPType judgeOperation(const string& input) {
    OPType re = TNULL;
    const string unaryOperators = "!";
    const string binaryOperators = "+-*/|&";
    const string comparisonOperators = "== != < > <= >=";  // 比较运算符

    string trimmedInput = removeSpaces(input);  // 去除空格

    if (trimmedInput.empty()) {
        return TNULL;
    }

    // 检查是否为一元运算
    if (unaryOperators.find(trimmedInput[0]) != string::npos && trimmedInput.size() == 1) {
        return TUnary;
    }

    // 检查是否为二元运算
    for (size_t i = 0; i < trimmedInput.size(); ++i) {
        if (binaryOperators.find(trimmedInput[i]) != string::npos) {
            return TBinary;
        }
    }

    // 检查是否为比较运算
    for (size_t i = 0; i < trimmedInput.size() - 1; ++i) {
        // 处理双字符比较运算符（如 ==, !=, <=, >=）
        if ((trimmedInput[i] == '=' && trimmedInput[i + 1] == '=') ||
            (trimmedInput[i] == '!' && trimmedInput[i + 1] == '=') ||
            (trimmedInput[i] == '<' && trimmedInput[i + 1] == '=') ||
            (trimmedInput[i] == '>' && trimmedInput[i + 1] == '=')) {
            return TComparison;
        }
    }

    // 处理单字符比较运算符（如 <, >）
    if (trimmedInput == "<" || trimmedInput == ">" || trimmedInput == "!=") {
        return TComparison;
    }

    return TNULL;
}

std::string stripOuterParentheses(const std::string exp) {
    if (exp.empty()) return exp;

    int count = 0;
    size_t len = exp.length();

    // 检查是否有最外层的括号
    if (exp[0] == '(' && exp[len - 1] == ')') {
        for (size_t i = 0; i < len; ++i) {
            if (exp[i] == '(') count++;
            else if (exp[i] == ')') count--;

            // 如果在最后一个字符前 count 归零，说明外层括号不是成对的
            if (count == 0 && i < len - 1) {
                return exp;  // 说明最外层括号不是冗余的，不能去掉
            }
        }
        // 如果 count == 0，说明最外层括号是多余的，可以去掉
        return exp.substr(1, len - 2);
    }
    return exp;
}

