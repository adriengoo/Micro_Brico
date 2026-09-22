# État de l'automatisation — CAMP-001

Statut au 2026-09-22 : 10 fiches sur 13 sont automatisées et validées. Les 3 autres sont explicitement en attente : aucun verdict PASS n'est simulé.

## Exécution validée

La commande suivante a été exécutée dans un environnement où les variables `MB_TEST_DB_*` désignent exclusivement `localhost:3306/db4qtest` avec le compte de test, et où `QT_QPA_PLATFORM=offscreen` :

```powershell
& 'C:\Qt\Tools\CMake_64\bin\ctest.exe' --test-dir build/Desktop_Qt_6_11_2_MinGW_64_bit_Debug --output-on-failure -R '^CAMP-001\.'
```

Résultat : **11 contrôles CTest passés sur 11** (le garde-fou sans SQL, TC-001 et les 9 cas ci-dessous). Les cas IHM sont marqués `RUN_SERIAL` et lancés dans un processus distinct, afin de préserver l'isolation malgré le défaut de fermeture existant de `MainWindow`.

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
| TC-042 | PASS | Signature administrateur : la sortie reste rattachée au bénéficiaire, la réservation demeure active et le log de délégation existe. |

## Données et remise à zéro

Le pilote [`tc_campaign_cases.cpp`](../../automation/tc_campaign_cases.cpp) ouvre une connexion QMYSQL nommée `camp001_cases_fixture_db4qtest`, dont la configuration refuse tout nom de base autre que `db4qtest`. L'application sous test reçoit séparément `microbrico_test_db4qtest`.

Chaque cas crée seulement des données synthétiques nommées, puis les supprime dans `cleanup`, y compris après un échec Qt Test : utilisateurs `TEST_ADMIN`, `TEST_U_010`, `TEST_U_011`, `UINFO_031`, `UINFO_040`, `UINFO_042`, kits `TEST-K-003`, `TEST-K-020`, `TEST-K-021`, `TEST-K-031`, `TEST-K-31W`, `TEST-K-040`, `TEST-K-042`, leurs items, réservations, sorties et logs liés. Aucun accès à `dbtest` n'est effectué. Les mots de passe administrateur proviennent des variables d'environnement et ne sont pas versionnés.

## Fiches laissées en attente

| Fiche | Obstacle vérifié |
| --- | --- |
| TC-030 | L'utinfo de fixture est désormais compatible (`UINFO_030`), mais la fiche demande J+7 alors que le calendrier de réservation ne permet que le vendredi. |
| TC-041 | Un parcours IHM complet a été exécuté : sortie créée, réservation encore active et `quantity_out=2`, mais `item.quantity` reste à 2 au lieu de 0. `Connect_db::update_items_quantity_of_kit()` persiste la quantité courante de l'item de popup sans la diminuer. La fiche reste en attente de correction produit. |
| TC-043 | La restitution complète ne restaure pas le stock : `SORTIE_calculate_remaining_quantity()` laisse `quantity` à 0 lorsque la quantité restituée est égale à `quantity_out`, puis remet seulement `quantity_out` à 0. La fiche reste en attente de correction produit ; sa fixture de sortie active doit ensuite être formalisée. |

Note TC-031 : la fixture respecte littéralement J+7. Quand cette date n'est pas un vendredi, le pilote règle le `QCalendarWidget` par API pour contrôler le filtre de disponibilité réel sur J+7 ; l'IHM utilisateur reste limitée au vendredi, défaut déjà consigné pour TC-030.

Avant de poursuivre, il faut adapter la règle de date de TC-030 au calendrier disponible et corriger les mouvements de stock de sortie/restitution. Les préconisations de [`review.md`](review.md) restent applicables.
