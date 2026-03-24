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

    void setGoto(const std::string& label) { gotoLabel_ = label; }
    bool hasGoto() const { return !gotoLabel_.empty(); }
    std::string consumeGoto() { std::string l = gotoLabel_; gotoLabel_.clear(); return l; }

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
    std::string gotoLabel_;

    void showBanner();
    void autoRegisterDssh();
};
