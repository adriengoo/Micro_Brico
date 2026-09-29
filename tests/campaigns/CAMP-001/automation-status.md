# État de l'automatisation — CAMP-001

Statut au 2026-09-22 : 12 fiches sur 13 sont automatisées et validées. La fiche restante est explicitement en attente : aucun verdict PASS n'est simulé.

## Compte rendu de campagne

Depuis le 2026-09-29, [`Run-CAMP-001.ps1`](../../../Run-CAMP-001.ps1) produit un rapport JUnit XML horodaté à chaque exécution dans [`reports/`](reports/). Il indique le commit, les tags qui pointent sur ce commit, la branche, l'état de l'arbre Git et les étapes détaillées de chaque contrôle CTest. Les fichiers générés restent locaux et ne sont pas versionnés ; voir [`reporting.md`](reporting.md).

## Exécution validée

La commande suivante a été exécutée dans un environnement où les variables `MB_TEST_DB_*` désignent exclusivement `localhost:3306/db4qtest` avec le compte de test, et où `QT_QPA_PLATFORM=offscreen` :

```powershell
& 'C:\Qt\Tools\CMake_64\bin\ctest.exe' --test-dir build/Desktop_Qt_6_11_2_MinGW_64_bit_Debug --output-on-failure -R '^CAMP-001\.'
```

Résultat : **13 contrôles CTest passés sur 13** (le garde-fou sans SQL, TC-001 et les 11 cas ci-dessous). Les cas IHM sont marqués `RUN_SERIAL` et lancés dans un processus distinct, afin de préserver l'isolation malgré le défaut de fermeture existant de `MainWindow`.

| Fiche | Statut | Couverture automatisée réelle |
| --- | --- | --- |
| TC-001 | PASS | Connexion administrateur, session IHM et garde-fou de configuration. |
| TC-002 | PASS | Mot de passe invalide : accès non accordé et jeton inchangé. |
| TC-003 | PASS | Jeton invalidé via fixture, puis action IHM `RESA_pushButton_resa_showResa` : retour à l'état déconnecté et accès protégés retirés. |
| TC-010 | PASS | Création avec caution/adhésion, recherche par utinfo puis nom, persistance de l'e-mail. |
| TC-011 | PASS | Utinfo déjà existant : aucune ligne, e-mail ou log parasite créé. |
| TC-020 | PASS | Création du kit, affichage de ses deux items, recherche par code et contrôle des trois quantités persistées. |
| TC-021 | PASS | Kit `En panne` visible mais refusé à l'ajout au panier de réservation. |
| TC-031 | PASS | Fixture réelle `UINFO_031`, réservation `31031`, témoin `31032`, désactivation, disponibilité à J+7, intégrité du compteur et trace utilisateur+kit. |
| TC-040 | PASS | Sortie standard bloquée sans validation du mot de passe bénéficiaire ; aucune sortie ni mouvement de stock. |
| TC-041 | PASS | Sortie standard : `quantity=2`, `quantity_out=2`, `quantity_init=2`, réservation active et traces sans signature forcée. |
| TC-042 | PASS | Signature administrateur : la sortie reste rattachée au bénéficiaire, la réservation demeure active et le log de délégation existe. |
| TC-043 | PASS | Restitution complète : sortie désactivée, `quantity=2`, `quantity_out=0`, `quantity_init=2` et traces contrôlées. |

## Données et remise à zéro

Le pilote [`tc_campaign_cases.cpp`](../../automation/tc_campaign_cases.cpp) ouvre une connexion QMYSQL nommée `camp001_cases_fixture_db4qtest`, dont la configuration refuse tout nom de base autre que `db4qtest`. L'application sous test reçoit séparément `microbrico_test_db4qtest`.

Chaque cas crée seulement des données synthétiques nommées, puis les supprime dans `cleanup`, y compris après un échec Qt Test : utilisateurs `TEST_ADMIN`, `TEST_U_010`, `TEST_U_011`, `UINFO_031`, `UINFO_040`, `UINFO_041`, `UINFO_042`, `UINFO_043`, kits `TEST-K-003`, `TEST-K-020`, `TEST-K-021`, `TEST-K-031`, `TEST-K-31W`, `TEST-K-040`, `TEST-K-041`, `TEST-K-042`, `TEST-K-043`, leurs items, réservations, sorties et logs liés. Aucun accès à `dbtest` n'est effectué. Les mots de passe administrateur proviennent des variables d'environnement et ne sont pas versionnés.

Les sorties IHM créent également une valeur séquentielle dans `uid_sortie`, conformément au comportement applicatif. Ce compteur technique global n'est pas remis à zéro par les fixtures ; les données fonctionnelles synthétiques, elles, le sont.

## Fiches laissées en attente

| Fiche | Obstacle vérifié |
| --- | --- |
| TC-030 | L'utinfo de fixture est désormais compatible (`UINFO_030`), mais la fiche demande J+7 alors que le calendrier de réservation ne permet que le vendredi. |

Note TC-031 : la fixture respecte littéralement J+7. Quand cette date n'est pas un vendredi, le pilote règle le `QCalendarWidget` par API pour contrôler le filtre de disponibilité réel sur J+7 ; l'IHM utilisateur reste limitée au vendredi, défaut déjà consigné pour TC-030.

Avant de poursuivre, il faut adapter la règle de date de TC-030 au calendrier disponible. Les préconisations de [`review.md`](review.md) restent applicables.
