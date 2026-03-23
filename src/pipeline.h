#pragma once
#include "parser.h"
#include <string>
#include <functional>
#include <unordered_map>
#include <windows.h>

class Shell;

using BuiltinFunc = std::function<int(Shell& shell, const std::vector<std::string>& args)>;

class PipelineEngine {
public:
    PipelineEngine();

    void registerBuiltin(const std::string& name, BuiltinFunc func);
    bool isBuiltin(const std::string& name) const;

    void registerAlias(const std::string& alias, const std::string& target);
    std::string resolveAlias(const std::string& name) const;

    int execute(Shell& shell, const CommandLine& cmdLine);

private:
    std::unordered_map<std::string, BuiltinFunc> builtins_;
    std::unordered_map<std::string, std::string> aliases_;

    int executePipeline(Shell& shell, const Pipeline& pipeline);
    int executeCommand(Shell& shell, const Command& cmd,
                       HANDLE hStdin, HANDLE hStdout);
    int executeExternal(const Command& cmd, HANDLE hStdin, HANDLE hStdout);
    HANDLE startExternal(const Command& cmd, HANDLE hStdin, HANDLE hStdout);

    static std::string normalize(const std::string& name);
};
