####### Compiler en Release depuis VS Code (Qt 6.11.2 / MinGW)

    Extensions : C/C++ et CMake Tools de Microsoft.
    Ouvrir le dossier du projet, puis Ctrl+Maj+B : la tâche par défaut
    "CMake: Build Release" configure CMake puis compile micro_brico_app.
    Exécutable : build/Desktop_Qt_6_11_2_MinGW_64_bit_Release/micro_brico_app.exe

    Pour utiliser aussi la vue CMake Tools et IntelliSense en Release :
    - Ctrl+Maj+P > CMake: Select Configure Preset > Qt 6.11.2 MinGW Release.
    - CMake: Configure.
    - CMake: Select Build Preset > Build Qt 6.11.2 MinGW Release.
    - CMake: Build.
    La tâche Ctrl+Maj+B compile toujours en Release, quel que soit le preset
    sélectionné dans la vue CMake Tools.

    Mémo : tasks.json et raccourcis de compilation
    - CMakePresets.json définit les configurations de compilation Debug/Release.
    - .vscode/tasks.json définit les tâches exécutables par VS Code :
      "CMake: Configure Release" prépare la compilation et génère compile_commands.json ;
      "CMake: Build Release" exécute cette configuration, puis compile micro_brico_app.
    - Ctrl+Maj+B lance ces tâches : compilation toujours en Release.
    - Pour changer de mode, cliquer sur le nom du preset CMake dans la barre
      d'état en bas de VS Code, puis choisir Debug ou Release. Sélectionner aussi
      le preset correspondant via CMake: Select Build Preset si nécessaire.
    - F7 lance CMake: Build avec le preset sélectionné (Debug ou Release).
      Cette commande utilise CMake Tools directement : tasks.json est facultatif.

    La configuration Release dans c_cpp_properties.json concerne IntelliSense ;
    elle ne sélectionne pas le mode de compilation. compile_commands.json est
    généré par CMake grâce à CMAKE_EXPORT_COMPILE_COMMANDS=ON.

    Commandes équivalentes dans un terminal PowerShell à la racine du projet :
    & C:/Qt/Tools/CMake_64/bin/cmake.exe --preset qt-6.11.2-mingw-release
    & C:/Qt/Tools/CMake_64/bin/cmake.exe --build --preset qt-6.11.2-mingw-release --parallel