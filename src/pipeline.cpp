#include "pipeline.h"
#include "shell.h"
#include <algorithm>
#include <cctype>
#include <iostream>
#include <io.h>
#include <fcntl.h>

PipelineEngine::PipelineEngine() {}

void PipelineEngine::registerBuiltin(const std::string& name, BuiltinFunc func) {
    builtins_[normalize(name)] = std::move(func);
}

bool PipelineEngine::isBuiltin(const std::string& name) const {
    return builtins_.count(normalize(name)) > 0;
}

void PipelineEngine::registerAlias(const std::string& alias, const std::string& target) {
    aliases_[normalize(alias)] = target;
}

std::string PipelineEngine::resolveAlias(const std::string& name) const {
    auto it = aliases_.find(normalize(name));
    if (it != aliases_.end()) return it->second;
    return name;
}

void PipelineEngine::registerScriptCommand(const ScriptCommand& cmd) {
    scripts_[normalize(cmd.name)] = cmd;
}

bool PipelineEngine::isScriptCommand(const std::string& name) const {
    return scripts_.count(normalize(name)) > 0;
}

int PipelineEngine::executeScriptCommand(Shell& shell, const std::string& name,
                                          const std::vector<std::string>& args) {
    auto it = scripts_.find(normalize(name));
    if (it == scripts_.end()) return 1;

    const auto& script = it->second;
    int lastResult = 0;

    for (const auto& line : script.lines) {
        // Parameter ersetzen: %0=name, %1=erstes Arg, %2=zweites, ...
        // %*=alle Argumente
        std::string expanded = line;

        // %* durch alle Argumente ersetzen
        std::string allArgs;
        for (size_t i = 1; i < args.size(); i++) {
            if (i > 1) allArgs += " ";
            allArgs += args[i];
        }
        size_t pos;
        while ((pos = expanded.find("%*")) != std::string::npos) {
            expanded.replace(pos, 2, allArgs);
        }

        // %0-%9 ersetzen
        for (int p = 9; p >= 0; p--) {
            std::string placeholder = "%" + std::to_string(p);
            std::string value;
            if (p == 0) {
                value = name;
            } else if ((size_t)p < args.size()) {
                value = args[p];
            }
            while ((pos = expanded.find(placeholder)) != std::string::npos) {
                expanded.replace(pos, placeholder.size(), value);
            }
        }

        auto cmdLine = Parser::parse(expanded);
        lastResult = execute(shell, cmdLine);
    }

    return lastResult;
}

std::string PipelineEngine::normalize(const std::string& name) {
    std::string lower = name;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c) { return (char)std::tolower(c); });
    return lower;
}

int PipelineEngine::execute(Shell& shell, const CommandLine& cmdLine) {
    int exitCode = 0;

    for (size_t i = 0; i < cmdLine.entries.size(); i++) {
        const auto& entry = cmdLine.entries[i];

        if (i > 0 && cmdLine.entries[i - 1].conditionalAnd && exitCode != 0)
            continue;

        exitCode = executePipeline(shell, entry.pipeline);
        shell.setLastExitCode(exitCode);
    }

    return exitCode;
}

int PipelineEngine::executePipeline(Shell& shell, const Pipeline& pipeline) {
    if (pipeline.commands.empty()) return 0;

    // Einzelner Befehl: einfach ausfuehren
    if (pipeline.commands.size() == 1) {
        return executeCommand(shell, pipeline.commands[0],
                            GetStdHandle(STD_INPUT_HANDLE),
                            GetStdHandle(STD_OUTPUT_HANDLE));
    }

    // Mehrere Befehle: Pipes aufbauen, alle Prozesse parallel starten
    size_t n = pipeline.commands.size();

    // Pipes erstellen
    std::vector<HANDLE> pipeRead(n - 1), pipeWrite(n - 1);
    SECURITY_ATTRIBUTES sa = {};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;

    for (size_t i = 0; i < n - 1; i++) {
        if (!CreatePipe(&pipeRead[i], &pipeWrite[i], &sa, 0)) {
            std::cerr << "Dosshell: Pipe-Fehler\n";
            return 1;
        }
    }

    // Alle Commands starten
    std::vector<HANDLE> processes;

    for (size_t i = 0; i < n; i++) {
        HANDLE hIn = (i == 0) ? GetStdHandle(STD_INPUT_HANDLE) : pipeRead[i - 1];
        HANDLE hOut = (i == n - 1) ? GetStdHandle(STD_OUTPUT_HANDLE) : pipeWrite[i];

        const Command& cmd = pipeline.commands[i];
        std::string resolvedName = resolveAlias(cmd.name);
        std::string normalizedName = normalize(resolvedName);

        if (builtins_.count(normalizedName)) {
            // Built-in: sofort ausfuehren
            executeCommand(shell, cmd, hIn, hOut);
        } else {
            // Extern: Prozess starten ohne zu warten
            Command resolvedCmd = cmd;
            resolvedCmd.name = resolvedName;
            HANDLE hProcess = startExternal(resolvedCmd, hIn, hOut);
            if (hProcess != INVALID_HANDLE_VALUE) {
                processes.push_back(hProcess);
            }
        }

        // Pipe-Handles im Parent schliessen (gehoeren jetzt dem Child)
        if (i < n - 1) CloseHandle(pipeWrite[i]);
        if (i > 0) CloseHandle(pipeRead[i - 1]);
    }

    // Auf alle externen Prozesse warten
    int exitCode = 0;
    if (!processes.empty()) {
        WaitForMultipleObjects((DWORD)processes.size(), processes.data(), TRUE, INFINITE);
        DWORD code;
        GetExitCodeProcess(processes.back(), &code);
        exitCode = (int)code;
        for (HANDLE h : processes) CloseHandle(h);
    }

    return exitCode;
}

int PipelineEngine::executeCommand(Shell& shell, const Command& cmd,
                                    HANDLE hStdin, HANDLE hStdout) {
    std::string resolvedName = resolveAlias(cmd.name);
    std::string normalizedName = normalize(resolvedName);

    // Redirection-Handles
    HANDLE hFileIn = INVALID_HANDLE_VALUE;
    HANDLE hFileOut = INVALID_HANDLE_VALUE;

    if (!cmd.redirectIn.empty()) {
        hFileIn = CreateFileA(cmd.redirectIn.c_str(), GENERIC_READ, FILE_SHARE_READ,
                              nullptr, OPEN_EXISTING, 0, nullptr);
        if (hFileIn == INVALID_HANDLE_VALUE) {
            std::cerr << "Dosshell: Kann '" << cmd.redirectIn << "' nicht oeffnen\n";
            return 1;
        }
        hStdin = hFileIn;
    }

    if (!cmd.redirectOut.empty()) {
        DWORD disposition = cmd.appendOut ? OPEN_ALWAYS : CREATE_ALWAYS;
        hFileOut = CreateFileA(cmd.redirectOut.c_str(), GENERIC_WRITE, 0,
                               nullptr, disposition, 0, nullptr);
        if (hFileOut == INVALID_HANDLE_VALUE) {
            std::cerr << "Dosshell: Kann '" << cmd.redirectOut << "' nicht erstellen\n";
            if (hFileIn != INVALID_HANDLE_VALUE) CloseHandle(hFileIn);
            return 1;
        }
        if (cmd.appendOut) SetFilePointer(hFileOut, 0, nullptr, FILE_END);
        hStdout = hFileOut;
    }

    int result;

    auto it = builtins_.find(normalizedName);
    if (it != builtins_.end()) {
        // Built-in: stdout umleiten wenn noetig
        HANDLE origStdout = GetStdHandle(STD_OUTPUT_HANDLE);
        int origFd = -1;

        if (hStdout != origStdout) {
            SetStdHandle(STD_OUTPUT_HANDLE, hStdout);
            origFd = _dup(_fileno(stdout));
            int fd = _open_osfhandle((intptr_t)hStdout, _O_WRONLY);
            if (fd >= 0) {
                _dup2(fd, _fileno(stdout));
                _close(fd);
            }
        }

        std::vector<std::string> fullArgs;
        fullArgs.push_back(resolvedName);
        fullArgs.insert(fullArgs.end(), cmd.args.begin(), cmd.args.end());
        result = it->second(shell, fullArgs);
        std::cout.flush();

        if (origFd >= 0) {
            _dup2(origFd, _fileno(stdout));
            _close(origFd);
            SetStdHandle(STD_OUTPUT_HANDLE, origStdout);
        }
    } else if (scripts_.count(normalizedName)) {
        // Script-Befehl aus .ini
        std::vector<std::string> fullArgs;
        fullArgs.push_back(resolvedName);
        fullArgs.insert(fullArgs.end(), cmd.args.begin(), cmd.args.end());
        result = executeScriptCommand(shell, normalizedName, fullArgs);
    } else {
        Command resolvedCmd = cmd;
        resolvedCmd.name = resolvedName;
        result = executeExternal(resolvedCmd, hStdin, hStdout);
    }

    if (hFileIn != INVALID_HANDLE_VALUE) CloseHandle(hFileIn);
    if (hFileOut != INVALID_HANDLE_VALUE) CloseHandle(hFileOut);

    return result;
}

HANDLE PipelineEngine::startExternal(const Command& cmd, HANDLE hStdin, HANDLE hStdout) {
    std::string cmdLine = cmd.name;
    for (const auto& arg : cmd.args) {
        cmdLine += " ";
        if (arg.find(' ') != std::string::npos)
            cmdLine += "\"" + arg + "\"";
        else
            cmdLine += arg;
    }

    STARTUPINFOA si = {};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = hStdin;
    si.hStdOutput = hStdout;
    si.hStdError = GetStdHandle(STD_ERROR_HANDLE);

    PROCESS_INFORMATION pi = {};

    SetHandleInformation(hStdin, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);
    SetHandleInformation(hStdout, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);

    std::vector<char> cmdBuf(cmdLine.begin(), cmdLine.end());
    cmdBuf.push_back('\0');

    if (!CreateProcessA(nullptr, cmdBuf.data(), nullptr, nullptr, TRUE, 0,
                         nullptr, nullptr, &si, &pi)) {
        std::cerr << "Dosshell: '" << cmd.name << "' nicht gefunden oder Fehler "
                  << GetLastError() << "\n";
        return INVALID_HANDLE_VALUE;
    }

    CloseHandle(pi.hThread);
    return pi.hProcess;
}

int PipelineEngine::executeExternal(const Command& cmd, HANDLE hStdin, HANDLE hStdout) {
    HANDLE hProcess = startExternal(cmd, hStdin, hStdout);
    if (hProcess == INVALID_HANDLE_VALUE) return 1;

    WaitForSingleObject(hProcess, INFINITE);
    DWORD exitCode;
    GetExitCodeProcess(hProcess, &exitCode);
    CloseHandle(hProcess);
    return (int)exitCode;
}
