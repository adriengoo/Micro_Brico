# Socle technique d'automatisation — CAMP-001

Statut : **intégré — TC-001, TC-002, TC-003, TC-010, TC-011, TC-020, TC-021, TC-040 et TC-042 automatisés et validés sur `db4qtest` le 2026-09-22**.

## Périmètre et garantie de base

Tous les accès de cette intégration ont ciblé exclusivement `db4qtest` sur `localhost:3306`, avec le compte de test dédié communiqué au responsable de campagne. `dbtest` n'a été ni ouvert, ni lu, ni modifié.

Le contrôle Liquibase suivant a été exécuté sur `db4qtest` et indique que son schéma est à jour :

```powershell
& 'C:\Program Files\liquibase\liquibase.bat' --defaults-file=liquibase_AGO_qtest.properties status --verbose
```

## Connexion de test intégrée

[`tests/automation/test_database_config.cpp`](../../automation/test_database_config.cpp) valide les variables suivantes avant toute ouverture de base :

| Variable | Règle |
| --- | --- |
| `MB_TEST_DB_ENABLED` | doit valoir `1` |
| `MB_TEST_DB_DRIVER` | doit valoir `QMYSQL` |
| `MB_TEST_DB_HOST`, `MB_TEST_DB_PORT` | hôte non vide et port 1–65535 |
| `MB_TEST_DB_NAME` | doit valoir exactement `db4qtest` |
| `MB_TEST_DB_USER`, `MB_TEST_DB_PASSWORD` | obligatoires ; le secret ne doit pas être versionné |
| `MB_TEST_ADMIN_PASSWORD` | secret du compte fonctionnel injecté par la fixture TC-001 |

Dans [`mainwindow.cpp`](../../../mainwindow.cpp), l'opt-in `MB_TEST_DB_ENABLED=1` crée la connexion `microbrico_test_db4qtest`, puis la transmet à `Connect_db`. Une configuration demandée mais invalide provoque un arrêt explicite, sans repli possible vers `dbtest`. Hors opt-in, le comportement de connexion normal est conservé. Le raccourci de connexion automatique Debug est désactivé seulement sous cet opt-in, car TC-001 doit commencer sans session.

## CTest et pilotes disponibles

Deux cibles existent :

- `micro_brico_test_foundation` / `CAMP-001.TC-001.foundation` : garde sans SQL ; elle est `Skipped` sans opt-in explicite.
- `micro_brico_tc001` / `CAMP-001.TC-001.login-session` : test Qt Test IHM, avec QMYSQL, limité à `db4qtest`.
- `micro_brico_campaign_cases` : pilotes Qt Test sériels TC-002, TC-003, TC-010, TC-011, TC-020, TC-021, TC-040 et TC-042. Chaque cas s'exécute dans son propre processus et remet à zéro uniquement ses fixtures `db4qtest`.

Pour TC-003, l'action cartographiée est `RESA_pushButton_resa_showResa` : elle appelle `get_user_by_utinfo`, dont la requête passe par `is_user_connected`. Après invalidation contrôlée du jeton, le pilote vérifie le retour à l'état IHM déconnecté, sans créer de réservation.

Le pilote IHM ([`tc001_login_session.cpp`](../../automation/tc001_login_session.cpp)) :

1. ouvre une connexion de fixture distincte, également vers `db4qtest` ;
2. supprime les résidus de fixture `TAD001`, puis ajoute l'administrateur temporaire avec un mot de passe fourni via `MB_TEST_ADMIN_PASSWORD` ;
3. instancie `MainWindow` et vérifie sa connexion `microbrico_test_db4qtest` ainsi que l'état IHM déconnecté ;
4. ouvre `login_connect`, saisit les identifiants, valide la connexion, puis valide le basculement des actions connexion/déconnexion ;
5. vérifie la création du jeton et l'absence de réservation ou sortie active ;
6. efface les données de fixture et contrôle l'absence finale de `TAD001`.

Dernier résultat : **PASS** en 0,62 s avec `QT_QPA_PLATFORM=offscreen`.

## Cartographie IHM disponible

| Fiches | Contrôles Qt Test principaux |
| --- | --- |
| TC-001 à TC-003 | `actionSe_connecter`, `actionSe_d_connecter`, dialogue `login_connect`, `lineEdit_utinfo`, `lineEdit_mdp`, `pushButton_ok` |
| TC-010 à TC-011 | `GESUSER_tableWidget_user`, `GESUSER_lineEdit_finduserbyname`, `GESUSER_lineEdit_finduserbyutinfo`, dialogue `popupAddUsers` |
| TC-020 à TC-021 | `GESKIT_tableWidget_kit`, `GESKIT_pushButton_addkit`, dialogue `PoppupAddKit` |
| TC-030 à TC-031 | `RESA_lineEdit_resa_utinfo_user`, `RESA_listWidget_panierResa`, `RESA_calendarWidget`, `RESA_pushButton_reserver`, `RESA_pushButton_suppr_resa` |
| TC-040 à TC-043 | `SORTIE_lineEdit_utinfo`, `SORTIE_lineEdit_resa_mdp`, `SORTIE_checkBox_resa_mdpadmin`, `SORTIE_pushButton_sortir`, `pushButton_restituerKit`, vue `PopupSortirResa` |

Les boutons génériques de dialogue (`pushButton_OK`, `pushButton_sortir`) doivent toujours être recherchés à partir de leur fenêtre parente.

## Fixtures et données modifiées

- [`TC-001-login-session.fixture.yaml`](../../automation/fixtures/TC-001-login-session.fixture.yaml) décrit la fixture et [`TC-001-login-session-checks.sql`](../../automation/sql/TC-001-login-session-checks.sql) ses requêtes de contrôle.
- TC-001 insère puis supprime uniquement `utilisateur.TAD001` et les éventuels `log`, `resa`, `sortie` qui lui seraient rattachés.
- Les autres pilotes insèrent puis suppriment uniquement `TEST_ADMIN`, `TEST_U_010`, `TEST_U_011`, `UINFO_040`, `UINFO_042`, `TEST-K-003`, `TEST-K-020`, `TEST-K-021`, `TEST-K-040`, `TEST-K-042` et leurs lignes liées `log`, `resa`, `sortie`, `item`.
- Il ne modifie pas `uid_resa` ni `uid_sortie` ; le nettoyage est vérifié par le test.
- Aucun mot de passe fonctionnel ou MySQL n'est versionné.

## Commandes de reconstruction et d'exécution

```powershell
$env:MB_TEST_DB_ENABLED = '1'
$env:MB_TEST_DB_DRIVER = 'QMYSQL'
$env:MB_TEST_DB_HOST = 'localhost'
$env:MB_TEST_DB_PORT = '3306'
$env:MB_TEST_DB_NAME = 'db4qtest'
$env:MB_TEST_DB_USER = '<compte-de-test>'
$env:MB_TEST_DB_PASSWORD = '<mot-de-passe-MySQL>'
$env:MB_TEST_ADMIN_PASSWORD = '<mot-de-passe-du-compte-fixture>'
$env:QT_QPA_PLATFORM = 'offscreen'

& 'C:\Qt\Tools\CMake_64\bin\cmake.exe' --preset qt-6.11.2-mingw-debug
& 'C:\Qt\Tools\CMake_64\bin\cmake.exe' --build build/Desktop_Qt_6_11_2_MinGW_64_bit_Debug --target micro_brico_campaign_cases
& 'C:\Qt\Tools\CMake_64\bin\ctest.exe' --test-dir build/Desktop_Qt_6_11_2_MinGW_64_bit_Debug -R '^CAMP-001\.' --output-on-failure
```

## Limites restantes

1. La fermeture existante de `MainWindow` crash dans la chaîne `LogBrowser` / `log_stuffs` : un événement de log différé atteint l'IHM après sa destruction. TC-001 termine son processus après le nettoyage de fixture afin de ne pas tester ce chemin hors périmètre. Il faut corriger ce défaut avant des tests créant/détruisant plusieurs fenêtres.
2. La déconnexion retire l'accès dans l'IHM, comme exigé par TC-001, mais le code courant ne vide pas le jeton persistant. Le contrôle SQL C3 reste donc observationnel tant que la règle de révocation n'est pas définie.
3. Les quatre fiches restantes sont explicitement en attente, avec leurs blocages fonctionnels et de fixture, dans [`automation-status.md`](automation-status.md). Toute nouvelle fixture devra garder la même isolation `db4qtest` et être nettoyée après chaque cas.
4. Les fiches restent soumises aux corrections et à la nouvelle revue décrites dans [`review.md`](review.md).
