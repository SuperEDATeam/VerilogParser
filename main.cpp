#include <iostream>
#include <cstdlib>
#include <fstream>
#include <string>
#include <direct.h>

#include "MyParser.h"
#include "Mynetlist.h"
#include "VerilogDotConverter.h"
#include "MySharedExpressions.h"

using namespace std;

int main(int argc, char* argv[]) {

    // 这段代码不能少，获取路径
    char cwd[1024];
    if (_getcwd(cwd, sizeof(cwd)) == nullptr) {
        std::cout << "Workplace Error! "<< std::endl;
        return 0;
    }
    std::string currentPath(cwd);

    // 读取输入文件内容
    std::string filePath = currentPath+"\\input\\input.v";  // 替换为你的文件路径
    std::ifstream file(filePath);  // 打开文件
    if (!file.is_open()) {
        std::cerr << "无法打开文件：" << filePath << std::endl;
        return 1;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();  // 将文件内容全部读入 buffer
    std::string input = buffer.str();  // 转换为字符串
    file.close();

    //std::string input = "module carry(a, b, c, cout, a_equal_b, a_shift_b)\n;input a, b, c\n;output cout, a_equal_b, a_shift_b\n;wire x\n;assign a_shift_b = a & b\n;assign a_equal_b = a == b\n;assign x = a & b & c\n;assign cout = x | c | b\n;endmodule";
    //std::string input = "module carry(a, cout);input a,;output cout;wire x;assign x = a;assign cout = x;endmodule;";
    //std::string input = "module carry(a, b, c, cout, a_equal_b, a_shift_b);input a, b, c;output cout, a_equal_b, a_shift_b;wire x, temp_0;assign temp_0 = a & b;assign a_shift_b = temp_0;assign a_equal_b = a == b;assign x = temp_0 | c;assign cout = x | c | b;endmodule";

    input.erase(std::remove(input.begin(), input.end(), '\n'), input.end());
    input.erase(std::remove(input.begin(), input.end(), '\r'), input.end());

    // 任务1
    Lexer lexer(input);
    Parser parser(lexer);
    MyDesign* des = new MyDesign;
    MyMidModuleMap midmap;
    try {
        parser.Parse(midmap);
    }
    catch (const std::runtime_error& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

	// 任务2
    std::string dotFilePath = currentPath + "\\output\\show1.dot";
    std::string pngFilePath = currentPath + "\\output\\show1.png";
    std::string mapStringPath = currentPath + "\\output\\map.txt";
    lexer.init();
    VerilogDotConverter* converter = new VerilogDotConverter(lexer,parser,midmap,des);

    // 获取 map 字符串
    std::string mapStr = converter->getMapString();
    // 写入 map.txt 文件
    std::ofstream mapFile(mapStringPath);
    if (mapFile.is_open()) {
        mapFile << mapStr;
        mapFile.close();
    }

    converter->getNetlist(dotFilePath);
    converter->convertDotToPng(dotFilePath, pngFilePath);
    converter->openPngFile(pngFilePath);

	// 任务3
    input = optimizeSharedExpressions(input);
    // std::cout << input << endl;

    Lexer lexer2(input);
    Parser parser2(lexer2);
    MyDesign* des2 = new MyDesign;
    MyMidModuleMap midmap2;
    try {
        parser2.Parse(midmap2);
    }
    catch (const std::runtime_error& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    dotFilePath = "output\\show2.dot";
    pngFilePath = "output\\show2.png";
    lexer2.init();
    VerilogDotConverter* converter2 = new VerilogDotConverter(lexer2, parser2, midmap2, des2);

    //// 获取 map 字符串
    //mapStr = converter2->getMapString();
    //// 写入 map.txt 文件
    //std::ofstream mapFile2(mapStringPath);
    //if (mapFile2.is_open()) {
    //    mapFile2 << mapStr;
    //    mapFile2.close();
    //}

    converter2->getNetlist(dotFilePath);
    converter2->convertDotToPng(dotFilePath, pngFilePath);
    converter2->openPngFile(pngFilePath);

	// 任务4，任务5
    std::string blif_path = "output\\output.blif";  // 修改为实际 output 文件夹所在地址
	converter2->convertVerilogToBlif(blif_path);  // 将优化后的 Verilog 转换为 BLIF 格式

    std::cout << "\n请选择调度方式：\n";
    std::cout << "1. ML-RCS 启发式调度\n";
    std::cout << "2. ILP 最优调度（耗时更久）\n";
    std::cout << "3. MR-LCS 最小资源调度\n";
    std::cout << "请输入选项（1、2 或 3）：";

    int choice;
    std::cin >> choice;  // 从 cmd 输入的选项值

    std::string command;

    if (choice == 1) {
        std::cout << "[启动 ML-RCS 启发式调度...]\n";
        command = "python python\\mlrcs.py";  //改为实际python文件存储地址
    }
    else if (choice == 2) {
        std::cout << "[启动 ILP 精确调度...]\n";
        command = "python python\\ILPtest.py"; //同理
    }
    else if (choice == 3) {
        std::cout << "[启动 MR-LCS 最小资源调度...]\n";
        command = "python python\\mrlcs.py"; // 同理
    }
    else {
        std::cerr << "无效输入，程序终止。\n";
        return 1;
    }

    int result = system(command.c_str()); //此处调度python文件，对应 command 存储的 str，在python文件中提供了不同的返回值(0--正确，1--特定错误)

    if (result != 0) {
        std::cerr << "调度失败，错误码：" << result << "\n";
        return 1;
    }

    std::cout << "调度完成。\n";

    return 0;
}