#pragma once
#include <string>
#include <vector>

class Shell;

class ScriptRunner {
public:
    // .dssh Script ausfuehren
    // args[0] = Scriptname, args[1..] = Parameter
    static int execute(Shell& shell, const std::string& path,
                       const std::vector<std::string>& args);

    // .dssh Datei suchen: erst exakt, dann mit .dssh-Endung, dann in .dosshell/
    // Gibt leeren String zurueck wenn nicht gefunden
    static std::string findScript(const std::string& name);
};
