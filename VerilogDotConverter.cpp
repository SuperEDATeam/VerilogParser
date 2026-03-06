// VerilogDotConverter.cpp
#include "VerilogDotConverter.h"

VerilogDotConverter::VerilogDotConverter() {
    // Initialize design container
    des_ = new MyDesign();
}

VerilogDotConverter::~VerilogDotConverter() {
    delete lexer_;
    delete parser_;
    delete des_;
}

std::string VerilogDotConverter::getNetlist(const std::string& filePath) {
	des_->My_show_netlist(midmap_,filePath);
    return "";
}

std::string VerilogDotConverter::getMapString() const {
    const MyMidModuleMap& midmap = midmap_;
    std::ostringstream oss;

    for (const auto& module_pair : midmap.m_module_map) {
        const std::string& module_name = module_pair.first;
        const MyMidModule* module = module_pair.second;

        oss << "Module: " << module_name << "\n";
        oss << "  Modname: " << module->modname << "\n";

        oss << "  Wires: \n";
        for (const auto& wire_pair : module->wires) {
            oss << "    " << wire_pair.first << "\n";
        }

        oss << "  Gates: \n";
        for (const auto& gate_pair : module->gate_insert_order) {
            oss << "    " << gate_pair << "\n";
        }
    }

    return oss.str();
}

void VerilogDotConverter::convertDotToPng(const std::string& dotFilePath, const std::string& pngFilePath) {
    // 使用Graphviz的dot工具将.dot文件转换为.png
    std::string command = "dot -Tpng \"" + dotFilePath + "\" -o \"" + pngFilePath + "\"";
    std::system(command.c_str());
}

void VerilogDotConverter::openPngFile(const std::string& pngFilePath) {

    std::string command = "start \"\" \"" + pngFilePath + "\"";
    std::system(command.c_str());
}

void VerilogDotConverter::convertVerilogToBlif(const std::string& blifFilePath) {
    des_->My_show_blif(midmap_,blifFilePath);
}