#pragma once
#include "console.h"
#include "parser.h"
#include "pipeline.h"
#include <string>

class Shell {
public:
    Shell();

    int run();
    int runScript(const std::string& path, const std::vector<std::string>& args);

    Console& console() { return console_; }
    PipelineEngine& engine() { return engine_; }

    bool isRunning() const { return running_; }
    void quit(int exitCode = 0);
    int lastExitCode() const { return lastExitCode_; }
    void setLastExitCode(int code) { lastExitCode_ = code; }

    std::string getPrompt() const;
    void setPromptFormat(const std::string& fmt) { promptFormat_ = fmt; }
    const std::string& getPromptFormat() const { return promptFormat_; }

    std::string getCurrentDir() const;

private:
    Console console_;
    PipelineEngine engine_;

    bool running_ = true;
    int lastExitCode_ = 0;
    std::string promptFormat_ = "$P$G";

    void showBanner();
};
