# Tests automatisés — CAMP-001

Ce dossier contient les livrables et l’automatisation de la campagne de test `CAMP-001`.

Les tests n’utilisent **que** la base MySQL `db4qtest`. Ils refusent explicitement toute autre base, notamment `dbtest`, afin de ne pas modifier une base servant aux essais manuels ou à l’application.

## Prérequis

- MySQL démarré localement sur `localhost:3306` ;
- Liquibase installé et accessible via la commande `liquibase` ;
- Qt 6.11.2 MinGW, CMake et Ninja installés aux chemins configurés dans `CMakePresets.json` ;
- PowerShell sous Windows.

## 1. Créer une base de test dédiée

Cette étape est à faire une seule fois, ou lorsqu’une base de test propre est nécessaire. Ne réutilisez ni une base de développement partagée, ni `dbtest`.

Avec un compte MySQL administrateur, exécutez par exemple :

```powershell
mysql -u root -p -e "CREATE DATABASE db4qtest CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci; CREATE USER IF NOT EXISTS 'tester'@'localhost' IDENTIFIED BY 'tester'; GRANT ALL PRIVILEGES ON db4qtest.* TO 'tester'@'localhost'; FLUSH PRIVILEGES;"
```

La commande crée la base et le compte local utilisés actuellement par le script de campagne. Pour choisir un autre compte ou mot de passe, adaptez à la fois :

- `liquibase/liquibase_AGO_qtest.properties` ;
- les variables `MB_TEST_DB_USER` et `MB_TEST_DB_PASSWORD` dans `Run-CAMP-001.ps1`.

Le compte doit pouvoir appliquer le schéma Liquibase et créer, modifier, supprimer puis nettoyer les données de fixture de test.

## 2. Installer ou mettre à jour le schéma avec Liquibase

Depuis la racine du dépôt :

```powershell
Push-Location .\liquibase
liquibase --defaults-file=liquibase_AGO_qtest.properties update
liquibase --defaults-file=liquibase_AGO_qtest.properties status --verbose
Pop-Location
```

Le fichier [liquibase_AGO_qtest.properties](../liquibase/liquibase_AGO_qtest.properties) cible `jdbc:mysql://localhost:3306/db4qtest` et applique le changelog `microbrico-changelog.mysql.sql`.

## 3. Lancer la campagne simplement

Depuis la racine du dépôt :

```powershell
.\Run-CAMP-001.ps1
```

Le script configure `db4qtest`, compile les tests, lance CTest et génère les rapports. Il crée et supprime uniquement les fixtures synthétiques de campagne dans cette base.

Dans VS Code, utilisez aussi la tâche **CAMP-001: lancer la campagne et produire le dashboard** via `Ctrl+Shift+P`, puis `Tasks: Run Test Task`.

## 4. Lancer les tests manuellement avec CMake et CTest

Si vous ne souhaitez pas utiliser le script, définissez d’abord la configuration de test dans le terminal PowerShell courant :

```powershell
$env:MB_TEST_DB_ENABLED = '1'
$env:MB_TEST_DB_DRIVER = 'QMYSQL'
$env:MB_TEST_DB_HOST = 'localhost'
$env:MB_TEST_DB_PORT = '3306'
$env:MB_TEST_DB_NAME = 'db4qtest'
$env:MB_TEST_DB_USER = 'tester'
$env:MB_TEST_DB_PASSWORD = 'tester'
$env:MB_TEST_ADMIN_PASSWORD = 'CAMP001-fixture-admin'
$env:QT_QPA_PLATFORM = 'offscreen'
```

Configurez, compilez et exécutez la campagne :

```powershell
& 'C:\Qt\Tools\CMake_64\bin\cmake.exe' --preset qt-6.11.2-mingw-debug
& 'C:\Qt\Tools\CMake_64\bin\cmake.exe' --build build\Desktop_Qt_6_11_2_MinGW_64_bit_Debug
& 'C:\Qt\Tools\CMake_64\bin\ctest.exe' --test-dir build\Desktop_Qt_6_11_2_MinGW_64_bit_Debug --output-on-failure -R '^CAMP-001\.'
```

Pour cibler deux cas précis :

```powershell
& 'C:\Qt\Tools\CMake_64\bin\ctest.exe' --test-dir build\Desktop_Qt_6_11_2_MinGW_64_bit_Debug --output-on-failure -R '^CAMP-001\.(tc041_output_user_signature_stock|tc043_return_stock_restoration)$'
```

L’exécution manuelle par CTest affiche les résultats dans le terminal mais ne génère pas le rapport enrichi de campagne. Utilisez `Run-CAMP-001.ps1` pour obtenir automatiquement le JUnit et le dashboard.

## 5. Consulter les résultats

Après un lancement par `Run-CAMP-001.ps1` :

- le terminal affiche le résumé CTest, par exemple `13/13 tests passed` ;
- le rapport JUnit horodaté est écrit dans `tests/campaigns/CAMP-001/reports/` avec l’extension `.junit.xml` ;
- le dernier dashboard est toujours disponible sous [latest.dashboard.html](campaigns/CAMP-001/reports/latest.dashboard.html) ;
- le détail brut du dernier CTest est dans `build/Desktop_Qt_6_11_2_MinGW_64_bit_Debug/Testing/Temporary/LastTest.log`.

Ouvrez `latest.dashboard.html` dans VS Code, puis sélectionnez **Open in Integrated Browser**. Le tableau de bord indique le bilan, la date et l’heure, le commit, le tag, la branche, l’état Git, puis toutes les étapes de chaque fiche de test. Les fichiers générés restent locaux et sont ignorés par Git.

Pour le format JUnit et la logique de localisation d’une étape en échec, consultez [reporting.md](campaigns/CAMP-001/reporting.md). Pour le détail de l’architecture de test, consultez [technical-foundation.md](campaigns/CAMP-001/technical-foundation.md).

## Organisation de la campagne

Une campagne suit cet ordre :

1. stratégie de test ;
2. fiches de test ;
3. revue des fiches ;
4. exécution et collecte des résultats ;
5. synthèse.

Les contrats et l’état de progression se trouvent dans `tests/contracts/` et `tests/campaigns/CAMP-001/`.
