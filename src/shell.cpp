#include "shell.h"
#include "builtins.h"
#include "script.h"
#include <filesystem>
#include <chrono>
#include <ctime>

namespace fs = std::filesystem;

Shell::Shell() {
    registerAllBuiltins(engine_);
    console_.setTitle("Dosshell");
}

void Shell::showBanner() {
    console_.writeLine("");
    console_.writeColored("  ____                 _          _ _ ", Color::Cyan);
    console_.writeLine("");
    console_.writeColored(" |  _ \\  ___  ___ ___| |__   ___| | |", Color::Cyan);
    console_.writeLine("");
    console_.writeColored(" | | | |/ _ \\/ __/ __| '_ \\ / _ \\ | |", Color::Cyan);
    console_.writeLine("");
    console_.writeColored(" | |_| | (_) \\__ \\__ \\ | | |  __/ | |", Color::Cyan);
    console_.writeLine("");
    console_.writeColored(" |____/ \\___/|___/___/_| |_|\\___|_|_|", Color::Cyan);
    console_.writeLine("");
    console_.writeLine("");
    console_.writeColored("  Dosshell v0.0.1", Color::White);
    console_.write(" - Eine moderne Shell mit DOS-Feeling\n");
    console_.writeColored("  Tippe 'help' fuer verfuegbare Befehle.\n", Color::DarkGray);
    console_.writeLine("");
}

std::string Shell::getCurrentDir() const {
    return fs::current_path().string();
}

std::string Shell::getPrompt() const {
    // Erst Klartext aus Format bauen
    std::string text;
    size_t i = 0;

    while (i < promptFormat_.size()) {
        if (promptFormat_[i] == '$' && i + 1 < promptFormat_.size()) {
            char code = promptFormat_[i + 1];
            switch (code) {
                case 'P': case 'p':
                    text += getCurrentDir();
                    break;
                case 'G': case 'g':
                    text += ">";
                    break;
                case 'L': case 'l':
                    text += "<";
                    break;
                case 'N': case 'n':
                    text += getCurrentDir().substr(0, 2);
                    break;
                case 'D': case 'd': {
                    auto now = std::chrono::system_clock::now();
                    auto time = std::chrono::system_clock::to_time_t(now);
                    struct tm tm_buf;
                    localtime_s(&tm_buf, &time);
                    char buf[20];
                    std::strftime(buf, sizeof(buf), "%d.%m.%Y", &tm_buf);
                    text += buf;
                    break;
                }
                case 'T': case 't': {
                    auto now = std::chrono::system_clock::now();
                    auto time = std::chrono::system_clock::to_time_t(now);
                    struct tm tm_buf;
                    localtime_s(&tm_buf, &time);
                    char buf[20];
                    std::strftime(buf, sizeof(buf), "%H:%M:%S", &tm_buf);
                    text += buf;
                    break;
                }
                case '$':
                    text += "$";
                    break;
                case '_':
                    text += "\n";
                    break;
                case 'E': case 'e':
                    text += std::to_string(lastExitCode_);
                    break;
                default:
                    text += "$";
                    text += code;
                    break;
            }
            i += 2;
        } else {
            text += promptFormat_[i];
            i++;
        }
    }

    // Farben: Pfad gelb, > weiss, dann reset
    auto gtPos = text.rfind('>');
    if (gtPos != std::string::npos) {
        return "\x1b[33m" + text.substr(0, gtPos) + "\x1b[97m>\x1b[0m";
    }
    return "\x1b[33m" + text + "\x1b[0m";
}

void Shell::quit(int exitCode) {
    running_ = false;
    lastExitCode_ = exitCode;
}

int Shell::runScript(const std::string& path, const std::vector<std::string>& args) {
    return ScriptRunner::execute(*this, path, args);
}

int Shell::run() {
    showBanner();

    while (running_) {
        std::string prompt = getPrompt();
        std::string line = console_.readLine(prompt);

        // Trim
        auto start = line.find_first_not_of(" \t");
        if (start == std::string::npos) continue;
        line = line.substr(start);

        // Parsen und ausfuehren
        auto cmdLine = Parser::parse(line);
        lastExitCode_ = engine_.execute(*this, cmdLine);
    }

    return lastExitCode_;
}
