#include "console.h"

Console::Console() {
    hStdout_ = GetStdHandle(STD_OUTPUT_HANDLE);
    hStdin_ = GetStdHandle(STD_INPUT_HANDLE);

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hStdout_, &csbi);
    originalAttribs_ = csbi.wAttributes;

    GetConsoleMode(hStdin_, &originalInMode_);
    GetConsoleMode(hStdout_, &originalOutMode_);

    enableVT();
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
}

Console::~Console() {
    resetColor();
    SetConsoleMode(hStdin_, originalInMode_);
    SetConsoleMode(hStdout_, originalOutMode_);
}

void Console::enableVT() {
    DWORD outMode = 0;
    GetConsoleMode(hStdout_, &outMode);
    outMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hStdout_, outMode);
}

void Console::write(const std::string& text) {
    DWORD written;
    WriteConsoleA(hStdout_, text.c_str(), (DWORD)text.size(), &written, nullptr);
}

void Console::writeLine(const std::string& text) {
    write(text + "\n");
}

void Console::writeColored(const std::string& text, Color fg, Color bg) {
    setColor(fg, bg);
    write(text);
    resetColor();
}

void Console::setColor(Color fg, Color bg) {
    SetConsoleTextAttribute(hStdout_, (WORD)fg | ((WORD)bg << 4));
}

void Console::resetColor() {
    SetConsoleTextAttribute(hStdout_, originalAttribs_);
}

void Console::clear() {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hStdout_, &csbi);
    DWORD size = csbi.dwSize.X * csbi.dwSize.Y;
    COORD topLeft = {0, 0};
    DWORD written;
    FillConsoleOutputCharacterA(hStdout_, ' ', size, topLeft, &written);
    FillConsoleOutputAttribute(hStdout_, csbi.wAttributes, size, topLeft, &written);
    SetConsoleCursorPosition(hStdout_, topLeft);
}

void Console::setTitle(const std::string& title) {
    SetConsoleTitleA(title.c_str());
}

int Console::getWidth() {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hStdout_, &csbi);
    return csbi.srWindow.Right - csbi.srWindow.Left + 1;
}

int Console::getHeight() {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hStdout_, &csbi);
    return csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
}

// Sichtbare Laenge ohne ANSI-Escape-Codes
size_t Console::visibleLength(const std::string& s) {
    size_t len = 0;
    bool inEsc = false;
    for (char c : s) {
        if (c == '\x1b') { inEsc = true; continue; }
        if (inEsc) { if (c == 'm') inEsc = false; continue; }
        len++;
    }
    return len;
}

std::string Console::readLine(const std::string& prompt) {
    write(prompt);

    size_t promptVis = visibleLength(prompt);
    std::string line;
    size_t cursorPos = 0;

    // Raw-Mode
    DWORD oldMode;
    GetConsoleMode(hStdin_, &oldMode);
    SetConsoleMode(hStdin_, ENABLE_WINDOW_INPUT);

    auto redraw = [&]() {
        write("\r");
        write(prompt);
        write(line);
        write("\x1b[K"); // Rest der Zeile loeschen
        // Cursor positionieren
        size_t target = promptVis + cursorPos;
        write("\r");
        if (target > 0) {
            write("\x1b[" + std::to_string(target) + "C");
        }
    };

    while (true) {
        INPUT_RECORD ir;
        DWORD read;
        ReadConsoleInputA(hStdin_, &ir, 1, &read);

        if (ir.EventType != KEY_EVENT || !ir.Event.KeyEvent.bKeyDown)
            continue;

        WORD vk = ir.Event.KeyEvent.wVirtualKeyCode;
        char ch = ir.Event.KeyEvent.uChar.AsciiChar;

        if (vk == VK_RETURN) {
            write("\n");
            break;
        }
        else if (vk == VK_BACK) {
            if (cursorPos > 0) {
                line.erase(cursorPos - 1, 1);
                cursorPos--;
                redraw();
            }
        }
        else if (vk == VK_DELETE) {
            if (cursorPos < line.size()) {
                line.erase(cursorPos, 1);
                redraw();
            }
        }
        else if (vk == VK_LEFT) {
            if (cursorPos > 0) {
                cursorPos--;
                write("\x1b[D");
            }
        }
        else if (vk == VK_RIGHT) {
            if (cursorPos < line.size()) {
                cursorPos++;
                write("\x1b[C");
            }
        }
        else if (vk == VK_HOME) {
            cursorPos = 0;
            redraw();
        }
        else if (vk == VK_END) {
            cursorPos = line.size();
            redraw();
        }
        else if (vk == VK_ESCAPE) {
            line.clear();
            cursorPos = 0;
            redraw();
        }
        else if (ch == 3) { // Ctrl+C
            line.clear();
            write("^C\n");
            break;
        }
        else if (ch == 12) { // Ctrl+L
            clear();
            write(prompt);
            write(line);
            cursorPos = line.size();
        }
        else if (ch >= 32 && ch < 127) {
            line.insert(cursorPos, 1, ch);
            cursorPos++;
            redraw();
        }
    }

    SetConsoleMode(hStdin_, oldMode);
    return line;
}
