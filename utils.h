#ifndef UTILS_H
#define UTILS_H
#include <vector>
#include <string>
using namespace std;

enum OPType {
    TUnary,
    TBinary,
    TNULL,
    TComparison
};

std::string stringf(const char* fmt, ...);
std::string join_label_pieces(std::vector<std::string> pieces);
bool splitByOperator(const std::string& str, string& left, std::string& right, std::string& op);
string removeSpaces(const string& input);
OPType judgeOperation(const string& input);
std::string stripOuterParentheses(const std::string exp);
#endif