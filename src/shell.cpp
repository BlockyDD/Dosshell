#include "shell.h"
#include "builtins.h"
#include "script.h"
#include <filesystem>
#include <fstream>
#include <chrono>
#include <ctime>
#include <shlobj.h>

namespace fs = std::filesystem;

Shell::Shell() {
    registerAllBuiltins(engine_);
    console_.setTitle("Dosshell");
    autoRegisterDssh();
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

// ============================================================
// Auto-Registrierung: .dssh Dateityp + Icon beim Start
// ============================================================

static std::string getAutoRegDir() {
    const char* home = std::getenv("USERPROFILE");
    if (!home) home = std::getenv("HOME");
    if (!home) return "";
    return std::string(home) + "\\.dosshell";
}

static bool generateDsshIcon(const std::string& iconPath) {
    struct BGRA { uint8_t b, g, r, a; };
    BGRA pixels[32][32];

    BGRA transparent = {0, 0, 0, 0};
    BGRA border      = {66, 52, 50, 255};
    BGRA titleBar    = {54, 42, 40, 255};
    BGRA darkBg      = {30, 22, 20, 255};
    BGRA redDot      = {86, 95, 255, 255};
    BGRA yellowDot   = {46, 189, 255, 255};
    BGRA greenDot    = {63, 201, 39, 255};
    BGRA promptGreen = {123, 250, 80, 255};
    BGRA cursorWhite = {220, 200, 200, 255};

    for (int r = 0; r < 32; r++)
        for (int c = 0; c < 32; c++)
            pixels[r][c] = darkBg;

    for (int c = 0; c < 32; c++) { pixels[0][c] = border; pixels[31][c] = border; }
    for (int r = 0; r < 32; r++) { pixels[r][0] = border; pixels[r][31] = border; }

    pixels[0][0] = transparent; pixels[0][1] = transparent;
    pixels[0][30] = transparent; pixels[0][31] = transparent;
    pixels[1][0] = transparent; pixels[31][0] = transparent;
    pixels[31][31] = transparent; pixels[31][30] = transparent;
    pixels[31][0] = transparent; pixels[31][1] = transparent;
    pixels[30][0] = transparent; pixels[30][31] = transparent;

    for (int r = 1; r <= 3; r++)
        for (int c = 1; c <= 30; c++)
            pixels[r][c] = titleBar;

    for (int c = 1; c <= 30; c++) pixels[4][c] = border;

    pixels[2][3] = redDot;    pixels[2][4] = redDot;
    pixels[2][6] = yellowDot; pixels[2][7] = yellowDot;
    pixels[2][9] = greenDot;  pixels[2][10] = greenDot;

    auto setBlock = [&](int r, int c1, int c2, BGRA color) {
        for (int c = c1; c <= c2; c++) pixels[r][c] = color;
    };
    setBlock(11, 5, 7, promptGreen);
    setBlock(12, 7, 9, promptGreen);
    setBlock(13, 9, 11, promptGreen);
    setBlock(14, 11, 13, promptGreen);
    setBlock(15, 9, 11, promptGreen);
    setBlock(16, 7, 9, promptGreen);
    setBlock(17, 5, 7, promptGreen);
    setBlock(21, 16, 23, cursorWhite);
    setBlock(22, 16, 23, cursorWhite);

    std::ofstream out(iconPath, std::ios::binary);
    if (!out.is_open()) return false;

    uint16_t reserved = 0, type = 1, count = 1;
    out.write((char*)&reserved, 2);
    out.write((char*)&type, 2);
    out.write((char*)&count, 2);

    uint8_t width = 32, height = 32, colorPalette = 0, reservedByte = 0;
    uint16_t planes = 1, bpp = 32;
    uint32_t imageSize = 40 + (32 * 32 * 4) + (32 * 4);
    uint32_t imageOffset = 6 + 16;
    out.write((char*)&width, 1);
    out.write((char*)&height, 1);
    out.write((char*)&colorPalette, 1);
    out.write((char*)&reservedByte, 1);
    out.write((char*)&planes, 2);
    out.write((char*)&bpp, 2);
    out.write((char*)&imageSize, 4);
    out.write((char*)&imageOffset, 4);

    uint32_t headerSize = 40;
    int32_t bmpWidth = 32, bmpHeight = 64;
    uint16_t bmpPlanes = 1, bmpBpp = 32;
    uint32_t compression = 0, imgDataSize = 32 * 32 * 4, zero = 0;
    out.write((char*)&headerSize, 4);
    out.write((char*)&bmpWidth, 4);
    out.write((char*)&bmpHeight, 4);
    out.write((char*)&bmpPlanes, 2);
    out.write((char*)&bmpBpp, 2);
    out.write((char*)&compression, 4);
    out.write((char*)&imgDataSize, 4);
    out.write((char*)&zero, 4);
    out.write((char*)&zero, 4);
    out.write((char*)&zero, 4);
    out.write((char*)&zero, 4);

    for (int r = 31; r >= 0; r--)
        for (int c = 0; c < 32; c++)
            out.write((char*)&pixels[r][c], 4);

    uint8_t andMask[32 * 4] = {};
    out.write((char*)andMask, sizeof(andMask));
    out.close();
    return true;
}

void Shell::autoRegisterDssh() {
    char exeBuf[MAX_PATH];
    GetModuleFileNameA(nullptr, exeBuf, MAX_PATH);
    std::string exePath = exeBuf;
    std::string expectedCmd = "\"" + exePath + "\" \"%1\" %*";

    // Pruefen ob bereits korrekt registriert
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_CURRENT_USER,
                       "Software\\Classes\\DosshellScript\\shell\\open\\command",
                       0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        char buf[MAX_PATH * 2] = {};
        DWORD size = sizeof(buf);
        if (RegQueryValueExA(hKey, nullptr, nullptr, nullptr,
                              (BYTE*)buf, &size) == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            if (std::string(buf) == expectedCmd)
                return; // Bereits korrekt registriert
        } else {
            RegCloseKey(hKey);
        }
    }

    // .dosshell Verzeichnis + Icon
    std::string dsshDir = getAutoRegDir();
    if (dsshDir.empty()) return;
    fs::create_directories(dsshDir);

    std::string iconPath = dsshDir + "\\dssh.ico";
    generateDsshIcon(iconPath);

    // .dssh -> DosshellScript
    if (RegCreateKeyExA(HKEY_CURRENT_USER, "Software\\Classes\\.dssh",
                         0, nullptr, 0, KEY_WRITE, nullptr, &hKey, nullptr) == ERROR_SUCCESS) {
        const char* progId = "DosshellScript";
        RegSetValueExA(hKey, nullptr, 0, REG_SZ, (const BYTE*)progId,
                       (DWORD)strlen(progId) + 1);
        RegCloseKey(hKey);
    }

    if (RegCreateKeyExA(HKEY_CURRENT_USER, "Software\\Classes\\DosshellScript",
                         0, nullptr, 0, KEY_WRITE, nullptr, &hKey, nullptr) == ERROR_SUCCESS) {
        const char* typeName = "Dosshell Script";
        RegSetValueExA(hKey, nullptr, 0, REG_SZ, (const BYTE*)typeName,
                       (DWORD)strlen(typeName) + 1);
        RegCloseKey(hKey);
    }

    if (RegCreateKeyExA(HKEY_CURRENT_USER,
                         "Software\\Classes\\DosshellScript\\DefaultIcon",
                         0, nullptr, 0, KEY_WRITE, nullptr, &hKey, nullptr) == ERROR_SUCCESS) {
        RegSetValueExA(hKey, nullptr, 0, REG_SZ, (const BYTE*)iconPath.c_str(),
                       (DWORD)iconPath.size() + 1);
        RegCloseKey(hKey);
    }

    if (RegCreateKeyExA(HKEY_CURRENT_USER,
                         "Software\\Classes\\DosshellScript\\shell\\open\\command",
                         0, nullptr, 0, KEY_WRITE, nullptr, &hKey, nullptr) == ERROR_SUCCESS) {
        RegSetValueExA(hKey, nullptr, 0, REG_SZ, (const BYTE*)expectedCmd.c_str(),
                       (DWORD)expectedCmd.size() + 1);
        RegCloseKey(hKey);
    }

    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);
}
