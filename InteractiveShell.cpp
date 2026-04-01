#include "InteractiveShell.h"

#include <iostream>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <stdexcept>
#include <sys/stat.h>
#include <windows.h>   // Sleep()

// ---------- Constructor / Command registration ----------

InteractiveShell::InteractiveShell() {
    registerCommands();
}

void InteractiveShell::registerCommands() {
    commands["read_verilog"] = [this](const auto& a) { cmdReadVerilog(a); };
    commands["write_rtlil"]  = [this](const auto& a) { cmdWriteRtlil(a);  };
    commands["write_dot"]    = [this](const auto& a) { cmdWriteDot(a);    };
    commands["write_blif"]   = [this](const auto& a) { cmdWriteBlif(a);   };
    commands["opt_share"]    = [this](const auto& a) { cmdOptShare(a);    };
    commands["opt_const"]    = [this](const auto& a) { cmdOptConst(a);    };
    commands["opt_clean"]    = [this](const auto& a) { cmdOptClean(a);    };
    commands["show"]         = [this](const auto& a) { cmdShow(a);        };
    commands["stats"]        = [this](const auto& a) { cmdStats(a);       };
    commands["help"]         = [this](const auto& a) { cmdHelp(a);        };
    commands["exit"]         = [this](const auto& a) { cmdExit(a);        };
    commands["web"]          = [this](const auto& a) { cmdWeb(a);         };

    // Aliases
    commands["read"]  = commands["read_verilog"];
    commands["quit"]  = commands["exit"];
    commands["q"]     = commands["exit"];
    commands["?"]     = commands["help"];
}

// ---------- Main loop ----------

void InteractiveShell::run() {
    std::cout << "\n";
    std::cout << "  ================================================\n";
    std::cout << "   VerilogCompiler -- Interactive RTL Synthesis\n";
    std::cout << "   Yosys-style CLI.  Type 'help' for commands.\n";
    std::cout << "  ================================================\n\n";

    std::string line;
    while (true) {
        std::cout << "\nyosys> ";
        std::cout.flush();
        if (!std::getline(std::cin, line)) break; // EOF
        executeCommand(line);
    }
}

void InteractiveShell::executeCommand(const std::string& line) {
    // Strip comments (# ...)
    std::string trimmed = line;
    size_t hashPos = trimmed.find('#');
    if (hashPos != std::string::npos) trimmed = trimmed.substr(0, hashPos);

    // Tokenize
    std::istringstream iss(trimmed);
    std::vector<std::string> tokens;
    std::string tok;
    while (iss >> tok) tokens.push_back(tok);
    if (tokens.empty()) return;

    const std::string& cmd = tokens[0];
    std::vector<std::string> args(tokens.begin() + 1, tokens.end());

    auto it = commands.find(cmd);
    if (it != commands.end()) {
        try {
            it->second(args);
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << "\n";
        }
    } else {
        std::cerr << "Unknown command: '" << cmd << "'  (type 'help' for a list)\n";
    }
}

// ---------- Utility ----------

std::string InteractiveShell::readVerilogFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open())
        throw std::runtime_error("Cannot open file: " + path);

    std::stringstream buf;
    buf << file.rdbuf();
    std::string content = buf.str();

    // Strip newlines (Lexer requires a single-line string)
    content.erase(std::remove(content.begin(), content.end(), '\n'), content.end());
    content.erase(std::remove(content.begin(), content.end(), '\r'), content.end());
    return content;
}

// ---------- Command implementations ----------

void InteractiveShell::cmdReadVerilog(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cerr << "Usage: read_verilog <file.v>\n";
        return;
    }

    std::string content;
    try {
        content = readVerilogFile(args[0]);
    } catch (const std::exception& e) {
        std::cerr << e.what() << "\n";
        return;
    }

    currentDesign.m_module_map.clear();
    hasDesign = false;

    Lexer  lexer(content);
    Parser parser(lexer);
    try {
        parser.Parse(currentDesign);
        hasDesign = true;
    } catch (const std::exception& e) {
        std::cerr << "Parse error: " << e.what() << "\n";
        return;
    }

    for (auto& kv : currentDesign.m_module_map) {
        if (!kv.second) continue;
        auto* mod = kv.second;
        int inputs = 0, outputs = 0, middle = 0;
        for (auto& w : mod->wires) {
            switch (w.second->wiretype) {
                case PWire::INPORT:  ++inputs;  break;
                case PWire::OUTPORT: ++outputs; break;
                case PWire::MIDDLE:  ++middle;  break;
            }
        }
        std::cout << "Loaded module: " << mod->modname << "\n";
        std::cout << "  inputs=" << inputs << "  outputs=" << outputs
                  << "  wires=" << middle << "  gates=" << mod->gates.size() << "\n";
    }
}

void InteractiveShell::cmdWriteRtlil(const std::vector<std::string>& args) {
    if (!hasDesign) { std::cerr << "No design loaded. Run read_verilog first.\n"; return; }
    if (args.empty()) { std::cerr << "Usage: write_rtlil <output.il>\n"; return; }

    RtlilGenerator gen;
    if (gen.generate(currentDesign, args[0]) == 0)
        std::cout << "RTLIL written to: " << args[0] << "\n";
}

void InteractiveShell::cmdWriteDot(const std::vector<std::string>& args) {
    if (!hasDesign) { std::cerr << "No design loaded. Run read_verilog first.\n"; return; }
    if (args.empty()) { std::cerr << "Usage: write_dot <output.dot>\n"; return; }

    MyDesign des;
    des.My_show_netlist(currentDesign, args[0]);
    std::cout << "DOT written to: " << args[0] << "\n";
}

void InteractiveShell::cmdWriteBlif(const std::vector<std::string>& args) {
    if (!hasDesign) { std::cerr << "No design loaded. Run read_verilog first.\n"; return; }
    if (args.empty()) { std::cerr << "Usage: write_blif <output.blif>\n"; return; }

    MyDesign des;
    des.My_show_blif(currentDesign, args[0]);
    std::cout << "BLIF written to: " << args[0] << "\n";
}

void InteractiveShell::cmdOptShare(const std::vector<std::string>& args) {
    if (!hasDesign) { std::cerr << "No design loaded. Run read_verilog first.\n"; return; }

    int before = 0;
    for (auto& kv : currentDesign.m_module_map)
        if (kv.second) before += (int)kv.second->gates.size();

    int eliminated = optimizer::optShareIR(currentDesign);

    int after = 0;
    for (auto& kv : currentDesign.m_module_map)
        if (kv.second) after += (int)kv.second->gates.size();

    std::cout << "opt_share: " << before << " gates -> " << after
              << " gates  (" << eliminated << " redundant expression(s) removed)\n";
}

void InteractiveShell::cmdOptConst(const std::vector<std::string>& args) {
    if (!hasDesign) { std::cerr << "No design loaded. Run read_verilog first.\n"; return; }
    int n = optimizer::optConstProp(currentDesign);
    std::cout << "opt_const: " << n << " constant(s) propagated\n";
}

void InteractiveShell::cmdOptClean(const std::vector<std::string>& args) {
    if (!hasDesign) { std::cerr << "No design loaded. Run read_verilog first.\n"; return; }
    int n = optimizer::optClean(currentDesign);
    std::cout << "opt_clean: " << n << " dead gate(s) removed\n";
}

void InteractiveShell::cmdShow(const std::vector<std::string>& args) {
    if (!hasDesign) { std::cerr << "No design loaded. Run read_verilog first.\n"; return; }

    std::string dotFile = "output\\temp_show.dot";
    std::string pngFile = "output\\temp_show.png";
    if (!args.empty()) {
        dotFile = args[0];
        pngFile = args[0] + ".png";
    }

    MyDesign des;
    des.My_show_netlist(currentDesign, dotFile);
    std::cout << "DOT written to: " << dotFile << "\n";

    bool ok = VerilogDotConverter::convertDotToPng(dotFile, pngFile);
    if (ok) {
        VerilogDotConverter::openPngFile(pngFile);
        std::cout << "Circuit diagram opened: " << pngFile << "\n";
    } else {
        std::cout << "Tip: open '" << dotFile << "' in a Graphviz viewer manually.\n";
        std::cout << "     Or install Graphviz: https://graphviz.org/download/\n";
    }
}

void InteractiveShell::cmdStats(const std::vector<std::string>& args) {
    if (!hasDesign) { std::cerr << "No design loaded. Run read_verilog first.\n"; return; }

    for (auto& kv : currentDesign.m_module_map) {
        if (!kv.second) continue;
        auto* mod = kv.second;
        int inputs = 0, outputs = 0, middle = 0;
        for (auto& w : mod->wires) {
            switch (w.second->wiretype) {
                case PWire::INPORT:  ++inputs;  break;
                case PWire::OUTPORT: ++outputs; break;
                case PWire::MIDDLE:  ++middle;  break;
            }
        }
        std::cout << "\n=== Module: " << mod->modname << " ===\n";
        std::cout << "  Wires : " << mod->wires.size()
                  << "  (in=" << inputs << " out=" << outputs << " wire=" << middle << ")\n";
        std::cout << "  Gates : " << mod->gates.size() << "\n";
        std::cout << "  Assign statements:\n";
        for (const auto& g : mod->gate_insert_order)
            std::cout << "    assign " << g << "\n";
    }
}

void InteractiveShell::cmdHelp(const std::vector<std::string>& args) {
    std::cout << "\nAvailable commands:\n";
    std::cout << "  read_verilog <file.v>      Parse a Verilog source file\n";
    std::cout << "  write_rtlil  <file.il>     Write RTLIL output (Yosys-compatible)\n";
    std::cout << "  write_dot    <file.dot>    Write Graphviz DOT netlist\n";
    std::cout << "  write_blif   <file.blif>   Write BLIF output\n";
    std::cout << "  show         [file.dot]    Generate and open PNG circuit diagram\n";
    std::cout << "  opt_share                  Shared sub-expression elimination (IR level)\n";
    std::cout << "  opt_const                  Constant propagation\n";
    std::cout << "  opt_clean                  Dead-code elimination\n";
    std::cout << "  stats                      Print design statistics\n";
    std::cout << "  web                        Launch web UI (backend + frontend + browser)\n";
    std::cout << "  help  / ?                  Show this help\n";
    std::cout << "  exit  / quit / q           Quit\n";
    std::cout << "\nExample session:\n";
    std::cout << "  yosys> read_verilog input/carry.v\n";
    std::cout << "  yosys> write_rtlil output/carry.il\n";
    std::cout << "  yosys> opt_share\n";
    std::cout << "  yosys> write_rtlil output/carry_opt.il\n";
    std::cout << "  yosys> show\n";
    std::cout << "  yosys> exit\n\n";
}

void InteractiveShell::cmdExit(const std::vector<std::string>& args) {
    std::cout << "Bye!\n";
    exit(0);
}

// ---------- Helper: directory existence check ----------

bool InteractiveShell::dirExists(const std::string& p) {
    struct _stat st;
    return _stat(p.c_str(), &st) == 0 && (st.st_mode & _S_IFDIR);
}

// ---------- web: launch backend + frontend, open browser ----------

void InteractiveShell::cmdWeb(const std::vector<std::string>& args) {
    // Verify that frontend/ and backend/ sub-directories exist relative to CWD.
    // (Expected CWD is main/ -- where Main.exe lives and where Main.sln is.)
    if (!dirExists("frontend") || !dirExists("backend")) {
        std::cerr << "Error: 'frontend' or 'backend' directory not found in the current\n"
                  << "       working directory.  Run Main.exe from the 'main/' folder.\n";
        return;
    }

    std::cout << "Starting backend  server (http://localhost:3001) ...\n";
    // /D sets the working directory for the spawned window; the window title
    // makes it easy to identify in the taskbar.
    system("start \"VerilogCompiler Backend\" /D backend node server.js");

    std::cout << "Starting frontend server (http://localhost:5173) ...\n";
    system("start \"VerilogCompiler Frontend\" /D frontend npm run dev");

    // Give Vite time to compile + start before opening the browser.
    std::cout << "Waiting for servers to initialise (3 s) ...\n";
    Sleep(3000);

    std::cout << "Opening browser ...\n";
    system("start http://localhost:5173");

    std::cout << "\nWeb UI is available at:\n"
              << "  Frontend : http://localhost:5173\n"
              << "  Backend  : http://localhost:3001\n"
              << "Close the two server windows to stop the web servers.\n";
}
