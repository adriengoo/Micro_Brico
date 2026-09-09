# Mission : agent « socle technique de test » — phase de défrichage

Prépare le socle d'automatisation de CAMP-001 à partir du dépôt et des fichiers sous `tests/`.

## Hypothèse future

Une base MySQL de test, distincte de `dbtest`, sera créée ultérieurement par le responsable de campagne. Elle aura la même structure, mise à jour via Liquibase.

## Interdiction stricte pour cette phase

Ne te connecte à aucune base MySQL. N'exécute aucune requête SQL, aucun script Liquibase, aucun test qui tenterait d'ouvrir une connexion QMYSQL, et ne modifie aucune base.

## Livrables et actions autorisées

1. Cartographier les widgets et actions IHM nécessaires aux fiches dans `tests/campaigns/CAMP-001/technical-foundation.md`.
2. Mettre en place, si cela peut être fait sans ouvrir de base, une cible Qt Test/CTest et une configuration de connexion de test future, sélectionnée explicitement par variables d'environnement ou paramètres documentés. La configuration normale de l'application doit rester inchangée par défaut.
3. Créer des modèles versionnés de fixtures et de contrôles SQL, sans les exécuter, avec les champs déterministes nécessaires aux P0.
4. Préparer un test pilote correspondant à TC-001 uniquement s'il échoue clairement ou se met volontairement en attente lorsque la configuration de test est absente ; il ne doit jamais tomber silencieusement sur `dbtest`.
5. Documenter les commandes qui seront à exécuter plus tard, après création de la base de test, sans les lancer maintenant.

Ne modifie pas les fiches de test, la stratégie, les règles métier, ni les fichiers utilisateur déjà modifiés. Ne démarre ni révision de fiches ni exécution de campagne.
