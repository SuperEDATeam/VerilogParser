// VerilogDotConverter.cpp
#include "VerilogDotConverter.h"
#include <cstdio>
#include <sys/stat.h>

VerilogDotConverter::VerilogDotConverter() {
    des_ = new MyDesign();
}

VerilogDotConverter::~VerilogDotConverter() {
    delete lexer_;
    delete parser_;
    delete des_;
}

std::string VerilogDotConverter::getNetlist(const std::string& filePath) {
    des_->My_show_netlist(midmap_, filePath);
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
        oss << "  Wires:\n";
        for (const auto& wire_pair : module->wires)
            oss << "    " << wire_pair.first << "\n";
        oss << "  Gates:\n";
        for (const auto& gate_pair : module->gate_insert_order)
            oss << "    " << gate_pair << "\n";
    }
    return oss.str();
}

// ---------- Graphviz path search ----------

std::string VerilogDotConverter::findDotExe() {
    // Common Windows installation paths for Graphviz
    static const char* candidates[] = {
        // Modern MSI installer (2.44+)
        "C:\\Program Files\\Graphviz\\bin\\dot.exe",
        "C:\\Program Files (x86)\\Graphviz\\bin\\dot.exe",
        // Older installers
        "C:\\Program Files\\Graphviz 2.38\\bin\\dot.exe",
        "C:\\Program Files (x86)\\Graphviz 2.38\\bin\\dot.exe",
        "C:\\Program Files\\Graphviz2.38\\bin\\dot.exe",
        "C:\\Program Files (x86)\\Graphviz2.38\\bin\\dot.exe",
        // Manual / portable installs
        "C:\\Graphviz\\bin\\dot.exe",
        "D:\\Graphviz\\bin\\dot.exe",
        nullptr
    };

    struct _stat st;
    for (int i = 0; candidates[i]; ++i) {
        if (_stat(candidates[i], &st) == 0)
            return std::string(candidates[i]);
    }

    // Nothing found in known paths -- hope it is on PATH
    return "dot";
}

// ---------- DOT -> PNG ----------

bool VerilogDotConverter::convertDotToPng(const std::string& dotFilePath,
                                           const std::string& pngFilePath) {
    std::string dotExe = findDotExe();

    // Quote both the exe path and the file paths in case of spaces
    std::string cmd = "\"" + dotExe + "\" -Tpng \""
                    + dotFilePath + "\" -o \"" + pngFilePath + "\"";

    int ret = std::system(cmd.c_str());
    if (ret != 0) {
        std::fprintf(stderr,
            "\nError: Graphviz 'dot' failed (exit code %d).\n"
            "  Command  : %s\n"
            "  If Graphviz is not installed, download it from:\n"
            "    https://graphviz.org/download/\n"
            "  If it is installed but not on PATH, add its bin/ folder to your\n"
            "  system PATH or copy dot.exe next to Main.exe.\n\n",
            ret, cmd.c_str());
        return false;
    }
    return true;
}

// ---------- Open PNG in default viewer ----------

void VerilogDotConverter::openPngFile(const std::string& pngFilePath) {
    struct _stat st;
    if (_stat(pngFilePath.c_str(), &st) != 0) {
        std::fprintf(stderr,
            "openPngFile: '%s' not found -- PNG was not generated.\n",
            pngFilePath.c_str());
        return;
    }
    std::string command = "start \"\" \"" + pngFilePath + "\"";
    std::system(command.c_str());
}

// ---------- BLIF ----------

void VerilogDotConverter::convertVerilogToBlif(const std::string& blifFilePath) {
    des_->My_show_blif(midmap_, blifFilePath);
}
