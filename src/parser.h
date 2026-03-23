#pragma once
#include <string>
#include <vector>

// Ein einzelnes Token
struct Token {
    enum Type {
        Word,       // normales Wort/Argument
        Pipe,       // |
        RedirectOut,  // >
        RedirectAppend, // >>
        RedirectIn,   // <
        Ampersand,  // & (Hintergrund / Verkettung)
        AmpAmp,     // && (bedingte Verkettung)
        Semicolon,  // ;
    };

    Type type;
    std::string value;
};

// Ein einzelner Befehl in der Pipeline
struct Command {
    std::string name;
    std::vector<std::string> args;
    std::string redirectIn;    // < datei
    std::string redirectOut;   // > datei
    bool appendOut = false;    // >> statt >
};

// Eine komplette Pipeline (cmd1 | cmd2 | cmd3)
struct Pipeline {
    std::vector<Command> commands;
};

// Mehrere Pipelines getrennt durch ; oder &&
struct CommandLine {
    struct Entry {
        Pipeline pipeline;
        bool conditionalAnd = false; // true = &&, false = ; oder erstes
    };
    std::vector<Entry> entries;
};

class Parser {
public:
    // Umgebungsvariablen expandieren (%VAR% Syntax)
    static std::string expandVariables(const std::string& input);

    // Tokenizer
    static std::vector<Token> tokenize(const std::string& input);

    // Parser: Tokens -> CommandLine
    static CommandLine parse(const std::string& input);

private:
    static Command parseCommand(const std::vector<Token>& tokens, size_t& pos);
};
