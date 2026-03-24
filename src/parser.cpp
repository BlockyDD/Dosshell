#include "parser.h"
#include <cctype>
#include <cstdlib>
#include <ctime>

static bool sRandomSeeded = false;

std::string Parser::expandVariables(const std::string& input) {
    std::string result;
    size_t i = 0;
    while (i < input.size()) {
        if (input[i] == '%') {
            size_t end = input.find('%', i + 1);
            if (end != std::string::npos && end > i + 1) {
                std::string varName = input.substr(i + 1, end - i - 1);
                if (varName == "RANDOM") {
                    if (!sRandomSeeded) { srand((unsigned)time(nullptr)); sRandomSeeded = true; }
                    result += std::to_string(rand() % 32768);
                } else {
                    const char* val = std::getenv(varName.c_str());
                    if (val) {
                        result += val;
                    } else {
                        result += input.substr(i, end - i + 1);
                    }
                }
                i = end + 1;
            } else {
                result += input[i++];
            }
        } else {
            result += input[i++];
        }
    }
    return result;
}

std::vector<Token> Parser::tokenize(const std::string& input) {
    std::vector<Token> tokens;
    size_t i = 0;

    while (i < input.size()) {
        // Whitespace ueberspringen
        while (i < input.size() && std::isspace((unsigned char)input[i])) i++;
        if (i >= input.size()) break;

        char ch = input[i];

        // Pipe
        if (ch == '|') {
            tokens.push_back({Token::Pipe, "|"});
            i++;
            continue;
        }

        // Redirect
        if (ch == '>') {
            if (i + 1 < input.size() && input[i + 1] == '>') {
                tokens.push_back({Token::RedirectAppend, ">>"});
                i += 2;
            } else {
                tokens.push_back({Token::RedirectOut, ">"});
                i++;
            }
            continue;
        }

        if (ch == '<') {
            tokens.push_back({Token::RedirectIn, "<"});
            i++;
            continue;
        }

        // Ampersand
        if (ch == '&') {
            if (i + 1 < input.size() && input[i + 1] == '&') {
                tokens.push_back({Token::AmpAmp, "&&"});
                i += 2;
            } else {
                tokens.push_back({Token::Ampersand, "&"});
                i++;
            }
            continue;
        }

        // Semikolon
        if (ch == ';') {
            tokens.push_back({Token::Semicolon, ";"});
            i++;
            continue;
        }

        // Wort (mit Quoting)
        std::string word;
        while (i < input.size()) {
            ch = input[i];

            if (std::isspace((unsigned char)ch) || ch == '|' || ch == '>' ||
                ch == '<' || ch == '&' || ch == ';') {
                break;
            }

            // Doppelte Anfuehrungszeichen
            if (ch == '"') {
                i++; // Oeffnendes " ueberspringen
                while (i < input.size() && input[i] != '"') {
                    if (input[i] == '\\' && i + 1 < input.size()) {
                        // Escape-Sequenz
                        i++;
                        switch (input[i]) {
                            case 'n': word += '\n'; break;
                            case 't': word += '\t'; break;
                            case '"': word += '"'; break;
                            case '\\': word += '\\'; break;
                            default: word += '\\'; word += input[i]; break;
                        }
                    } else {
                        word += input[i];
                    }
                    i++;
                }
                if (i < input.size()) i++; // Schliessendes "
                continue;
            }

            // Einfache Anfuehrungszeichen (kein Escape)
            if (ch == '\'') {
                i++;
                while (i < input.size() && input[i] != '\'') {
                    word += input[i++];
                }
                if (i < input.size()) i++;
                continue;
            }

            word += ch;
            i++;
        }

        if (!word.empty()) {
            tokens.push_back({Token::Word, word});
        }
    }

    return tokens;
}

Command Parser::parseCommand(const std::vector<Token>& tokens, size_t& pos) {
    Command cmd;

    while (pos < tokens.size()) {
        const auto& tok = tokens[pos];

        // Stopper: Pipe, Semicolon, AmpAmp
        if (tok.type == Token::Pipe || tok.type == Token::Semicolon ||
            tok.type == Token::AmpAmp || tok.type == Token::Ampersand) {
            break;
        }

        if (tok.type == Token::RedirectOut) {
            pos++;
            if (pos < tokens.size() && tokens[pos].type == Token::Word) {
                cmd.redirectOut = tokens[pos].value;
                cmd.appendOut = false;
                pos++;
            }
            continue;
        }

        if (tok.type == Token::RedirectAppend) {
            pos++;
            if (pos < tokens.size() && tokens[pos].type == Token::Word) {
                cmd.redirectOut = tokens[pos].value;
                cmd.appendOut = true;
                pos++;
            }
            continue;
        }

        if (tok.type == Token::RedirectIn) {
            pos++;
            if (pos < tokens.size() && tokens[pos].type == Token::Word) {
                cmd.redirectIn = tokens[pos].value;
                pos++;
            }
            continue;
        }

        if (tok.type == Token::Word) {
            if (cmd.name.empty()) {
                cmd.name = tok.value;
            } else {
                cmd.args.push_back(tok.value);
            }
            pos++;
            continue;
        }

        pos++;
    }

    return cmd;
}

CommandLine Parser::parse(const std::string& input) {
    // Erst Variablen expandieren
    std::string expanded = expandVariables(input);

    // Tokenisieren
    auto tokens = tokenize(expanded);

    CommandLine cmdLine;
    size_t pos = 0;

    while (pos < tokens.size()) {
        Pipeline pipeline;

        // Commands in einer Pipeline lesen (getrennt durch |)
        while (pos < tokens.size()) {
            Command cmd = parseCommand(tokens, pos);
            if (!cmd.name.empty()) {
                pipeline.commands.push_back(std::move(cmd));
            }

            if (pos < tokens.size() && tokens[pos].type == Token::Pipe) {
                pos++; // Pipe ueberspringen
                continue;
            }
            break;
        }

        if (!pipeline.commands.empty()) {
            CommandLine::Entry entry;
            entry.pipeline = std::move(pipeline);
            entry.conditionalAnd = false;

            // Separator lesen
            if (pos < tokens.size()) {
                if (tokens[pos].type == Token::AmpAmp) {
                    entry.conditionalAnd = true;
                    pos++;
                } else if (tokens[pos].type == Token::Semicolon ||
                           tokens[pos].type == Token::Ampersand) {
                    pos++;
                }
            }

            cmdLine.entries.push_back(std::move(entry));
        }
    }

    return cmdLine;
}
