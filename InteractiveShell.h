#pragma once
#ifndef INTERACTIVE_SHELL_H
#define INTERACTIVE_SHELL_H

#include <string>
#include <vector>
#include <functional>
#include <map>

#include "MyMidData.h"
#include "MyParser.h"
#include "Mynetlist.h"
#include "RtlilGenerator.h"
#include "Optimizer.h"
#include "VerilogDotConverter.h"

// Yosys-style interactive command-line interface
class InteractiveShell {
public:
    InteractiveShell();

    // Start the interactive read-eval loop (blocks until 'exit')
    void run();

    // Parse and execute a single command line (also used for batch/test)
    void executeCommand(const std::string& line);

private:
    MyMidModuleMap currentDesign;
    bool hasDesign = false;

    // Command table: name -> handler(args)
    std::map<std::string, std::function<void(const std::vector<std::string>&)>> commands;

    void registerCommands();

    // Command handlers
    void cmdReadVerilog (const std::vector<std::string>& args);
    void cmdWriteRtlil  (const std::vector<std::string>& args);
    void cmdWriteDot    (const std::vector<std::string>& args);
    void cmdWriteBlif   (const std::vector<std::string>& args);
    void cmdOptShare    (const std::vector<std::string>& args);
    void cmdOptConst    (const std::vector<std::string>& args);
    void cmdOptClean    (const std::vector<std::string>& args);
    void cmdShow        (const std::vector<std::string>& args);
    void cmdStats       (const std::vector<std::string>& args);
    void cmdHelp        (const std::vector<std::string>& args);
    void cmdExit        (const std::vector<std::string>& args);
    void cmdWeb         (const std::vector<std::string>& args);

    // Read a Verilog file and strip newlines (Lexer requires single-line input)
    static std::string readVerilogFile(const std::string& path);

    // Check whether a filesystem path is an existing directory
    static bool dirExists(const std::string& path);
};

#endif
