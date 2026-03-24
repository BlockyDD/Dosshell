#include "script.h"
#include "shell.h"
#include "parser.h"
#include <fstream>
#include <filesystem>
#include <iostream>
#include <algorithm>

namespace fs = std::filesystem;

static std::string getDosshellDir() {
    const char* home = std::getenv("USERPROFILE");
    if (!home) home = std::getenv("HOME");
    if (!home) return "";
    return std::string(home) + "\\.dosshell";
}

std::string ScriptRunner::findScript(const std::string& name) {
    // 1. Exakter Pfad mit .dssh Endung
    if (fs::exists(name)) {
        fs::path p(name);
        if (p.extension() == ".dssh")
            return fs::absolute(p).string();
    }

    // 2. Name + .dssh Endung
    std::string withExt = name + ".dssh";
    if (fs::exists(withExt))
        return fs::absolute(withExt).string();

    // 3. Im .dosshell Verzeichnis suchen
    std::string dir = getDosshellDir();
    if (!dir.empty()) {
        fs::path inDir = fs::path(dir) / (name + ".dssh");
        if (fs::exists(inDir))
            return inDir.string();

        // Auch ohne Endung pruefen (falls name schon .dssh hat)
        fs::path inDirExact = fs::path(dir) / name;
        if (fs::exists(inDirExact) && inDirExact.extension() == ".dssh")
            return inDirExact.string();
    }

    return "";
}

static std::string expandScriptParams(const std::string& line,
                                       const std::string& scriptName,
                                       const std::vector<std::string>& args) {
    std::string result = line;

    // %* -> alle Argumente (ab args[1])
    std::string allArgs;
    for (size_t i = 1; i < args.size(); i++) {
        if (i > 1) allArgs += " ";
        allArgs += args[i];
    }
    size_t pos;
    while ((pos = result.find("%*")) != std::string::npos) {
        result.replace(pos, 2, allArgs);
    }

    // %0-%9 ersetzen (rueckwaerts damit %10 nicht als %1 + "0" geparst wird)
    for (int p = 9; p >= 0; p--) {
        std::string placeholder = "%" + std::to_string(p);
        std::string value;
        if (p == 0) {
            value = scriptName;
        } else if (static_cast<size_t>(p) < args.size()) {
            value = args[p];
        }
        while ((pos = result.find(placeholder)) != std::string::npos) {
            result.replace(pos, placeholder.size(), value);
        }
    }

    return result;
}

int ScriptRunner::execute(Shell& shell, const std::string& path,
                          const std::vector<std::string>& args) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Dosshell: Kann Script '" << path << "' nicht oeffnen\n";
        return 1;
    }

    std::string scriptName = fs::path(path).stem().string();
    bool echoOn = true;
    int lastResult = 0;
    std::string line;

    // Alle Zeilen einlesen (fuer goto-Support spaeter)
    std::vector<std::string> lines;
    while (std::getline(file, line)) {
        // Trailing CR/LF entfernen
        while (!line.empty() && (line.back() == '\r' || line.back() == '\n'))
            line.pop_back();
        lines.push_back(line);
    }
    file.close();

    for (size_t lineIdx = 0; lineIdx < lines.size(); lineIdx++) {
        if (!shell.isRunning()) break;

        line = lines[lineIdx];

        // Trim leading whitespace
        auto start = line.find_first_not_of(" \t");
        if (start == std::string::npos) continue;
        line = line.substr(start);

        // Trim trailing whitespace
        auto end = line.find_last_not_of(" \t");
        if (end != std::string::npos) line = line.substr(0, end + 1);

        if (line.empty()) continue;

        // Kommentare: REM, #, ;
        if (line[0] == '#' || line[0] == ';') continue;
        {
            std::string lower = line;
            std::transform(lower.begin(), lower.end(), lower.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            if (lower == "rem" || (lower.size() > 3 && lower.substr(0, 4) == "rem "))
                continue;
        }

        // @-Prefix: Echo fuer diese Zeile unterdruecken
        bool suppressEcho = false;
        if (line[0] == '@') {
            suppressEcho = true;
            line = line.substr(1);
            start = line.find_first_not_of(" \t");
            if (start == std::string::npos) continue;
            line = line.substr(start);
        }

        // echo off / echo on Direktive erkennen
        {
            std::string lower = line;
            std::transform(lower.begin(), lower.end(), lower.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            if (lower == "echo off") {
                echoOn = false;
                continue;
            }
            if (lower == "echo on") {
                echoOn = true;
                continue;
            }
        }

        // exit /b — nur Script beenden, nicht die Shell
        {
            std::string lower = line;
            std::transform(lower.begin(), lower.end(), lower.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            if (lower == "exit /b" || lower.substr(0, 8) == "exit /b ") {
                // Optional: Exit-Code nach /b
                if (lower.size() > 8) {
                    try {
                        lastResult = std::stoi(line.substr(8));
                    } catch (...) {}
                }
                shell.setLastExitCode(lastResult);
                return lastResult;
            }
        }

        // Parameter expandieren
        line = expandScriptParams(line, scriptName, args);

        // Echo ausgeben wenn aktiv und nicht unterdrueckt
        if (echoOn && !suppressEcho) {
            shell.console().writeColored("> ", Color::DarkGray);
            shell.console().writeColored(line + "\n", Color::DarkGray);
        }

        // Zeile ausfuehren
        auto cmdLine = Parser::parse(line);
        lastResult = shell.engine().execute(shell, cmdLine);
        shell.setLastExitCode(lastResult);
    }

    return lastResult;
}
