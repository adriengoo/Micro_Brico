# Stratégie de test — CAMP-001

Statut : **validée le 2026-09-06**.

Ce document est le livrable exclusif de l'agent « stratégie de test ». La conception des fiches est autorisée ; leur revue et leur exécution restent soumises à validation explicite.

## 1. Objet et périmètre observé

Cette campagne cible l'application de bureau **MicroBrico**, construite en C++17 avec Qt Widgets. Le dépôt montre une application de gestion de prêts de kits, connectée directement à une base MySQL.

Fonctionnalités observées dans le code :

| Domaine | Capacités observées |
| --- | --- |
| Connexion | Authentification par `utinfo` et mot de passe, calcul SHA-1 côté client, jeton de session conservé dans `utilisateur.token`, déconnexion lorsqu'un jeton ne correspond plus. |
| Utilisateurs | Consultation, recherche par nom ou utinfo, ajout, modification, suppression et consultation des logs utilisateur. Les attributs manipulés comprennent identité, courriel, téléphone, privilège, caution, adhésion et date de caution. |
| Kits et items | Consultation, recherche par nom/code, ajout, modification, duplication, affichage des items et des logs associés. Un kit porte notamment un code unique, un état de panne et une liste d'items avec quantités. |
| Réservations | Recherche de kits, panier, choix de date via calendrier, association à un utilisateur par utinfo, création et visualisation de réservations, retrait d'un kit d'une réservation, désactivation des réservations dépassées. |
| Sorties et restitutions | Affichage des réservations et sorties actives d'un utilisateur, validation par mot de passe utilisateur ou administrateur, sortie d'un kit, sortie immédiate depuis le panier, restitution avec contrôle des quantités d'items, retrait d'un kit d'une réservation. |
| Traçabilité | Écriture de logs reliés à un utilisateur et/ou un kit ; consultation de logs applicatifs, utilisateur et kit via les fenêtres dédiées. |

Le périmètre ne couvre pas, à ce stade, les scripts de packaging/installateur ni l'import CSV (`script/copy_csv_to_db.py`) : leur comportement métier et leur mode d'emploi ne sont pas décrits dans le code C++ principal. Ils pourront faire l'objet d'une campagne séparée.

## 2. Dépendances, environnement et données nécessaires

### Éléments observés

- Le projet CMake réclame Qt Widgets (Qt 5 ou Qt 6 pour Widgets, puis explicitement Qt 6 SQL), C++17 et génère l'exécutable `micro_brico_app`.
- L'accès aux données passe par `QSqlDatabase` / `QSqlQuery`. La connexion attend une connexion Qt nommée `dbtest` (`QSqlDatabase::database("dbtest")`). La création de cette connexion n'est pas visible dans les fichiers examinés et doit être confirmée.
- Le README indique MySQL 8.0.35, un service local sur le port 3306 et l'usage de Liquibase. Il documente une base de développement `dbtest` et des comptes locaux d'exemple.
- Le changelog Liquibase crée les tables `utilisateur`, `kit`, `item`, `resa`, `sortie`, leurs compteurs `uid_resa`/`uid_sortie` et `log`, avec des contraintes et index partiels.
- L'installateur contient une archive du pilote Qt MySQL pour Qt 6.7.3 / MinGW 64 bits, alors que le README cite Qt 6.11.2. La compatibilité exacte du pilote est donc à valider sur la machine de test.

### Préconditions de campagne proposées

1. Construire l'application avec une version Qt 6 cohérente, le module Qt SQL et un pilote `QMYSQL` compatible avec le compilateur et l'architecture utilisés.
2. Créer une base **dédiée aux tests**, jamais la base de production, et y appliquer le changelog avec Liquibase.
3. Créer ou charger un jeu de données réinitialisable comprenant au minimum : un administrateur, un utilisateur standard, un utilisateur sans adhésion/caution si ces règles sont effectivement attendues, plusieurs kits dont un en panne, et des kits à un ou plusieurs items.
4. Initialiser les tables de compteurs `uid_resa` et `uid_sortie` avec une valeur documentée : le code lit la dernière valeur puis l'incrémente.
5. Prévoir un mécanisme de remise à zéro reproductible de la base (recréation + Liquibase + données de référence, ou sauvegarde restaurable) entre lots qui modifient les réservations, sorties ou quantités.
6. Collecter les sorties Qt (`qDebug`/`qInfo`) et les erreurs SQL pendant l'exécution ; elles constituent actuellement la source de diagnostic la plus directe.

## 3. Risques et priorités

Les constats techniques ci-dessous sont observés dans le dépôt ; l'impact utilisateur doit être confirmé par exécution sur une base de test.

| Priorité | Risque | Éléments observés | Orientation de validation |
| --- | --- | --- | --- |
| P0 | Intégrité des prêts, retours et stocks | Une sortie met à jour les quantités d'items, crée une sortie et des logs ; une restitution recalcule les quantités puis désactive la sortie. Les opérations regroupent plusieurs requêtes. | Vérifier l'état final de chaque table et des quantités après succès, annulation et erreur ; contrôler l'absence de demi-opération. |
| P0 | Cohérence réservation/sortie et concurrence | Les numéros sont calculés depuis `uid_resa`/`uid_sortie` sous `LOCK TABLES`. Les flux réservation et sortie manipulent plusieurs tables. | Tester plusieurs kits dans une même réservation, kit déjà réservé/sorti, annulation, et au moins un scénario avec deux sessions si le produit est multi-poste. |
| P0 | Autorisation de sortie/restitution | L'IHM exige une validation de mot de passe pour sortir ou restituer, avec une option de signature administrateur. | Vérifier les refus sans validation, mot de passe erroné, validation utilisateur et validation administrateur, ainsi que la traçabilité. |
| P1 | Authentification et session | Le mot de passe est comparé sous SHA-1 ; un jeton de session est stocké en base et relu avant les requêtes via `is_user_connected`. | Vérifier connexion valide/invalide, déconnexion, conflit de session et restrictions réelles des actions selon le privilège. Évaluer ensuite la conformité sécurité : SHA-1 est obsolète pour le stockage de mots de passe. |
| P1 | Persistance et saisies | De nombreuses requêtes SQL sont construites par concaténation de texte à partir de champs utilisateur (utilisateur, kit, item, recherche, log). | Tester caractères spéciaux, apostrophes, chaînes longues, doublons (utinfo, code kit), valeurs limites et comportements en erreur SQL. Le risque d'injection SQL doit être traité comme un sujet de sécurité à corriger, pas seulement comme un test d'acceptation. |
| P1 | Cycle de vie des réservations | Les réservations dont la date est antérieure à la date courante sont désactivées ; un retrait désactive toutes les lignes portant l'id d'un kit. | Vérifier dates passée/présente/future, fuseau/date système, annulation partielle et absence d'effet sur une autre réservation du même kit. |
| P1 | État des kits | L'état réservé est calculé pour une date, l'état sorti depuis les sorties actives, et l'état en panne est stocké sur le kit. | Vérifier les combinaisons disponible/réservé/sorti/en panne et les filtres/recherches ; confirmer les règles métier d'interdiction attendues. |
| P2 | Traçabilité | Les logs sont ajoutés lors des modifications et des sorties/restitutions ; ils sont affichés par kit/utilisateur. | Vérifier présence, rattachement, ordre, contenu utile et absence de fuite de données sensibles. |
| P2 | Robustesse IHM | Les flux utilisent listes, sélections, fenêtres popup et états locaux de panier. | Vérifier sélections absentes, fenêtres fermées/annulées, répétition rapide d'actions, rafraîchissement après modification et messages utilisateur. |
| P2 | Installation/compatibilité | Les versions Qt et pilote MySQL documentées ne sont pas alignées. | Vérifier lancement sur une machine propre, détection du pilote SQL et message exploitable si la base ou le pilote est indisponible. |

## 4. Niveaux de test recommandés

1. **Tests unitaires C++ (à introduire)** : classes de valeur (`Money`, `Item`, `Kit`, `Resa`, `Sortie`, `Utilisateur`), calcul de quantités lors de restitution et règles de date. Utiliser Qt Test (`Qt6::Test`) ; ces tests ne doivent pas dépendre de MySQL.
2. **Tests d'intégration base de données** : `Connect_db` contre une instance MySQL de test migrée par Liquibase. Vérifier les insertions, mises à jour, contraintes, erreurs et cohérence de l'état en base.
3. **Tests fonctionnels IHM** : parcours Qt Widgets depuis connexion jusqu'à réservation, sortie et restitution. Vérifier à la fois l'IHM et les données persistées.
4. **Tests exploratoires orientés risque** : incohérences de stock, annulations, perte de connexion, saisies imprévues, sessions concurrentes et disponibilité du pilote/base.
5. **Régression** : exécuter à chaque modification les unités, puis les intégrations et un sous-ensemble IHM couvrant connexion, réservation et retour.

## 5. Approche d'automatisation réaliste

- Ajouter une cible de tests Qt Test dans CMake et enregistrer les exécutables avec CTest. Le dépôt ne déclare actuellement ni `enable_testing()`, ni cible de test, ni framework de test.
- Isoler progressivement l'accès SQL de la logique d'IHM : pour les tests unitaires, fournir un dépôt/faux service au lieu d'appeler la base globale `g_connect_db`.
- Pour les tests d'intégration, démarrer une base MySQL éphémère ou dédiée, appliquer Liquibase avant chaque suite et recharger un jeu de données minimal connu. Les tests doivent interroger la base pour vérifier les effets, pas uniquement l'affichage.
- Pour l'IHM, exploiter les `objectName` déjà générés dans les `.ui` (par exemple `RESA_pushButton_reserver`, `SORTIE_pushButton_sortir`) avec Qt Test : `QTest::mouseClick`, saisie clavier, recherche de widgets et validation de dialogues. Ajouter des `objectName` explicites aux contrôles des popups si ceux-ci sont insuffisants.
- Exécuter les scénarios GUI de manière sérielle : la base, les tables verrouillées et les compteurs globaux rendent le parallélisme risqué. Les unitaires purs peuvent rester parallèles.
- Produire des artefacts exploitables : journal applicatif, résultat CTest/JUnit si disponible, export SQL ciblé ou requêtes de contrôle à la fin des scénarios critiques, et captures uniquement pour les échecs IHM.

## 6. Proposition de lots de tests

| Lot | But | Priorité | Critère de sortie du lot |
| --- | --- | --- | --- |
| L1 — Socle et connexion | Démarrage, disponibilité MySQL/pilote, connexion, déconnexion et session. | P0/P1 | Les actions protégées sont accessibles uniquement avec une session valide et les erreurs d'environnement sont visibles. |
| L2 — Gestion utilisateurs | Recherche, création, modification, suppression, doublons et droits. | P1 | Les données affichées et persistées concordent ; les règles de privilèges sont confirmées. |
| L3 — Catalogue kits/items | Création, modification, duplication, recherche, panne, items et quantités initiales. | P1 | Les codes uniques, données d'items et états de kit restent cohérents. |
| L4 — Réservations | Panier, date, utilisateur, création, affichage, annulation et expiration. | P0 | Chaque réservation affecte les bons kits/utilisateur/date sans rendre disponible un kit qui ne devrait pas l'être. |
| L5 — Sorties, restitutions et stock | Signature, sortie planifiée/immédiate, contrôle des items, retour et logs. | P0 | L'état des sorties, stocks, réservations et logs est cohérent après chaque parcours. |
| L6 — Robustesse et régression | Cas limites, perte de session/base, caractères spéciaux, concurrence et parcours de régression. | P1/P2 | Pas de corruption de données ; les erreurs sont maîtrisées et les P0 restent verts. |

## 7. Hypothèses et points à confirmer avant les fiches de test

- La cible exacte (version Qt, compilateur, architecture Windows et pilote QMYSQL) doit être arrêtée ; les références Qt 6.7.3 et 6.11.2 du dépôt divergent.
    - Il s'agit bien de qt 6.11.2 su windows 11
- Le mécanisme qui crée et ouvre la connexion Qt `dbtest` doit être localisé ou documenté. Sans lui, les tests d'intégration ne peuvent pas être fiabilisés.
    - Est il possible d'ouvrir une base de donnée type sqlite qui serait mise à jour/ renseignée/ réinitialisée avec les données utiles à chaque test? afin de ne pas toucher à la base mysql dbtest.
- Les règles métier d'éligibilité (adhésion/caution à jour, utilisateur standard contre administrateur, kit en panne, kit réservé à une autre personne) ne sont pas explicitement imposées de façon évidente dans les fichiers examinés ; elles doivent être validées avec le métier.
    - Fichier regles_metier.md à créer
- La politique attendue pour une réservation contenant plusieurs kits, une annulation partielle, une sortie immédiate puis son annulation, et les réservations passées doit être confirmée.
    - ne pas tester cela pour l'instant
- Le modèle de concurrence doit être précisé : application mono-poste ou plusieurs clients pointant vers la même base. Cela détermine la profondeur des tests de verrouillage et de session.
    - Application multiposte sur réseau local accedant à la même base mysql. La stratégie de test ne doit pas mettre en oeuvre de tests aussi poussés que des tests multi instances de l'application qui fonctionneraient en parallèle.
- La stratégie de sécurité doit être arbitrée : SHA-1 et la concaténation SQL sont présents dans le code ; il faut définir si CAMP-001 constate ces risques ou si elle doit aussi vérifier leur correction dans une évolution dédiée.
    - on reste sur du sha-1, application vulnérable mais le risque de piratage est faible étant donné son déploiement dans un réseau local fermé.
- Les données de test autorisées, comptes de test et méthode de remise à zéro doivent être fournis avant toute exécution automatisée modifiant MySQL.
    - Le script ../../script/copy_csv_to_db.py permet de populer la base de donnée complète avec tous les véritables articles. Mais pour des tests, le testeur doit pouvoir générer des jeux de données nécéssaire uniquement à son test. 
