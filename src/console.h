#pragma once
#include <string>
#include <windows.h>

enum class Color : WORD {
    Black       = 0,
    DarkBlue    = 1,
    DarkGreen   = 2,
    DarkCyan    = 3,
    DarkRed     = 4,
    DarkMagenta = 5,
    DarkYellow  = 6,
    Gray        = 7,
    DarkGray    = 8,
    Blue        = 9,
    Green       = 10,
    Cyan        = 11,
    Red         = 12,
    Magenta     = 13,
    Yellow      = 14,
    White       = 15
};

class Console {
public:
    Console();
    ~Console();

    void write(const std::string& text);
    void writeLine(const std::string& text = "");
    void writeColored(const std::string& text, Color fg, Color bg = Color::Black);

    void setColor(Color fg, Color bg = Color::Black);
    void resetColor();

    void clear();
    void setTitle(const std::string& title);

    // Eingabe mit Line-Editing (Prompt kann ANSI-Farbcodes enthalten)
    std::string readLine(const std::string& prompt);

    int getWidth();
    int getHeight();

private:
    HANDLE hStdout_;
    HANDLE hStdin_;
    WORD originalAttribs_;
    DWORD originalInMode_;
    DWORD originalOutMode_;

    void enableVT();
    static size_t visibleLength(const std::string& s);
};
