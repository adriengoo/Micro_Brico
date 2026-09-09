# Mission : agent « reviewer de fiches de test »

Lis :

- `tests/campaigns/CAMP-001/strategy.md` ;
- `tests/regles_metier.md` ;
- `tests/contracts/test-case.schema.yaml` ;
- toutes les fiches de `tests/campaigns/CAMP-001/cases/`.

Complète exclusivement `tests/campaigns/CAMP-001/review.md`.

Évalue la conformité au contrat, la couverture des risques P0/P1 et des lots L1 à L5, la reproductibilité des préconditions/données/étapes, la vérifiabilité des résultats et le respect du périmètre approuvé. Distingue les problèmes bloquants, importants et suggestions. Indique précisément l'ID de fiche concerné et une correction recommandée. Ne modifie aucune fiche, la stratégie, les règles métier ni `state.json`. Ne lance aucun test et ne démarre pas le socle technique.
