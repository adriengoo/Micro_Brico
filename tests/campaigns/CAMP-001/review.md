# Revue des fiches de test — CAMP-001

Date : 2026-09-06  
Statut : **à corriger avant validation de la revue**

## Synthèse

Les 13 fiches respectent la structure prescrite par `test-case.schema.yaml` : elles possèdent toutes un identifiant, une priorité, une exigence, des préconditions, données, étapes avec résultat attendu, critères PASS et indication d'automatisation. Elles couvrent les lots L1 à L5 et les parcours principaux P0 de réservation, sortie et restitution.

La revue ne peut toutefois pas être validée en l'état. Certaines fiches présentent comme attendues des règles métier explicitement non validées, et plusieurs scénarios P0 ne donnent pas un jeu de données ni des contrôles de persistance suffisamment déterministes pour une future automatisation. Les corrections ci-dessous doivent être appliquées aux fiches avant le démarrage du socle technique.

## Problèmes bloquants

| Fiche | Constat | Correction recommandée |
| --- | --- | --- |
| TC-021 | Le résultat attendu impose qu'un kit en panne ne soit pas ajoutable, alors que RM-003 est « À confirmer ». La note ne suffit pas : le verdict PASS/FAIL dépend d'une règle non approuvée. | Mettre la fiche en attente de validation de RM-003, ou reformuler son objectif comme un constat du comportement actuel, sans déclarer l'interdiction attendue. |
| TC-042 | Le scénario affirme qu'un administrateur peut signer pour un autre utilisateur, alors que ce droit relève de RM-002, non validée. | Mettre la fiche en attente de RM-002 ou transformer le résultat en observation à faire valider par le métier avant exécution. |
| TC-043 | La nature du signataire et les conditions de restitution sont indéterminées (RM-001/RM-002) alors que le test exige une restitution réussie. | Préciser le signataire, les conditions d'éligibilité et le mot de passe attendus après validation des règles ; à défaut, placer la fiche en attente. |
| TC-030, TC-031, TC-040 à TC-043 | Les données de référence ne sont pas assez définies pour une remise à zéro automatisée : identifiants techniques réels, mots de passe/hachages, valeurs de caution/adhésion, lignes `resa`/`sortie`, dates exactes et état initial des compteurs ne sont pas spécifiés. `RESA_031` n'est notamment pas un identifiant de base exploitable. | Pour chaque fiche, fournir un fixture nominatif et déterministe (SQL, YAML ou fabrique de données) : valeurs de chaque ligne, dates absolues ou calculées selon une règle, identifiant de réservation/sortie réel ou critère de recherche univoque, et état initial attendu des compteurs. |

## Problèmes importants

| Fiche ou couverture | Constat | Correction recommandée |
| --- | --- | --- |
| TC-001 | La fiche est P0 mais ne vérifie pas le démarrage avec pilote/base indisponibles, pourtant inclus dans le lot L1 et dans le risque d'environnement de la stratégie. | Ajouter un cas L1 distinct de diagnostic de connexion/pilote, ou déclarer explicitement ce contrôle hors CAMP-001. |
| TC-003 | L'« action nécessitant un accès à la base » est seulement un exemple. Selon l'écran choisi, elle peut ne pas exercer le contrôle de jeton ; le test est donc non déterministe. | Nommer une action et le widget précis, puis définir la requête SQL de modification du jeton et le statut/UI exact qui démontre la déconnexion. |
| TC-010, TC-011 | Les « données obligatoires » et les valeurs attendues en base sont vagues ; TC-011 ne prouve pas que les autres données soumises ne sont pas partiellement créées. | Lister tous les champs réellement fournis par le fixture et contrôler explicitement l'absence de nouvelle ligne, ainsi que l'absence d'effets annexes/logs inattendus. |
| TC-020 | La fiche couvre la création mais pas l'unicité du code, la recherche par nom, la modification ni la duplication, éléments du lot L3. | Ajouter ou planifier explicitement ces cas avant de revendiquer la couverture L3. |
| TC-030 | Le contrôle du « kit apparaît réservé » et le log ne précisent ni l'écran, ni les champs, ni la ligne de log attendue. | Définir les colonnes/valeurs SQL et le texte ou type de log minimal attendu ; fixer TEST_DATE_030 par une règle sans ambiguïté (par exemple J+7). |
| TC-031 | La précondition affirme qu'une seule réservation existe, mais le résultat exige aussi qu'aucun autre enregistrement de test ne soit altéré sans fournir cet enregistrement témoin. | Ajouter une réservation témoin explicitement définie, ou retirer ce contrôle. Définir également le critère SQL d'« inactive ». |
| TC-040 | « Future ou présente » rend le comportement dépendant de la date et de la règle d'expiration. | Choisir une seule date conforme à RM-004 (après validation) et fournir la réservation fixture exacte. |
| TC-041, TC-043 | Les contrôles de stock parlent de « quantité persistée » sans identifier précisément la colonne/ligne item et sans documenter la relation entre quantité de sortie et quantité du catalogue. | Définir les requêtes de contrôle ou les champs métier à vérifier, l'état initial/complet et les valeurs attendues après chaque opération. |
| Tous les P0 L4/L5 | La stratégie demande la vérification de l'absence de demi-opération et de cas d'erreur/annulation ; aucun scénario ne couvre une erreur pendant l'écriture, une annulation de dialogue de sortie/restitution, ni la cohérence après échec. | Ajouter des fiches P0 ciblées ou inscrire ces risques comme reportés avec justification, avant de considérer la couverture P0 complète. |
| Lots L2 à L5 | La couverture actuelle traite seulement une partie des capacités identifiées : modification/suppression utilisateur, recherche/duplication kit, visualisation/retrait de réservation et retour avec quantité partielle sont absents. | Ajouter les fiches correspondantes, ou réviser le périmètre approuvé de CAMP-001 afin que la couverture annoncée reflète les cas réellement prévus. |

## Suggestions

| Fiche ou ensemble | Suggestion |
| --- | --- |
| Toutes les fiches candidates Qt Test | Ajouter les `objectName` ou le nom de fenêtre/dialogue visé par chaque étape IHM afin que le futur automate ne dépende ni de texte localisé ni de la structure implicite des widgets. |
| TC-002 | Préciser le message ou code d'erreur attendu et vérifier qu'aucun jeton de session n'est écrit pour TEST_ADMIN. |
| TC-041 et TC-042 | Vérifier aussi l'état de la réservation source après sortie, conformément au risque de cohérence réservation/sortie de la stratégie. |
| TC-030 à TC-043 | Associer chaque scénario à une fixture isolée et idempotente ; éviter les données libres ou partagées afin d'autoriser l'exécution sérielle répétée. |
| Priorités | Étiqueter explicitement dans chaque fiche les règles RM bloquantes et la décision de report, pour que les résultats ne soient jamais confondus avec un échec produit. |

## Décision de revue

**Non validée.** Corriger les problèmes bloquants, puis mettre à jour les fiches. Une nouvelle revue est requise avant l'autorisation du socle technique.
