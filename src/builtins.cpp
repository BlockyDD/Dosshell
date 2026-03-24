#include "builtins.h"
#include "shell.h"
#include "script.h"
#include "dosshell_plugin.h"
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
#include <cmath>

namespace fs = std::filesystem;

void registerAllBuiltins(PipelineEngine& engine) {
    engine.registerBuiltin("ls",      Builtins::cmd_ls);
    engine.registerBuiltin("cd",      Builtins::cmd_cd);
    engine.registerBuiltin("cat",     Builtins::cmd_cat);
    engine.registerBuiltin("cp",      Builtins::cmd_cp);
    engine.registerBuiltin("mv",      Builtins::cmd_mv);
    engine.registerBuiltin("rm",      Builtins::cmd_rm);
    engine.registerBuiltin("mk",      Builtins::cmd_mk);
    engine.registerBuiltin("echo",    Builtins::cmd_echo);
    engine.registerBuiltin("input",   Builtins::cmd_input);
    engine.registerBuiltin("clear",   Builtins::cmd_clear);
    engine.registerBuiltin("pause",   Builtins::cmd_pause);
    engine.registerBuiltin("set",     Builtins::cmd_set);
    engine.registerBuiltin("math",    Builtins::cmd_math);
    engine.registerBuiltin("help",    Builtins::cmd_help);
    engine.registerBuiltin("exit",    Builtins::cmd_exit);
    engine.registerBuiltin("color",   Builtins::cmd_color);
    engine.registerBuiltin("title",   Builtins::cmd_title);
    engine.registerBuiltin("prompt",  Builtins::cmd_prompt);
    engine.registerBuiltin("alias",   Builtins::cmd_alias);
    engine.registerBuiltin("time",    Builtins::cmd_time);
    engine.registerBuiltin("date",    Builtins::cmd_date);
    engine.registerBuiltin("load",    Builtins::cmd_load);
    engine.registerBuiltin("plugin",  Builtins::cmd_plugin);
    engine.registerBuiltin("call",    Builtins::cmd_call);
}

// ============================================================
// ls
// ============================================================
int Builtins::cmd_ls(Shell&, const std::vector<std::string>& args) {
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
// clear
// ============================================================
int Builtins::cmd_clear(Shell& shell, const std::vector<std::string>&) {
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
// cat
// ============================================================
int Builtins::cmd_cat(Shell&, const std::vector<std::string>& args) {
    if (args.size() < 2) {
        std::cerr << "Syntax: cat <datei>\n";
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
// cp
// ============================================================
int Builtins::cmd_cp(Shell&, const std::vector<std::string>& args) {
    if (args.size() < 3) {
        std::cerr << "Syntax: cp <quelle> <ziel>\n";
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
// rm — Dateien und Verzeichnisse loeschen
// ============================================================
int Builtins::cmd_rm(Shell&, const std::vector<std::string>& args) {
    if (args.size() < 2) {
        std::cerr << "Syntax: rm <ziel> [-r]\n";
        return 1;
    }
    bool recursive = false;
    std::vector<std::string> targets;
    for (size_t i = 1; i < args.size(); i++) {
        if (args[i] == "-r" || args[i] == "/r" || args[i] == "/s") recursive = true;
        else targets.push_back(args[i]);
    }
    for (const auto& t : targets) {
        try {
            fs::path p = t;
            if (!fs::exists(p)) {
                std::cerr << "Dosshell: '" << t << "' nicht gefunden\n";
                return 1;
            }
            if (fs::is_directory(p)) {
                if (!recursive && !fs::is_empty(p)) {
                    std::cerr << "Dosshell: '" << t << "' ist nicht leer. Verwende 'rm -r'.\n";
                    return 1;
                }
                auto count = fs::remove_all(p);
                if (recursive) std::cout << count << " Eintraege entfernt.\n";
            } else {
                fs::remove(p);
                std::cout << "Geloescht: " << t << "\n";
            }
        } catch (const std::exception& e) {
            std::cerr << "Dosshell: " << e.what() << "\n";
            return 1;
        }
    }
    return 0;
}

// ============================================================
// mv
// ============================================================
int Builtins::cmd_mv(Shell&, const std::vector<std::string>& args) {
    if (args.size() < 3) {
        std::cerr << "Syntax: mv <alt> <neu>\n";
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
// mk
// ============================================================
int Builtins::cmd_mk(Shell&, const std::vector<std::string>& args) {
    if (args.size() < 2) {
        std::cerr << "Syntax: mk <verzeichnis>\n";
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
// set — Umgebungsvariablen
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
// help
// ============================================================
int Builtins::cmd_help(Shell& shell, const std::vector<std::string>& args) {
    if (args.size() > 1) {
        std::string cmd = args[1];
        std::transform(cmd.begin(), cmd.end(), cmd.begin(),
                       [](unsigned char c) { return (char)std::tolower(c); });

        struct HelpEntry { const char* name; const char* desc; const char* syntax; };
        static const HelpEntry entries[] = {
            {"ls",      "Dateien und Ordner auflisten",     "ls [pfad] [-a] [-w]"},
            {"cd",      "Verzeichnis wechseln/anzeigen",    "cd [pfad]"},
            {"cat",     "Dateiinhalt anzeigen",             "cat <datei> [datei2 ...]"},
            {"cp",      "Datei kopieren",                   "cp <quelle> <ziel>"},
            {"mv",      "Datei verschieben/umbenennen",     "mv <alt> <neu>"},
            {"rm",      "Datei oder Ordner loeschen",       "rm <ziel> [-r]"},
            {"mk",      "Ordner erstellen",                 "mk <verzeichnis>"},
            {"echo",    "Text ausgeben",                    "echo [text]"},
            {"input",   "Eingabe in Variable lesen",        "input <variable> [prompt]"},
            {"clear",   "Bildschirm loeschen",              "clear"},
            {"pause",   "Auf Tastendruck warten",           "pause"},
            {"set",     "Umgebungsvariablen verwalten",     "set [name=wert]"},
            {"math",    "Mathematik berechnen",             "math <ausdruck>  (+ - * / %)"},
            {"color",   "Konsolenfarben setzen",            "color <fg> [bg]"},
            {"title",   "Fenstertitel setzen",              "title <text>"},
            {"prompt",  "Prompt-Format aendern",            "prompt [format]  ($P $G $D $T)"},
            {"alias",   "Alias definieren",                 "alias [name=befehl]"},
            {"time",    "Aktuelle Zeit anzeigen",           "time"},
            {"date",    "Aktuelles Datum anzeigen",         "date"},
            {"exit",    "Dosshell beenden",                 "exit [code]"},
            {"call",    ".dssh Script ausfuehren",          "call <script.dssh> [argumente...]"},
            {"load",    ".ini-Bibliothek laden",            "load [name]"},
            {"plugin",  "C++ DLL-Plugin laden",             "plugin <name|pfad>"},
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

    std::cout << "\nDosshell v0.0.1 - Eine moderne Shell\n";
    std::cout << "====================================\n\n";
    std::cout << "  Dateien          I/O              Shell\n";
    std::cout << "  ------           ---              -----\n";
    std::cout << "  ls   Auflisten   echo  Ausgabe    color  Farben\n";
    std::cout << "  cd   Wechseln    input Eingabe    title  Titel\n";
    std::cout << "  cat  Anzeigen    clear Loeschen   prompt Format\n";
    std::cout << "  cp   Kopieren    pause Warten     alias  Kuerzel\n";
    std::cout << "  mv   Verschieben                  time   Zeit\n";
    std::cout << "  rm   Loeschen    Variablen        date   Datum\n";
    std::cout << "  mk   Erstellen   ---------        exit   Beenden\n";
    std::cout << "                   set   Variablen  help   Hilfe\n";
    std::cout << "  Scripting        math  Rechnen\n";
    std::cout << "  ---------\n";
    std::cout << "  call   .dssh Script\n";
    std::cout << "  load   .ini Bibliothek\n";
    std::cout << "  plugin DLL Plugin\n\n";
    std::cout << "Tippe 'help <befehl>' fuer Details.\n";
    std::cout << ".dssh Scripte werden per Name automatisch gefunden.\n";
    std::cout << "Pipes (|), Umleitung (> >> <), && und %VARIABLEN% verfuegbar.\n";

    // Script-Befehle anzeigen
    const auto& scripts = shell.engine().scriptCommands();
    if (!scripts.empty()) {
        std::cout << "\nGeladene Script-Befehle:\n";
        for (const auto& [name, cmd] : scripts) {
            if (!cmd.description.empty()) {
                std::cout << "  " << std::left << std::setw(12) << name
                          << "- " << cmd.description << "\n";
            } else {
                std::cout << "  " << name << "\n";
            }
        }
    }
    std::cout << "\n";
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

    // Datei laden und parsen
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Dosshell: Kann '" << filePath.string() << "' nicht oeffnen\n";
        return 1;
    }

    int lineNum = 0;
    int errors = 0;
    int commandsRegistered = 0;
    std::string line;

    // Aktueller Command-Block (wenn in [command:name] Sektion)
    ScriptCommand currentCmd;
    bool inCommandSection = false;
    std::string lastComment;

    while (std::getline(file, line)) {
        lineNum++;

        // Trim
        auto start = line.find_first_not_of(" \t");
        if (start == std::string::npos) {
            // Leere Zeile in Command-Sektion: ignorieren
            continue;
        }
        line = line.substr(start);

        // Kommentar merken (koennte Beschreibung fuer naechsten Command sein)
        if (line[0] == ';' || line[0] == '#') {
            if (!inCommandSection) {
                lastComment = line.substr(1);
                auto cs = lastComment.find_first_not_of(" \t");
                if (cs != std::string::npos) lastComment = lastComment.substr(cs);
            }
            continue;
        }

        if (line.empty()) continue;

        // [command:name] Sektion erkennen
        if (line[0] == '[') {
            // Vorherigen Command-Block abschliessen
            if (inCommandSection && !currentCmd.name.empty() && !currentCmd.lines.empty()) {
                shell.engine().registerScriptCommand(currentCmd);
                commandsRegistered++;
            }

            // Neuen Command-Block pruefen
            if (line.size() > 9 && line.substr(0, 9) == "[command:" && line.back() == ']') {
                currentCmd = ScriptCommand{};
                currentCmd.name = line.substr(9, line.size() - 10);
                currentCmd.description = lastComment;
                inCommandSection = true;
                lastComment.clear();
            } else {
                inCommandSection = false;
            }
            continue;
        }

        if (inCommandSection) {
            // Zeile zum aktuellen Command-Block hinzufuegen
            currentCmd.lines.push_back(line);
        } else {
            // Normale Zeile: sofort ausfuehren
            auto cmdLine = Parser::parse(line);
            int result = shell.engine().execute(shell, cmdLine);
            if (result != 0) {
                std::cerr << "Dosshell: Fehler in Zeile " << lineNum
                          << ": " << line << "\n";
                errors++;
            }
        }
    }

    // Letzten Command-Block abschliessen
    if (inCommandSection && !currentCmd.name.empty() && !currentCmd.lines.empty()) {
        shell.engine().registerScriptCommand(currentCmd);
        commandsRegistered++;
    }

    if (commandsRegistered > 0) {
        std::cout << commandsRegistered << " Befehl(e) registriert aus '"
                  << filePath.filename().string() << "'\n";
    }

    if (errors > 0) {
        std::cerr << errors << " Fehler beim Laden von '" << name << "'\n";
        return 1;
    }

    return 0;
}

// ============================================================
// plugin
// ============================================================

// Globaler State fuer Plugin-API Callbacks
static Shell* g_pluginShell = nullptr;
static std::string g_cwdBuffer;

static void pluginPrint(const char* text) {
    if (g_pluginShell) {
        g_pluginShell->console().write(text);
    }
}

static int pluginExecute(const char* command) {
    if (!g_pluginShell) return 1;
    auto cmdLine = Parser::parse(command);
    return g_pluginShell->engine().execute(*g_pluginShell, cmdLine);
}

static const char* pluginGetenv(const char* name) {
    return std::getenv(name);
}

static const char* pluginGetcwd() {
    g_cwdBuffer = fs::current_path().string();
    return g_cwdBuffer.c_str();
}

static void pluginRegisterCommand(const char* name, const char* description,
                                   DosshellCommandFunc func) {
    if (!g_pluginShell) return;

    // Plugin-Funktion in eine BuiltinFunc wrappen
    DosshellCommandFunc capturedFunc = func;
    std::string capturedDesc = description ? description : "";

    g_pluginShell->engine().registerBuiltin(name,
        [capturedFunc](Shell&, const std::vector<std::string>& args) -> int {
            // std::vector<std::string> -> argc/argv
            std::vector<const char*> argv;
            for (const auto& a : args) argv.push_back(a.c_str());
            return capturedFunc((int)argv.size(), argv.data());
        });
}

int Builtins::cmd_plugin(Shell& shell, const std::vector<std::string>& args) {
    if (args.size() < 2) {
        std::cout << "Syntax: plugin <name|pfad>\n";
        std::cout << "Laedt eine C++ DLL als Plugin.\n";
        std::cout << "Die DLL muss dosshell_register() exportieren.\n\n";

        // Plugins im .dosshell Verzeichnis auflisten
        std::string dir = getDosshellDir();
        if (!dir.empty() && fs::exists(dir)) {
            std::cout << "Verfuegbare Plugins in " << dir << ":\n";
            bool found = false;
            for (const auto& entry : fs::directory_iterator(dir)) {
                if (entry.path().extension() == ".dll") {
                    std::cout << "  " << entry.path().stem().string() << "\n";
                    found = true;
                }
            }
            if (!found) std::cout << "  (keine .dll-Dateien gefunden)\n";
        }
        return 0;
    }

    std::string name = args[1];
    std::string dir = getDosshellDir();
    fs::path dllPath;

    // Pfad auflösen
    if (fs::exists(name)) {
        dllPath = fs::path(name);
    } else if (fs::exists(name + ".dll")) {
        dllPath = fs::path(name + ".dll");
    } else if (!dir.empty()) {
        dllPath = fs::path(dir) / (name + ".dll");
        if (!fs::exists(dllPath)) {
            dllPath = fs::path(dir) / name;
        }
    }

    if (!fs::exists(dllPath)) {
        std::cerr << "Dosshell: Plugin '" << name << "' nicht gefunden\n";
        return 1;
    }

    // Sicherheitspruefung
    if (!checkFilePermissions(dllPath)) {
        shell.console().writeColored(
            "WARNUNG: Die DLL hat unsichere Berechtigungen!\n", Color::Red);
        shell.console().write("Trotzdem laden? (j/n) ");
        std::string answer = shell.console().readLine("");
        if (answer != "j" && answer != "J" && answer != "ja" && answer != "Ja") {
            std::cout << "Abgebrochen.\n";
            return 1;
        }
    }

    // DLL laden
    HMODULE hModule = LoadLibraryA(dllPath.string().c_str());
    if (!hModule) {
        std::cerr << "Dosshell: Kann DLL nicht laden (Fehler "
                  << GetLastError() << ")\n";
        return 1;
    }

    // Register-Funktion finden
    auto registerFunc = (DosshellRegisterFunc)GetProcAddress(hModule, "dosshell_register");
    if (!registerFunc) {
        std::cerr << "Dosshell: DLL hat keine dosshell_register() Funktion\n";
        FreeLibrary(hModule);
        return 1;
    }

    // Plugin-API aufbauen
    DosshellPluginAPI api = {};
    api.api_version = DOSSHELL_API_VERSION;
    api.register_command = pluginRegisterCommand;
    api.print = pluginPrint;
    api.execute = pluginExecute;
    api.getenv = pluginGetenv;
    api.getcwd = pluginGetcwd;

    // Plugin registrieren
    g_pluginShell = &shell;
    registerFunc(&api);
    g_pluginShell = nullptr;

    std::cout << "Plugin '" << dllPath.filename().string() << "' geladen.\n";
    return 0;
}

// ============================================================
// call
// ============================================================
int Builtins::cmd_call(Shell& shell, const std::vector<std::string>& args) {
    if (args.size() < 2) {
        std::cerr << "Syntax: call <script.dssh> [argumente...]\n";
        return 1;
    }

    std::string scriptPath = ScriptRunner::findScript(args[1]);
    if (scriptPath.empty()) {
        std::cerr << "Dosshell: Script '" << args[1] << "' nicht gefunden\n";
        return 1;
    }

    // Args weiterreichen: args[1]=Scriptname, args[2..]=Parameter
    std::vector<std::string> scriptArgs(args.begin() + 1, args.end());
    return ScriptRunner::execute(shell, scriptPath, scriptArgs);
}

// ============================================================
// pause
// ============================================================
int Builtins::cmd_pause(Shell& shell, const std::vector<std::string>&) {
    shell.console().write("Druecke eine beliebige Taste . . . ");

    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    DWORD oldMode;
    GetConsoleMode(hStdin, &oldMode);
    SetConsoleMode(hStdin, ENABLE_WINDOW_INPUT);

    INPUT_RECORD ir;
    DWORD read;
    while (true) {
        ReadConsoleInputA(hStdin, &ir, 1, &read);
        if (ir.EventType == KEY_EVENT && ir.Event.KeyEvent.bKeyDown)
            break;
    }

    SetConsoleMode(hStdin, oldMode);
    shell.console().writeLine("");
    return 0;
}

// ============================================================
// math — Ausdruecke berechnen
// ============================================================
namespace MathParser {
    static void skipWS(const std::string& s, size_t& p) {
        while (p < s.size() && std::isspace((unsigned char)s[p])) p++;
    }
    static double parseExpr(const std::string& s, size_t& p);
    static double parseAtom(const std::string& s, size_t& p) {
        skipWS(s, p);
        if (p < s.size() && s[p] == '(') {
            p++;
            double val = parseExpr(s, p);
            skipWS(s, p);
            if (p < s.size() && s[p] == ')') p++;
            return val;
        }
        bool neg = false;
        if (p < s.size() && (s[p] == '-' || s[p] == '+')) {
            neg = (s[p] == '-'); p++;
            skipWS(s, p);
        }
        size_t start = p;
        while (p < s.size() && (std::isdigit((unsigned char)s[p]) || s[p] == '.')) p++;
        double val = (p > start) ? std::stod(s.substr(start, p - start)) : 0.0;
        return neg ? -val : val;
    }
    static double parseMulDiv(const std::string& s, size_t& p) {
        double left = parseAtom(s, p);
        while (true) {
            skipWS(s, p);
            if (p >= s.size()) break;
            char op = s[p];
            if (op != '*' && op != '/' && op != '%') break;
            p++;
            double right = parseAtom(s, p);
            if (op == '*') left *= right;
            else if (op == '/') left = (right != 0) ? left / right : 0;
            else left = std::fmod(left, right);
        }
        return left;
    }
    static double parseExpr(const std::string& s, size_t& p) {
        double left = parseMulDiv(s, p);
        while (true) {
            skipWS(s, p);
            if (p >= s.size()) break;
            char op = s[p];
            if (op != '+' && op != '-') break;
            p++;
            double right = parseMulDiv(s, p);
            if (op == '+') left += right; else left -= right;
        }
        return left;
    }
}

int Builtins::cmd_math(Shell&, const std::vector<std::string>& args) {
    if (args.size() < 2) {
        std::cerr << "Syntax: math <ausdruck>\n";
        std::cerr << "Beispiel: math 3 + 4 * (2 - 1)\n";
        return 1;
    }
    std::string expr;
    for (size_t i = 1; i < args.size(); i++) {
        if (i > 1) expr += " ";
        expr += args[i];
    }
    size_t pos = 0;
    double result = MathParser::parseExpr(expr, pos);
    std::ostringstream oss;
    if (result == std::floor(result) && std::abs(result) < 1e15)
        oss << static_cast<long long>(result);
    else
        oss << result;
    std::cout << oss.str() << "\n";
    _putenv_s("RESULT", oss.str().c_str());
    return 0;
}

// ============================================================
// input — Eingabe in Variable
// ============================================================
int Builtins::cmd_input(Shell& shell, const std::vector<std::string>& args) {
    if (args.size() < 2) {
        std::cerr << "Syntax: input <variable> [prompt]\n";
        return 1;
    }
    std::string varName = args[1];
    std::string prompt;
    for (size_t i = 2; i < args.size(); i++) {
        if (i > 2) prompt += " ";
        prompt += args[i];
    }
    if (!prompt.empty()) prompt += " ";
    std::string value = shell.console().readLine(prompt);
    _putenv_s(varName.c_str(), value.c_str());
    return 0;
}

