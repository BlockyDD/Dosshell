#pragma once
#include "parser.h"
#include <string>
#include <functional>
#include <unordered_map>
#include <windows.h>

class Shell;

using BuiltinFunc = std::function<int(Shell& shell, const std::vector<std::string>& args)>;

// Ein Script-Befehl: mehrere Zeilen Dosshell-Code mit %1 %2 ... Parametern
struct ScriptCommand {
    std::string name;
    std::string description;
    std::vector<std::string> lines;
};

class PipelineEngine {
public:
    PipelineEngine();

    void registerBuiltin(const std::string& name, BuiltinFunc func);
    bool isBuiltin(const std::string& name) const;

    void registerScriptCommand(const ScriptCommand& cmd);
    bool isScriptCommand(const std::string& name) const;
    int executeScriptCommand(Shell& shell, const std::string& name,
                             const std::vector<std::string>& args);

    void registerAlias(const std::string& alias, const std::string& target);
    std::string resolveAlias(const std::string& name) const;

    int execute(Shell& shell, const CommandLine& cmdLine);

    const std::unordered_map<std::string, ScriptCommand>& scriptCommands() const { return scripts_; }

private:
    std::unordered_map<std::string, BuiltinFunc> builtins_;
    std::unordered_map<std::string, std::string> aliases_;
    std::unordered_map<std::string, ScriptCommand> scripts_;

    int executePipeline(Shell& shell, const Pipeline& pipeline);
    int executeCommand(Shell& shell, const Command& cmd,
                       HANDLE hStdin, HANDLE hStdout);
    int executeExternal(const Command& cmd, HANDLE hStdin, HANDLE hStdout);
    HANDLE startExternal(const Command& cmd, HANDLE hStdin, HANDLE hStdout);

    static std::string normalize(const std::string& name);
};
