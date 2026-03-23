//
// Dosshell Beispiel-Plugin
//
// Kompilieren:
//   cl /LD /EHsc hello_plugin.cpp /Fe:hello_plugin.dll
//
// Verwenden:
//   plugin hello_plugin
//   hello Welt
//   fortune
//

#include "../src/dosshell_plugin.h"
#include <cstdio>
#include <cstdlib>
#include <ctime>

// Ein einfacher Begrüßungs-Befehl
int cmd_hello(int argc, const char** argv) {
    if (argc > 1) {
        printf("Hallo, ");
        for (int i = 1; i < argc; i++) {
            if (i > 1) printf(" ");
            printf("%s", argv[i]);
        }
        printf("!\n");
    } else {
        printf("Hallo Welt!\n");
    }
    return 0;
}

// Zufälliger Spruch
int cmd_fortune(int argc, const char** argv) {
    (void)argc; (void)argv;
    static const char* fortunes[] = {
        "Der fruehe Vogel faengt den Wurm.",
        "Wer nicht wagt, der nicht gewinnt.",
        "In der Kuerze liegt die Wuerze.",
        "Uebung macht den Meister.",
        "Ein Byte hat acht Bit.",
        "Es gibt 10 Arten von Menschen: die, die Binaer verstehen, und die, die es nicht tun.",
        "DOS ist nicht tot, es riecht nur komisch.",
    };
    int count = sizeof(fortunes) / sizeof(fortunes[0]);
    srand((unsigned)time(nullptr));
    printf("\n  \"%s\"\n\n", fortunes[rand() % count]);
    return 0;
}

// Plugin-Registrierung
DOSSHELL_EXPORT void dosshell_register(DosshellPluginAPI* api) {
    api->register_command("hello", "Begruessung", cmd_hello);
    api->register_command("fortune", "Zufaelliger Spruch", cmd_fortune);
}
