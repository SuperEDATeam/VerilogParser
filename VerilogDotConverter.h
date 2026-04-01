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

class VerilogDotConverter {
public:
    VerilogDotConverter();
    VerilogDotConverter(Lexer myLexer, Parser myParser,
                        MyMidModuleMap myMidModuleMap, MyDesign* myDesign) {
        lexer_  = &myLexer;
        parser_ = &myParser;
        midmap_ = myMidModuleMap;
        des_    = myDesign;
    }
    ~VerilogDotConverter();

    // Generate the Graphviz DOT netlist file
    std::string getNetlist(const std::string& filePath);

    // Return a human-readable dump of the module map
    std::string getMapString() const;

    // Convert a .dot file to PNG using Graphviz.
    // Returns true on success, false if dot.exe was not found or conversion failed.
    static bool convertDotToPng(const std::string& dotFilePath,
                                 const std::string& pngFilePath);

    // Open a PNG file with the default viewer.
    // Silently does nothing if the file does not exist.
    static void openPngFile(const std::string& pngFilePath);

    // Locate the Graphviz dot executable.
    // Searches common installation paths; returns the full path, or "dot" as fallback.
    static std::string findDotExe();

    // Convert the loaded Verilog design to BLIF format
    void convertVerilogToBlif(const std::string& blifFilePath);

private:
    std::string     fileinput_;
    Lexer*          lexer_  = nullptr;
    Parser*         parser_ = nullptr;
    MyMidModuleMap  midmap_;
    MyDesign*       des_    = nullptr;
};
