#include "builtins.h"
#include "shell.h"
#include <windows.h>
#include <aclapi.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <iomanip>
#include <ctime>
#include <chrono>

namespace fs = std::filesystem;

void registerAllBuiltins(PipelineEngine& engine) {
    engine.registerBuiltin("dir",     Builtins::cmd_dir);
    engine.registerBuiltin("ls",      Builtins::cmd_dir);
    engine.registerBuiltin("cd",      Builtins::cmd_cd);
    engine.registerBuiltin("chdir",   Builtins::cmd_cd);
    engine.registerBuiltin("cls",     Builtins::cmd_cls);
    engine.registerBuiltin("clear",   Builtins::cmd_cls);
    engine.registerBuiltin("echo",    Builtins::cmd_echo);
    engine.registerBuiltin("type",    Builtins::cmd_type);
    engine.registerBuiltin("cat",     Builtins::cmd_type);
    engine.registerBuiltin("copy",    Builtins::cmd_copy);
    engine.registerBuiltin("cp",      Builtins::cmd_copy);
    engine.registerBuiltin("del",     Builtins::cmd_del);
    engine.registerBuiltin("erase",   Builtins::cmd_del);
    engine.registerBuiltin("rm",      Builtins::cmd_del);
    engine.registerBuiltin("ren",     Builtins::cmd_ren);
    engine.registerBuiltin("rename",  Builtins::cmd_ren);
    engine.registerBuiltin("mv",      Builtins::cmd_ren);
    engine.registerBuiltin("md",      Builtins::cmd_md);
    engine.registerBuiltin("mkdir",   Builtins::cmd_md);
    engine.registerBuiltin("rd",      Builtins::cmd_rd);
    engine.registerBuiltin("rmdir",   Builtins::cmd_rd);
    engine.registerBuiltin("set",     Builtins::cmd_set);
    engine.registerBuiltin("ver",     Builtins::cmd_ver);
    engine.registerBuiltin("help",    Builtins::cmd_help);
    engine.registerBuiltin("exit",    Builtins::cmd_exit);
    engine.registerBuiltin("quit",    Builtins::cmd_exit);
    engine.registerBuiltin("color",   Builtins::cmd_color);
    engine.registerBuiltin("title",   Builtins::cmd_title);
    engine.registerBuiltin("prompt",  Builtins::cmd_prompt);
    engine.registerBuiltin("alias",   Builtins::cmd_alias);
    engine.registerBuiltin("pwd",     Builtins::cmd_pwd);
    engine.registerBuiltin("time",    Builtins::cmd_time);
    engine.registerBuiltin("date",    Builtins::cmd_date);
    engine.registerBuiltin("load",    Builtins::cmd_load);
}

// ============================================================
// dir
// ============================================================
int Builtins::cmd_dir(Shell&, const std::vector<std::string>& args) {
    std::string path = ".";
    bool showAll = false;
    bool wideFormat = false;

    for (size_t i = 1; i < args.size(); i++) {
        if (args[i] == "/a" || args[i] == "-a") showAll = true;
        else if (args[i] == "/w" || args[i] == "-w") wideFormat = true;
        else path = args[i];
    }

    try {
        fs::path dirPath = fs::absolute(path);
        if (!fs::exists(dirPath)) {
            std::cerr << "Dosshell: Verzeichnis nicht gefunden: " << path << "\n";
            return 1;
        }

        std::cout << "\n Verzeichnis von " << fs::canonical(dirPath).string() << "\n\n";

        size_t fileCount = 0, dirCount = 0;
        uintmax_t totalSize = 0;
        int col = 0;

        for (const auto& entry : fs::directory_iterator(dirPath)) {
            std::string name = entry.path().filename().string();
            if (!showAll && !name.empty() && name[0] == '.') continue;

            if (wideFormat) {
                if (entry.is_directory()) {
                    std::cout << "[" << std::left << std::setw(18) << name << "] ";
                    dirCount++;
                } else {
                    std::cout << std::left << std::setw(20) << name << " ";
                    fileCount++;
                    totalSize += entry.file_size();
                }
                col++;
                if (col >= 4) { std::cout << "\n"; col = 0; }
            } else {
                auto ftime = fs::last_write_time(entry.path());
                auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                    ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now());
                auto time = std::chrono::system_clock::to_time_t(sctp);
                struct tm tm_buf;
                localtime_s(&tm_buf, &time);
                char timeBuf[20];
                std::strftime(timeBuf, sizeof(timeBuf), "%d.%m.%Y  %H:%M", &tm_buf);
                std::cout << timeBuf;

                if (entry.is_directory()) {
                    std::cout << "    <DIR>          " << name << "\n";
                    dirCount++;
                } else {
                    auto size = entry.file_size();
                    totalSize += size;
                    std::ostringstream sizeStr;
                    sizeStr << size;
                    std::string s = sizeStr.str();
                    std::string formatted;
                    int count = 0;
                    for (int j = (int)s.size() - 1; j >= 0; j--) {
                        if (count > 0 && count % 3 == 0) formatted = "." + formatted;
                        formatted = s[j] + formatted;
                        count++;
                    }
                    std::cout << std::right << std::setw(17) << formatted << " " << name << "\n";
                    fileCount++;
                }
            }
        }

        if (wideFormat && col > 0) std::cout << "\n";
        std::cout << std::right << std::setw(16) << fileCount << " Datei(en)\n";
        std::cout << std::right << std::setw(16) << dirCount << " Verzeichnis(se)\n";

    } catch (const std::exception& e) {
        std::cerr << "Dosshell: " << e.what() << "\n";
        return 1;
    }
    return 0;
}

// ============================================================
// cd
// ============================================================
int Builtins::cmd_cd(Shell&, const std::vector<std::string>& args) {
    if (args.size() < 2) {
        std::cout << fs::current_path().string() << "\n";
        return 0;
    }

    std::string target = args[1];
    for (size_t i = 2; i < args.size(); i++) target += " " + args[i];

    try {
        fs::path newPath = fs::absolute(target);
        if (!fs::exists(newPath)) {
            std::cerr << "Dosshell: Verzeichnis nicht gefunden: " << target << "\n";
            return 1;
        }
        if (!fs::is_directory(newPath)) {
            std::cerr << "Dosshell: Kein Verzeichnis: " << target << "\n";
            return 1;
        }
        fs::current_path(newPath);
    } catch (const std::exception& e) {
        std::cerr << "Dosshell: " << e.what() << "\n";
        return 1;
    }
    return 0;
}

// ============================================================
// cls
// ============================================================
int Builtins::cmd_cls(Shell& shell, const std::vector<std::string>&) {
    shell.console().clear();
    return 0;
}

// ============================================================
// echo
// ============================================================
int Builtins::cmd_echo(Shell&, const std::vector<std::string>& args) {
    if (args.size() < 2) {
        std::cout << "ECHO ist eingeschaltet.\n";
        return 0;
    }
    for (size_t i = 1; i < args.size(); i++) {
        if (i > 1) std::cout << " ";
        std::cout << args[i];
    }
    std::cout << "\n";
    return 0;
}

// ============================================================
// type
// ============================================================
int Builtins::cmd_type(Shell&, const std::vector<std::string>& args) {
    if (args.size() < 2) {
        std::cerr << "Syntax: type <datei>\n";
        return 1;
    }
    for (size_t i = 1; i < args.size(); i++) {
        std::ifstream file(args[i]);
        if (!file.is_open()) {
            std::cerr << "Dosshell: Kann '" << args[i] << "' nicht oeffnen\n";
            return 1;
        }
        if (args.size() > 2) std::cout << "\n--- " << args[i] << " ---\n";
        std::string line;
        while (std::getline(file, line)) std::cout << line << "\n";
    }
    return 0;
}

// ============================================================
// copy
// ============================================================
int Builtins::cmd_copy(Shell&, const std::vector<std::string>& args) {
    if (args.size() < 3) {
        std::cerr << "Syntax: copy <quelle> <ziel>\n";
        return 1;
    }
    try {
        fs::copy(args[1], args[2], fs::copy_options::overwrite_existing);
        std::cout << "        1 Datei(en) kopiert.\n";
    } catch (const std::exception& e) {
        std::cerr << "Dosshell: " << e.what() << "\n";
        return 1;
    }
    return 0;
}

// ============================================================
// del
// ============================================================
int Builtins::cmd_del(Shell&, const std::vector<std::string>& args) {
    if (args.size() < 2) {
        std::cerr << "Syntax: del <datei>\n";
        return 1;
    }
    bool force = false;
    for (size_t i = 1; i < args.size(); i++) {
        if (args[i] == "/f" || args[i] == "-f") { force = true; continue; }
        try {
            fs::path target = args[i];
            if (!fs::exists(target)) {
                std::cerr << "Dosshell: '" << args[i] << "' nicht gefunden\n";
                return 1;
            }
            if (fs::is_directory(target) && !force) {
                std::cerr << "Dosshell: '" << args[i] << "' ist ein Verzeichnis. Verwende 'rd'.\n";
                return 1;
            }
            if (fs::remove(target)) std::cout << "Geloescht: " << args[i] << "\n";
        } catch (const std::exception& e) {
            std::cerr << "Dosshell: " << e.what() << "\n";
            return 1;
        }
    }
    return 0;
}

// ============================================================
// ren
// ============================================================
int Builtins::cmd_ren(Shell&, const std::vector<std::string>& args) {
    if (args.size() < 3) {
        std::cerr << "Syntax: ren <alter_name> <neuer_name>\n";
        return 1;
    }
    try {
        fs::rename(args[1], args[2]);
    } catch (const std::exception& e) {
        std::cerr << "Dosshell: " << e.what() << "\n";
        return 1;
    }
    return 0;
}

// ============================================================
// md
// ============================================================
int Builtins::cmd_md(Shell&, const std::vector<std::string>& args) {
    if (args.size() < 2) {
        std::cerr << "Syntax: md <verzeichnis>\n";
        return 1;
    }
    try {
        fs::create_directories(args[1]);
    } catch (const std::exception& e) {
        std::cerr << "Dosshell: " << e.what() << "\n";
        return 1;
    }
    return 0;
}

// ============================================================
// rd
// ============================================================
int Builtins::cmd_rd(Shell&, const std::vector<std::string>& args) {
    if (args.size() < 2) {
        std::cerr << "Syntax: rd <verzeichnis>\n";
        return 1;
    }
    bool recursive = false;
    for (size_t i = 1; i < args.size(); i++) {
        if (args[i] == "/s" || args[i] == "-r") { recursive = true; continue; }
        try {
            if (recursive) {
                auto count = fs::remove_all(args[i]);
                std::cout << count << " Eintraege entfernt.\n";
            } else {
                if (!fs::is_directory(args[i])) {
                    std::cerr << "Dosshell: '" << args[i] << "' ist kein Verzeichnis\n";
                    return 1;
                }
                if (!fs::is_empty(args[i])) {
                    std::cerr << "Dosshell: Verzeichnis nicht leer. Verwende 'rd /s'.\n";
                    return 1;
                }
                fs::remove(args[i]);
            }
        } catch (const std::exception& e) {
            std::cerr << "Dosshell: " << e.what() << "\n";
            return 1;
        }
    }
    return 0;
}

// ============================================================
// set
// ============================================================
int Builtins::cmd_set(Shell&, const std::vector<std::string>& args) {
    if (args.size() < 2) {
        LPCH env = GetEnvironmentStringsA();
        if (env) {
            const char* p = env;
            while (*p) {
                std::cout << p << "\n";
                p += strlen(p) + 1;
            }
            FreeEnvironmentStringsA(env);
        }
        return 0;
    }

    std::string assignment;
    for (size_t i = 1; i < args.size(); i++) {
        if (i > 1) assignment += " ";
        assignment += args[i];
    }

    auto eqPos = assignment.find('=');
    if (eqPos != std::string::npos) {
        std::string name = assignment.substr(0, eqPos);
        std::string value = assignment.substr(eqPos + 1);
        _putenv_s(name.c_str(), value.c_str());
    } else {
        const char* val = std::getenv(assignment.c_str());
        if (val) {
            std::cout << assignment << "=" << val << "\n";
        } else {
            std::cerr << "Umgebungsvariable '" << assignment << "' nicht definiert.\n";
            return 1;
        }
    }
    return 0;
}

// ============================================================
// ver
// ============================================================
int Builtins::cmd_ver(Shell&, const std::vector<std::string>&) {
    std::cout << "\nDosshell [Version 0.0.1]\n";
    std::cout << "Eine moderne Shell mit DOS-Feeling.\n";
    std::cout << "Inspiriert von PowerShell, gebaut in C++20.\n\n";
    return 0;
}

// ============================================================
// help
// ============================================================
int Builtins::cmd_help(Shell&, const std::vector<std::string>& args) {
    if (args.size() > 1) {
        std::string cmd = args[1];
        std::transform(cmd.begin(), cmd.end(), cmd.begin(),
                       [](unsigned char c) { return (char)std::tolower(c); });

        struct HelpEntry { const char* name; const char* desc; const char* syntax; };
        static const HelpEntry entries[] = {
            {"dir",     "Zeigt Verzeichnisinhalt an",       "dir [pfad] [/a] [/w]"},
            {"cd",      "Wechselt das Verzeichnis",         "cd [pfad]"},
            {"cls",     "Loescht den Bildschirm",           "cls"},
            {"echo",    "Gibt Text aus",                    "echo [text]"},
            {"type",    "Zeigt Dateiinhalt an",             "type <datei> [datei2 ...]"},
            {"copy",    "Kopiert eine Datei",               "copy <quelle> <ziel>"},
            {"del",     "Loescht eine Datei",               "del <datei> [/f]"},
            {"ren",     "Benennt eine Datei um",            "ren <alt> <neu>"},
            {"md",      "Erstellt ein Verzeichnis",         "md <verzeichnis>"},
            {"rd",      "Loescht ein Verzeichnis",          "rd <verzeichnis> [/s]"},
            {"set",     "Zeigt/setzt Umgebungsvariablen",   "set [name=wert]"},
            {"ver",     "Zeigt die Version an",             "ver"},
            {"color",   "Setzt Konsolenfarben",             "color <fg> [bg]"},
            {"title",   "Setzt den Fenstertitel",           "title <text>"},
            {"prompt",  "Setzt das Prompt-Format",          "prompt [format]"},
            {"alias",   "Definiert einen Alias",            "alias [name=befehl]"},
            {"pwd",     "Zeigt aktuelles Verzeichnis",      "pwd"},
            {"time",    "Zeigt die aktuelle Zeit",          "time"},
            {"date",    "Zeigt das aktuelle Datum",         "date"},
            {"exit",    "Beendet Dosshell",                 "exit [code]"},
            {"load",    "Laedt eine .ini-Bibliothek",       "load [name]"},
        };

        for (const auto& e : entries) {
            if (cmd == e.name) {
                std::cout << "\n" << e.name << " - " << e.desc << "\n";
                std::cout << "Syntax: " << e.syntax << "\n\n";
                return 0;
            }
        }
        std::cerr << "Dosshell: Keine Hilfe fuer '" << cmd << "'\n";
        return 1;
    }

    std::cout << "\n";
    std::cout << "Dosshell - Verfuegbare Befehle:\n";
    std::cout << "================================\n\n";
    std::cout << "  dir     - Verzeichnisinhalt anzeigen     set     - Umgebungsvariablen\n";
    std::cout << "  cd      - Verzeichnis wechseln           ver     - Version anzeigen\n";
    std::cout << "  cls     - Bildschirm loeschen            color   - Farben setzen\n";
    std::cout << "  echo    - Text ausgeben                  title   - Fenstertitel setzen\n";
    std::cout << "  type    - Dateiinhalt anzeigen           prompt  - Prompt aendern\n";
    std::cout << "  copy    - Datei kopieren                 alias   - Alias definieren\n";
    std::cout << "  del     - Datei loeschen                 pwd     - Aktuelles Verzeichnis\n";
    std::cout << "  ren     - Datei umbenennen               time    - Aktuelle Zeit\n";
    std::cout << "  md      - Verzeichnis erstellen          date    - Aktuelles Datum\n";
    std::cout << "  rd      - Verzeichnis loeschen           exit    - Dosshell beenden\n";
    std::cout << "  load    - .ini-Bibliothek laden\n\n";
    std::cout << "Tippe 'help <befehl>' fuer Details.\n";
    std::cout << "Externe Programme werden automatisch gesucht.\n";
    std::cout << "Pipes (|), Umleitung (> >> <) und && werden unterstuetzt.\n";
    std::cout << "Umgebungsvariablen: %VARNAME%\n\n";
    return 0;
}

// ============================================================
// exit
// ============================================================
int Builtins::cmd_exit(Shell& shell, const std::vector<std::string>& args) {
    int code = 0;
    if (args.size() > 1) {
        try { code = std::stoi(args[1]); } catch (...) {}
    }
    shell.quit(code);
    return code;
}

// ============================================================
// color
// ============================================================
int Builtins::cmd_color(Shell& shell, const std::vector<std::string>& args) {
    if (args.size() < 2) {
        std::cout << "Syntax: color <fg> [bg]\n";
        std::cout << "Farben: black, darkblue, darkgreen, darkcyan, darkred,\n";
        std::cout << "        darkmagenta, darkyellow, gray, darkgray, blue,\n";
        std::cout << "        green, cyan, red, magenta, yellow, white\n";
        std::cout << "Oder hex: 0-F (wie CMD)\n";
        return 0;
    }

    auto parseColor = [](const std::string& s) -> Color {
        if (s.size() == 1) {
            char c = (char)std::tolower((unsigned char)s[0]);
            if (c >= '0' && c <= '9') return (Color)(c - '0');
            if (c >= 'a' && c <= 'f') return (Color)(c - 'a' + 10);
        }
        std::string lower = s;
        std::transform(lower.begin(), lower.end(), lower.begin(),
                       [](unsigned char c) { return (char)std::tolower(c); });
        if (lower == "black") return Color::Black;
        if (lower == "darkblue") return Color::DarkBlue;
        if (lower == "darkgreen") return Color::DarkGreen;
        if (lower == "darkcyan") return Color::DarkCyan;
        if (lower == "darkred") return Color::DarkRed;
        if (lower == "darkmagenta") return Color::DarkMagenta;
        if (lower == "darkyellow") return Color::DarkYellow;
        if (lower == "gray") return Color::Gray;
        if (lower == "darkgray") return Color::DarkGray;
        if (lower == "blue") return Color::Blue;
        if (lower == "green") return Color::Green;
        if (lower == "cyan") return Color::Cyan;
        if (lower == "red") return Color::Red;
        if (lower == "magenta") return Color::Magenta;
        if (lower == "yellow") return Color::Yellow;
        if (lower == "white") return Color::White;
        return Color::Gray;
    };

    if (args[1].size() == 2 && args.size() == 2) {
        Color bg = parseColor(std::string(1, args[1][0]));
        Color fg = parseColor(std::string(1, args[1][1]));
        shell.console().setColor(fg, bg);
        return 0;
    }

    Color fg = parseColor(args[1]);
    Color bg = Color::Black;
    if (args.size() > 2) bg = parseColor(args[2]);
    shell.console().setColor(fg, bg);
    return 0;
}

// ============================================================
// title
// ============================================================
int Builtins::cmd_title(Shell& shell, const std::vector<std::string>& args) {
    if (args.size() < 2) {
        std::cerr << "Syntax: title <text>\n";
        return 1;
    }
    std::string title;
    for (size_t i = 1; i < args.size(); i++) {
        if (i > 1) title += " ";
        title += args[i];
    }
    shell.console().setTitle(title);
    return 0;
}

// ============================================================
// prompt
// ============================================================
int Builtins::cmd_prompt(Shell& shell, const std::vector<std::string>& args) {
    if (args.size() < 2) {
        std::cout << "Aktuelles Prompt: " << shell.getPromptFormat() << "\n";
        std::cout << "Codes: $P=Pfad, $G=>, $D=Datum, $T=Zeit, $N=Laufwerk, $$=$\n";
        return 0;
    }
    std::string fmt;
    for (size_t i = 1; i < args.size(); i++) {
        if (i > 1) fmt += " ";
        fmt += args[i];
    }
    shell.setPromptFormat(fmt);
    return 0;
}

// ============================================================
// alias
// ============================================================
int Builtins::cmd_alias(Shell& shell, const std::vector<std::string>& args) {
    if (args.size() < 2) {
        std::cout << "Syntax: alias name=befehl\n";
        return 0;
    }
    std::string assignment;
    for (size_t i = 1; i < args.size(); i++) {
        if (i > 1) assignment += " ";
        assignment += args[i];
    }
    auto eqPos = assignment.find('=');
    if (eqPos != std::string::npos) {
        std::string name = assignment.substr(0, eqPos);
        std::string target = assignment.substr(eqPos + 1);
        shell.engine().registerAlias(name, target);
        std::cout << "Alias: " << name << " -> " << target << "\n";
    } else {
        std::cerr << "Syntax: alias name=befehl\n";
        return 1;
    }
    return 0;
}

// ============================================================
// pwd
// ============================================================
int Builtins::cmd_pwd(Shell&, const std::vector<std::string>&) {
    std::cout << fs::current_path().string() << "\n";
    return 0;
}

// ============================================================
// time
// ============================================================
int Builtins::cmd_time(Shell&, const std::vector<std::string>&) {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    struct tm tm_buf;
    localtime_s(&tm_buf, &time);
    char buf[20];
    std::strftime(buf, sizeof(buf), "%H:%M:%S", &tm_buf);
    std::cout << "Aktuelle Zeit: " << buf << "\n";
    return 0;
}

// ============================================================
// date
// ============================================================
int Builtins::cmd_date(Shell&, const std::vector<std::string>&) {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    struct tm tm_buf;
    localtime_s(&tm_buf, &time);
    char buf[20];
    std::strftime(buf, sizeof(buf), "%d.%m.%Y", &tm_buf);
    std::cout << "Aktuelles Datum: " << buf << "\n";
    return 0;
}

// ============================================================
// load
// ============================================================
static std::string getDosshellDir() {
    const char* home = std::getenv("USERPROFILE");
    if (!home) home = std::getenv("HOME");
    if (!home) return "";
    return std::string(home) + "\\.dosshell";
}

static bool checkFilePermissions(const fs::path& filePath) {
    // Pruefen ob die Datei "zu offen" ist (Everyone/Users hat Schreibrechte)
    DWORD attrs = GetFileAttributesA(filePath.string().c_str());
    if (attrs == INVALID_FILE_ATTRIBUTES) return false;

    // Einfacher Check: Datei muss dem aktuellen User gehoeren
    // Wir pruefen ob die Datei ueberhaupt existiert und lesbar ist
    HANDLE hFile = CreateFileA(filePath.string().c_str(), GENERIC_READ,
                                FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) return false;

    // Pruefen ob andere Benutzer Schreibrechte haben
    SECURITY_INFORMATION secInfo = DACL_SECURITY_INFORMATION;
    DWORD needed = 0;
    GetFileSecurityA(filePath.string().c_str(), secInfo, nullptr, 0, &needed);

    bool safe = true;
    if (needed > 0) {
        std::vector<BYTE> buffer(needed);
        PSECURITY_DESCRIPTOR pSD = (PSECURITY_DESCRIPTOR)buffer.data();
        if (GetFileSecurityA(filePath.string().c_str(), secInfo, pSD, needed, &needed)) {
            PACL pDacl = nullptr;
            BOOL daclPresent = FALSE, daclDefault = FALSE;
            if (GetSecurityDescriptorDacl(pSD, &daclPresent, &pDacl, &daclDefault) && daclPresent && pDacl) {
                // Pruefen ob "Everyone" (Well-known SID S-1-1-0) Schreibzugriff hat
                SID_IDENTIFIER_AUTHORITY worldAuth = SECURITY_WORLD_SID_AUTHORITY;
                PSID pEveryoneSid = nullptr;
                if (AllocateAndInitializeSid(&worldAuth, 1, SECURITY_WORLD_RID,
                                              0, 0, 0, 0, 0, 0, 0, &pEveryoneSid)) {
                    TRUSTEE_A trustee = {};
                    trustee.TrusteeForm = TRUSTEE_IS_SID;
                    trustee.ptstrName = (LPSTR)pEveryoneSid;
                    ACCESS_MASK accessMask = 0;
                    if (GetEffectiveRightsFromAclA(pDacl, &trustee, &accessMask) == ERROR_SUCCESS) {
                        if (accessMask & (FILE_WRITE_DATA | FILE_APPEND_DATA)) {
                            safe = false;
                        }
                    }
                    FreeSid(pEveryoneSid);
                }
            }
        }
    }

    CloseHandle(hFile);
    return safe;
}

int Builtins::cmd_load(Shell& shell, const std::vector<std::string>& args) {
    if (args.size() < 2) {
        // Ohne Argument: verfuegbare .ini-Dateien auflisten
        std::string dir = getDosshellDir();
        if (dir.empty() || !fs::exists(dir)) {
            std::cout << "Dosshell-Verzeichnis nicht gefunden.\n";
            std::cout << "Erstelle: " << getDosshellDir() << "\n";
            std::cout << "Lege dort .ini-Dateien mit Dosshell-Befehlen ab.\n";
            return 0;
        }

        std::cout << "\nVerfuegbare Bibliotheken in " << dir << ":\n\n";
        bool found = false;
        for (const auto& entry : fs::directory_iterator(dir)) {
            if (entry.path().extension() == ".ini") {
                std::string name = entry.path().stem().string();
                // Erste Zeile als Beschreibung lesen (wenn Kommentar)
                std::ifstream f(entry.path());
                std::string firstLine;
                if (std::getline(f, firstLine) && firstLine.size() > 1 && firstLine[0] == ';') {
                    std::cout << "  " << std::left << std::setw(20) << name
                              << firstLine.substr(1) << "\n";
                } else {
                    std::cout << "  " << name << "\n";
                }
                found = true;
            }
        }
        if (!found) {
            std::cout << "  (keine .ini-Dateien gefunden)\n";
        }
        std::cout << "\nSyntax: load <name>\n";
        return 0;
    }

    // Bibliothek laden
    std::string name = args[1];
    std::string dir = getDosshellDir();
    fs::path filePath;

    // Erst als relativer/absoluter Pfad pruefen
    if (fs::exists(name) || fs::exists(name + ".ini")) {
        filePath = fs::exists(name) ? fs::path(name) : fs::path(name + ".ini");
    } else if (!dir.empty()) {
        // Dann im .dosshell-Verzeichnis suchen
        filePath = fs::path(dir) / (name + ".ini");
        if (!fs::exists(filePath)) {
            filePath = fs::path(dir) / name;
        }
    }

    if (!fs::exists(filePath)) {
        std::cerr << "Dosshell: Bibliothek '" << name << "' nicht gefunden\n";
        if (!dir.empty()) {
            std::cerr << "Gesucht in: " << dir << "\n";
        }
        return 1;
    }

    // Sicherheitspruefung
    if (!checkFilePermissions(filePath)) {
        shell.console().writeColored(
            "WARNUNG: Die Datei hat unsichere Berechtigungen!\n", Color::Red);
        shell.console().writeColored(
            "         Andere Benutzer koennten sie veraendert haben.\n", Color::Red);
        shell.console().write("Trotzdem laden? (j/n) ");
        std::string answer = shell.console().readLine("");
        if (answer != "j" && answer != "J" && answer != "ja" && answer != "Ja") {
            std::cout << "Abgebrochen.\n";
            return 1;
        }
    }

    // Datei Zeile fuer Zeile ausfuehren
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Dosshell: Kann '" << filePath.string() << "' nicht oeffnen\n";
        return 1;
    }

    int lineNum = 0;
    int errors = 0;
    std::string line;

    while (std::getline(file, line)) {
        lineNum++;

        // Trim
        auto start = line.find_first_not_of(" \t");
        if (start == std::string::npos) continue;
        line = line.substr(start);

        // Kommentare ueberspringen
        if (line[0] == ';' || line[0] == '#') continue;

        // Leere Zeilen ueberspringen
        if (line.empty()) continue;

        // Befehl ausfuehren
        auto cmdLine = Parser::parse(line);
        int result = shell.engine().execute(shell, cmdLine);
        if (result != 0) {
            std::cerr << "Dosshell: Fehler in Zeile " << lineNum
                      << ": " << line << "\n";
            errors++;
        }
    }

    if (errors > 0) {
        std::cerr << errors << " Fehler beim Laden von '" << name << "'\n";
        return 1;
    }

    return 0;
}
