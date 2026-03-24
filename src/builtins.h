#pragma once
#include "pipeline.h"

class Shell;

void registerAllBuiltins(PipelineEngine& engine);

namespace Builtins {
    // Dateisystem
    int cmd_ls(Shell& shell, const std::vector<std::string>& args);
    int cmd_cd(Shell& shell, const std::vector<std::string>& args);
    int cmd_cat(Shell& shell, const std::vector<std::string>& args);
    int cmd_cp(Shell& shell, const std::vector<std::string>& args);
    int cmd_mv(Shell& shell, const std::vector<std::string>& args);
    int cmd_rm(Shell& shell, const std::vector<std::string>& args);
    int cmd_mk(Shell& shell, const std::vector<std::string>& args);

    // Ein-/Ausgabe
    int cmd_echo(Shell& shell, const std::vector<std::string>& args);
    int cmd_input(Shell& shell, const std::vector<std::string>& args);
    int cmd_clear(Shell& shell, const std::vector<std::string>& args);
    int cmd_pause(Shell& shell, const std::vector<std::string>& args);

    // Variablen & Mathe
    int cmd_set(Shell& shell, const std::vector<std::string>& args);
    int cmd_math(Shell& shell, const std::vector<std::string>& args);

    // Shell
    int cmd_help(Shell& shell, const std::vector<std::string>& args);
    int cmd_exit(Shell& shell, const std::vector<std::string>& args);
    int cmd_color(Shell& shell, const std::vector<std::string>& args);
    int cmd_title(Shell& shell, const std::vector<std::string>& args);
    int cmd_prompt(Shell& shell, const std::vector<std::string>& args);
    int cmd_alias(Shell& shell, const std::vector<std::string>& args);
    int cmd_time(Shell& shell, const std::vector<std::string>& args);
    int cmd_date(Shell& shell, const std::vector<std::string>& args);

    // Kontrollfluss
    int cmd_if(Shell& shell, const std::vector<std::string>& args);
    int cmd_goto(Shell& shell, const std::vector<std::string>& args);

    // Scripting & Erweiterungen
    int cmd_call(Shell& shell, const std::vector<std::string>& args);
    int cmd_load(Shell& shell, const std::vector<std::string>& args);
    int cmd_plugin(Shell& shell, const std::vector<std::string>& args);
}
