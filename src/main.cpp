#include "shell.h"
#include "script.h"
#include "parser.h"
#include <string>
#include <vector>

int main(int argc, char* argv[]) {
    Shell shell;

    // dosshell script.dssh arg1 arg2 ...
    if (argc > 1) {
        std::string scriptPath = ScriptRunner::findScript(argv[1]);
        if (!scriptPath.empty()) {
            std::vector<std::string> args;
            for (int i = 1; i < argc; i++)
                args.push_back(argv[i]);
            return shell.runScript(scriptPath, args);
        }
        // Kein Script gefunden: als einzelnen Befehl ausfuehren
        std::string cmdStr;
        for (int i = 1; i < argc; i++) {
            if (i > 1) cmdStr += " ";
            cmdStr += argv[i];
        }
        auto cmdLine = Parser::parse(cmdStr);
        return shell.engine().execute(shell, cmdLine);
    }

    return shell.run();
}
