#pragma once
#include "pipeline.h"

class Shell;

void registerAllBuiltins(PipelineEngine& engine);

namespace Builtins {
    int cmd_dir(Shell& shell, const std::vector<std::string>& args);
    int cmd_cd(Shell& shell, const std::vector<std::string>& args);
    int cmd_cls(Shell& shell, const std::vector<std::string>& args);
    int cmd_echo(Shell& shell, const std::vector<std::string>& args);
    int cmd_type(Shell& shell, const std::vector<std::string>& args);
    int cmd_copy(Shell& shell, const std::vector<std::string>& args);
    int cmd_del(Shell& shell, const std::vector<std::string>& args);
    int cmd_ren(Shell& shell, const std::vector<std::string>& args);
    int cmd_md(Shell& shell, const std::vector<std::string>& args);
    int cmd_rd(Shell& shell, const std::vector<std::string>& args);
    int cmd_set(Shell& shell, const std::vector<std::string>& args);
    int cmd_ver(Shell& shell, const std::vector<std::string>& args);
    int cmd_help(Shell& shell, const std::vector<std::string>& args);
    int cmd_exit(Shell& shell, const std::vector<std::string>& args);
    int cmd_color(Shell& shell, const std::vector<std::string>& args);
    int cmd_title(Shell& shell, const std::vector<std::string>& args);
    int cmd_prompt(Shell& shell, const std::vector<std::string>& args);
    int cmd_alias(Shell& shell, const std::vector<std::string>& args);
    int cmd_pwd(Shell& shell, const std::vector<std::string>& args);
    int cmd_time(Shell& shell, const std::vector<std::string>& args);
    int cmd_date(Shell& shell, const std::vector<std::string>& args);
    int cmd_load(Shell& shell, const std::vector<std::string>& args);
    int cmd_plugin(Shell& shell, const std::vector<std::string>& args);
}
