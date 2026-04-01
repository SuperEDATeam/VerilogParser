#include "MySharedExpressions.h"
#include <sstream>
#include <unordered_map>
#include <algorithm>
#include <stdexcept>
#include <regex>

// 分割函数（支持清除空格）
std::vector<std::string> split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::istringstream tokenStream(str);
    std::string token;
    while (std::getline(tokenStream, token, delimiter)) {
        token.erase(0, token.find_first_not_of(" \t\r\n"));
        token.erase(token.find_last_not_of(" \t\r\n") + 1);
        if (!token.empty())
            tokens.push_back(token);
    }
    return tokens;
}

// 提取所有 assign 语句
std::vector<AssignStatement> parseAssignStatements(const std::string& input) {
    std::vector<AssignStatement> assigns;
    std::string keyword = "assign";
    size_t pos = 0;
    while ((pos = input.find(keyword, pos)) != std::string::npos) {
        size_t endPos = input.find(';', pos);
        if (endPos == std::string::npos) break;

        std::string assignStr = input.substr(pos + keyword.size(), endPos - (pos + keyword.size()));
        size_t eqPos = assignStr.find('=');
        if (eqPos != std::string::npos) {
            AssignStatement stmt;
            stmt.lhs = assignStr.substr(0, eqPos);
            stmt.rhs = assignStr.substr(eqPos + 1);
            stmt.lhs.erase(0, stmt.lhs.find_first_not_of(" \t\r\n"));
            stmt.lhs.erase(stmt.lhs.find_last_not_of(" \t\r\n") + 1);
            stmt.rhs.erase(0, stmt.rhs.find_first_not_of(" \t\r\n"));
            stmt.rhs.erase(stmt.rhs.find_last_not_of(" \t\r\n") + 1);
            assigns.push_back(stmt);
        }

        pos = endPos + 1;
    }
    return assigns;
}

// 主函数：优化共享子表达式

std::string optimizeSharedExpressions(const std::string& input) {
    std::vector<AssignStatement> assigns = parseAssignStatements(input);
    std::unordered_map<std::string, int> subExprCount;

    // 支持的操作符（顺序重要，长的优先匹配）
    std::vector<std::string> operators = { "==", "<=", ">=", "!=", "+", "-", "*", "/", "%", "<", ">", "&", "|", "^" };

    // 构建正则表达式匹配所有二元表达式
    std::string opPattern = "(";
    for (size_t i = 0; i < operators.size(); ++i) {
        if (i > 0) opPattern += "|";
        opPattern += "\\" + operators[i]; // 转义操作符
    }
    opPattern += ")";

    std::regex binaryExprRegex(R"((\w+)\s*)" + opPattern + R"(\s*(\w+))");

    // 收集二元子表达式频次
    for (const auto& stmt : assigns) {
        std::string expr = stmt.rhs;
        std::smatch match;
        std::string::const_iterator searchStart(expr.cbegin());

        while (std::regex_search(searchStart, expr.cend(), match, binaryExprRegex)) {
            std::string subExpr = match.str();
            subExpr.erase(0, subExpr.find_first_not_of(" \t\r\n"));
            subExpr.erase(subExpr.find_last_not_of(" \t\r\n") + 1);
            subExprCount[subExpr]++;
            searchStart = match.suffix().first;
        }
    }

    // 创建临时变量
    std::unordered_map<std::string, std::string> subExprToVar;
    int tempVarCounter = 0;
    for (const auto& pair : subExprCount) {
        if (pair.second > 1) {
            std::string tempVar = "temp_" + std::to_string(tempVarCounter++);
            subExprToVar[pair.first] = tempVar;
        }
    }

    std::ostringstream sharedDefs;
    std::ostringstream sharedAssign;
    if (!subExprToVar.empty()) {
        for (const auto& pair : subExprToVar) {
            sharedDefs << ", " << pair.second;
            sharedAssign << "assign " << pair.second << " = " << pair.first << ";";
        }
    }

    // 构建优化后的 assign 语句
    std::ostringstream optimizedCode;
    for (const auto& stmt : assigns) {
        std::string newRhs = stmt.rhs;

        // 替换子表达式（长的先替换）
        std::vector<std::pair<std::string, std::string>> orderedSubs(subExprToVar.begin(), subExprToVar.end());
        std::sort(orderedSubs.begin(), orderedSubs.end(), [](const auto& a, const auto& b) {
            return a.first.length() > b.first.length();
            });

        for (const auto& pair : orderedSubs) {
            size_t pos = 0;
            while ((pos = newRhs.find(pair.first, pos)) != std::string::npos) {
                newRhs.replace(pos, pair.first.length(), pair.second);
                pos += pair.second.length();
            }
        }

        optimizedCode << "assign " << stmt.lhs << " = " << newRhs << ";";
    }

    size_t moduleEndPos = input.find("endmodule");
    if (moduleEndPos == std::string::npos) {
        throw std::runtime_error("Invalid input: missing 'endmodule'");
    }

    size_t assignStart = input.find("assign");
    std::string result;
    if (!subExprToVar.empty()) {
        size_t inPut = input.find("input");
        size_t closeParen = input.rfind(")", inPut);
        result = input.substr(0, closeParen);
        result += sharedDefs.str();
        size_t semicolon = input.rfind(";", assignStart);
        result += input.substr(closeParen, semicolon - closeParen);
        result += sharedDefs.str();
        result += ";";
    }
    else {
        result = input.substr(0, assignStart);
    }

    result += sharedAssign.str();
    result += optimizedCode.str();
    result += "endmodule";

    return result;
}
