# Règles métier — MicroBrico

Statut : **validé le 2026-09-07**.

Ce document est la référence des règles fonctionnelles à associer aux fiches de test. Une règle est promue au statut « validée » seulement après confirmation par le responsable métier.

| ID | Règle validée | Statut |
| --- | --- | --- |
| RM-001 | Un utilisateur doit avoir payé sa caution pour pouvoir effectuer une réservation dans l'outil. Une caution marquée « payée » vaut adhésion. L'absence de caution payée ne bloque pas une sortie ni une restitution : l'application doit permettre une sortie immédiate supervisée par un administrateur, ainsi qu'une restitution. | Validée |
| RM-002 | Seul un administrateur connecté à l'application peut valider une sortie ou une restitution. Lors d'une sortie ou restitution standard, l'utilisateur non administrateur s'identifie avec son utinfo et son mot de passe dans la session de l'administrateur. Lorsqu'un administrateur coche la case « signature administrateur » pour une sortie, l'utilisateur bénéficiaire renseigne son utinfo mais n'a pas à saisir son mot de passe. | Validée |
| RM-003 | Un kit sorti peut être réservé. Le produit suppose qu'un kit n'est pas sorti plus d'une semaine par utilisateur et sera rendu le jour de la sortie suivante. Un kit déjà réservé ne peut pas être réservé par un autre utilisateur si la date de début est identique. Aucune limite de réservations d'un même utilisateur sur un même kit n'est prévue actuellement. Un kit marqué en panne par un administrateur, dans le menu de gestion des kits, ne peut être ni réservé ni sorti. | Validée |
| RM-004 | La suppression des réservations passées depuis le menu maintenance est hors périmètre de CAMP-001. | Hors périmètre CAMP-001 |
| RM-005 | `quantity` représente la quantité totale d'un item dans le kit ; `quantity_out` représente la quantité déclarée sortie. Lors d'une sortie, `quantity` reste inchangée et `quantity_out` est fixé à la quantité effectivement sortie, qui peut être inférieure à `quantity`. Lors d'une restitution complète, `quantity` reste inchangée et `quantity_out` revient à zéro. Lorsqu'une restitution indique une quantité inférieure à celle déclarée à la sortie, la quantité totale `quantity` est diminuée de l'écart correspondant aux objets non restitués, puis `quantity_out` revient à zéro. Une restitution ne peut pas dépasser la quantité initialement sortie ni rendre `quantity` négative. Une restitution partielle n'empêche pas une réservation ou une sortie ultérieure. | Validée |
| RM-006 | Une sortie, même lorsque tous les kits d'une réservation sont sortis, ne désactive ni ne supprime la réservation : elle reste active. La réservation est désactivée seulement lorsqu'un administrateur annule la réservation via le bouton prévu ou lorsqu'il désactive les réservations passées depuis le menu maintenance. Lors de l'annulation d'une réservation, tous les objets de cette réservation redeviennent réservables à la date de début concernée. | Validée |

## Implications pour les fiches de test

- Une réservation doit refuser un utilisateur dont la caution n'est pas payée ; une sortie ou restitution supervisée par un administrateur reste permise dans ce cas.
- Les sorties et restitutions doivent vérifier la session administrateur et l'identification de l'utilisateur bénéficiaire non administrateur. Pour une sortie avec « signature administrateur », seul l'utinfo du bénéficiaire est requis.
- Les tests de disponibilité doivent couvrir : kit en panne marqué par un administrateur refusé à la réservation et à la sortie ; conflit de réservation seulement à date de début identique ; réservation autorisée d'un kit actuellement sorti.
- Les tests de sortie doivent vérifier que `quantity` reste inchangée et que `quantity_out` reflète exactement la quantité sortie. Les tests de restitution doivent vérifier que `quantity` ne diminue qu'en cas d'objets non restitués, que `quantity_out` revient à zéro et qu'un retour partiel ne bloque pas une réservation ou sortie ultérieure.
- L'annulation d'une réservation doit rendre réservables, à la même date de début, tous les objets qu'elle contient.
- Une sortie ne doit pas désactiver ni supprimer la réservation source ; les tests de sortie doivent donc vérifier qu'elle demeure active.

## Décisions déjà actées pour CAMP-001

- Environnement cible : Windows 11 avec Qt 6.11.2.
- Déploiement : plusieurs postes accèdent à la même base MySQL sur un réseau local ; CAMP-001 ne met pas en œuvre de scénarios simultanés multi-instances.
- Sécurité : SHA-1 reste en place pour cette campagne ; le risque est consigné mais aucun test de correction cryptographique n'est prévu.
- Les scénarios de réservation à plusieurs kits, annulation partielle, sortie immédiate suivie d'annulation et maintenance des réservations passées sont hors périmètre initial.
- Les jeux de données de test doivent être minimaux, dédiés et réinitialisables pour chaque besoin de test. Le script `script/copy_csv_to_db.py` peut peupler l'ensemble des articles mais ne remplace pas ces jeux ciblés.
