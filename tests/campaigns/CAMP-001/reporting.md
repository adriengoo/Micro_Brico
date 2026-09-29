# Compte rendu JUnit — CAMP-001

Chaque exécution de [`Run-CAMP-001.ps1`](../../../Run-CAMP-001.ps1) produit un compte rendu JUnit XML horodaté dans ce dossier :

```powershell
.\Run-CAMP-001.ps1
```

Le chemin précis est affiché à la fin du lancement, par exemple `tests/campaigns/CAMP-001/reports/CAMP-001_20260929-231500123_f08b889.junit.xml`.

La même exécution produit aussi un tableau de bord HTML autonome, copié sous le nom stable `latest.dashboard.html`. Dans VS Code, ouvrir ce fichier puis choisir **Open in Integrated Browser** donne une vue synthétique de la campagne, proche d'un rendu Jenkins : tuiles de résultat, métadonnées Git, alerte sur un dépôt modifié et détail dépliable des étapes de chaque TC.

La tâche VS Code **CAMP-001: lancer la campagne et produire le dashboard** lance le script sans saisir la commande. Elle est également définie comme tâche de test par défaut.

Les rapports sont **historisés localement**, mais leurs fichiers XML sont ignorés par Git : ils reflètent un lancement et un poste donnés. Aucun mot de passe, variable d'environnement ou chaîne de connexion n'y est inscrit.

## Contenu du rapport

Le document est compatible avec le format JUnit courant (`testsuite`, `testcase`, `failure`, `system-out`, `properties`). Il contient :

- la date et l'heure locale de début du test ;
- le SHA du dernier commit, sa branche et les tags qui pointent exactement sur ce commit (ou `(aucun)`) ;
- un avertissement très visible et la liste des changements si l'arbre Git est modifié ;
- un `testcase` par contrôle CTest effectivement lancé ;
- pour chaque contrôle, les étapes de la fiche YAML correspondante, avec leur objectif, action, résultat attendu et verdict.

Quand un cas réussit, chaque étape est explicitement marquée `[OK]`. En cas d'échec, le pilote Qt émet un marqueur `CAMP001_STEP=<n>` : le rapport marque cette étape `[FAIL]`, les étapes antérieures `[OK]` et les suivantes `[NOT_RUN]`. La sortie Qt/CTest utile est ajoutée sous le cas en échec.

## Lecture rapide

Le rapport peut être ouvert dans un lecteur JUnit, un serveur CI, ou directement dans un éditeur XML. Pour afficher le dernier rapport depuis PowerShell :

```powershell
Get-ChildItem .\tests\campaigns\CAMP-001\reports\*.junit.xml |
  Sort-Object LastWriteTime -Descending |
  Select-Object -First 1 |
  Get-Content
```
