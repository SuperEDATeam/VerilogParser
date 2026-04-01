#include <iostream>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>
#include <direct.h>

#include "MyParser.h"
#include "Mynetlist.h"
#include "VerilogDotConverter.h"
#include "RtlilGenerator.h"
#include "Optimizer.h"
#include "InteractiveShell.h"

using namespace std;

// ---------- Batch-mode configuration ----------

struct BatchConfig {
    string inputFile;        // -v  input Verilog
    string outputRtlil;      // -o  unoptimised RTLIL
    string outputOptRtlil;   // -O  optimised RTLIL
    string outputDot;        // -d  DOT netlist
    string outputBlif;       // -b  BLIF
    bool   optShare  = false;
    bool   optConst  = false;
    bool   optClean  = false;
    bool   showPng   = false; // -s  open PNG viewer
};

static void printUsage(const char* prog) {
    cout << "Usage: " << prog << " [options]\n"
         << "  Without options: start interactive (Yosys-style) shell\n\n"
         << "Options:\n"
         << "  -v, --verilog <file>      Input Verilog file\n"
         << "  -o, --output  <file.il>   Write unoptimised RTLIL\n"
         << "  -O, --opt-out <file.il>   Write optimised RTLIL\n"
         << "  -d, --dot     <file.dot>  Write DOT netlist\n"
         << "  -b, --blif    <file.blif> Write BLIF\n"
         << "  --opt-share               Shared sub-expression elimination\n"
         << "  --opt-const               Constant propagation\n"
         << "  --opt-clean               Dead-code elimination\n"
         << "  -s, --show                Generate and open PNG circuit diagram\n"
         << "  -h, --help                Show this help\n\n"
         << "Examples:\n"
         << "  " << prog << "                                  # interactive mode\n"
         << "  " << prog << " -v carry.v -o carry.il           # write RTLIL\n"
         << "  " << prog << " -v carry.v --opt-share -O carry_opt.il -d carry.dot\n";
}

static BatchConfig parseArgs(int argc, char* argv[]) {
    BatchConfig cfg;
    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        auto nextArg = [&]() -> string {
            if (i + 1 < argc) return argv[++i];
            cerr << "Error: option " << arg << " requires an argument\n";
            exit(1);
        };

        if      (arg == "-v" || arg == "--verilog")  cfg.inputFile      = nextArg();
        else if (arg == "-o" || arg == "--output")   cfg.outputRtlil    = nextArg();
        else if (arg == "-O" || arg == "--opt-out")  cfg.outputOptRtlil = nextArg();
        else if (arg == "-d" || arg == "--dot")      cfg.outputDot      = nextArg();
        else if (arg == "-b" || arg == "--blif")     cfg.outputBlif     = nextArg();
        else if (arg == "--opt-share")               cfg.optShare  = true;
        else if (arg == "--opt-const")               cfg.optConst  = true;
        else if (arg == "--opt-clean")               cfg.optClean  = true;
        else if (arg == "-s" || arg == "--show")     cfg.showPng   = true;
        else if (arg == "-h" || arg == "--help")   { printUsage(argv[0]); exit(0); }
        else { cerr << "Unknown option: " << arg << "\n"; printUsage(argv[0]); exit(1); }
    }
    return cfg;
}

// ---------- Read Verilog from file ----------

static string readVerilog(const string& path) {
    ifstream file(path);
    if (!file.is_open()) {
        cerr << "Cannot open file: " << path << "\n";
        exit(1);
    }
    stringstream buf;
    buf << file.rdbuf();
    string content = buf.str();
    content.erase(remove(content.begin(), content.end(), '\n'), content.end());
    content.erase(remove(content.begin(), content.end(), '\r'), content.end());
    return content;
}

// ---------- Batch mode ----------

static int runBatch(const BatchConfig& cfg) {
    if (cfg.inputFile.empty()) {
        cerr << "Error: specify an input file with -v\n";
        return 1;
    }

    cout << "[1/4] Reading Verilog: " << cfg.inputFile << "\n";
    string content = readVerilog(cfg.inputFile);

    Lexer  lexer(content);
    Parser parser(lexer);
    MyMidModuleMap midmap;
    try {
        parser.Parse(midmap);
    } catch (const exception& e) {
        cerr << "Parse error: " << e.what() << "\n";
        return 1;
    }
    for (auto& kv : midmap.m_module_map)
        if (kv.second)
            cout << "      module=" << kv.second->modname
                 << "  gates=" << kv.second->gates.size() << "\n";

    // Write unoptimised RTLIL
    if (!cfg.outputRtlil.empty()) {
        cout << "[2/4] Writing unoptimised RTLIL: " << cfg.outputRtlil << "\n";
        RtlilGenerator gen;
        gen.generate(midmap, cfg.outputRtlil);
    } else {
        cout << "[2/4] Skipping unoptimised RTLIL (no -o specified)\n";
    }

    // Optimisation passes
    bool doOpt = cfg.optShare || cfg.optConst || cfg.optClean;
    if (doOpt) {
        cout << "[3/4] Running optimisation passes:\n";
        if (cfg.optShare) {
            int n = optimizer::optShareIR(midmap);
            cout << "      opt_share: " << n << " redundant expression(s) removed\n";
        }
        if (cfg.optConst) {
            int n = optimizer::optConstProp(midmap);
            cout << "      opt_const: " << n << " constant(s) propagated\n";
        }
        if (cfg.optClean) {
            int n = optimizer::optClean(midmap);
            cout << "      opt_clean: " << n << " dead gate(s) removed\n";
        }
    } else {
        cout << "[3/4] No optimisation passes requested\n";
    }

    // Write outputs
    cout << "[4/4] Writing outputs:\n";

    if (!cfg.outputOptRtlil.empty()) {
        cout << "      Optimised RTLIL: " << cfg.outputOptRtlil << "\n";
        RtlilGenerator gen;
        gen.generate(midmap, cfg.outputOptRtlil);
    }
    if (!cfg.outputDot.empty()) {
        cout << "      DOT: " << cfg.outputDot << "\n";
        MyDesign des;
        des.My_show_netlist(midmap, cfg.outputDot);
    }
    if (!cfg.outputBlif.empty()) {
        cout << "      BLIF: " << cfg.outputBlif << "\n";
        MyDesign des;
        des.My_show_blif(midmap, cfg.outputBlif);
    }
    if (cfg.showPng) {
        string dotFile = cfg.outputDot.empty() ? "output\\temp.dot" : cfg.outputDot;
        string pngFile = dotFile + ".png";
        if (cfg.outputDot.empty()) {
            MyDesign des;
            des.My_show_netlist(midmap, dotFile);
        }
        bool ok = VerilogDotConverter::convertDotToPng(dotFile, pngFile);
        if (ok) {
            cout << "      Opening PNG: " << pngFile << "\n";
            VerilogDotConverter::openPngFile(pngFile);
        } else {
            cout << "      DOT file saved to: " << dotFile << "\n";
            cout << "      PNG conversion failed (is Graphviz installed and on PATH?)\n";
        }
    }

    cout << "\nDone.\n";
    return 0;
}

// ---------- Entry point ----------

int main(int argc, char* argv[]) {
    char cwd[1024];
    _getcwd(cwd, sizeof(cwd)); // set working directory

    if (argc > 1) {
        // Batch mode
        BatchConfig cfg = parseArgs(argc, argv);
        return runBatch(cfg);
    } else {
        // Interactive mode
        InteractiveShell shell;
        shell.run();
        return 0;
    }
}
