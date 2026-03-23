#pragma once
//
// Dosshell Plugin API
//
// Um ein Plugin zu erstellen:
// 1. Diesen Header einbinden
// 2. Befehle als Funktionen schreiben: int mein_befehl(int argc, const char** argv)
// 3. dosshell_register() implementieren und Befehle registrieren
// 4. Als DLL kompilieren
// 5. In Dosshell laden mit: plugin <name>
//
// Beispiel:
//
//   #include "dosshell_plugin.h"
//   #include <cstdio>
//
//   int cmd_hello(int argc, const char** argv) {
//       if (argc > 1)
//           printf("Hallo, %s!\n", argv[1]);
//       else
//           printf("Hallo Welt!\n");
//       return 0;
//   }
//
//   DOSSHELL_EXPORT void dosshell_register(DosshellPluginAPI* api) {
//       api->register_command("hello", "Begruessung", cmd_hello);
//   }
//

#ifdef _WIN32
    #define DOSSHELL_EXPORT extern "C" __declspec(dllexport)
#else
    #define DOSSHELL_EXPORT extern "C" __attribute__((visibility("default")))
#endif

// Funktionstyp fuer Plugin-Befehle
// argc/argv wie in main(): argv[0]=Befehlsname, argv[1..]=Argumente
typedef int (*DosshellCommandFunc)(int argc, const char** argv);

// API die an Plugins uebergeben wird
struct DosshellPluginAPI {
    // Einen neuen Befehl registrieren
    void (*register_command)(const char* name, const char* description,
                             DosshellCommandFunc func);

    // Text ausgeben (geht ueber Dosshell Console)
    void (*print)(const char* text);

    // Einen Dosshell-Befehl ausfuehren (z.B. "dir /a")
    int (*execute)(const char* command);

    // Umgebungsvariable lesen
    const char* (*getenv)(const char* name);

    // Aktuelles Verzeichnis
    const char* (*getcwd)(void);

    // Plugin-Version (fuer zukuenftige Kompatibilitaet)
    int api_version;
};

// Signatur der Register-Funktion die jedes Plugin exportieren muss
typedef void (*DosshellRegisterFunc)(DosshellPluginAPI* api);

#define DOSSHELL_API_VERSION 1
