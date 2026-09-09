-- CAMP-001 / TC-001 — SQL controls for a future dedicated MySQL test schema.
-- TEMPLATE ONLY. Do not execute during the technical-foundation discovery phase.
-- Bind :utinfo to the fixture value TAD001 in the future test runner.

-- C1: fixture account exists, is an admin and starts without a session token.
SELECT id, utinfo, privilege, adhesion_payed, token
FROM utilisateur
WHERE utinfo = :utinfo;

-- C2: successful login must assign a non-empty token to this account only.
SELECT id, utinfo, token
FROM utilisateur
WHERE utinfo = :utinfo;

-- C3: observe the token after logout. TC-001's accepted criterion is IHM
-- deauthentication; current code does not clear this persisted token yet.
SELECT id, utinfo, token
FROM utilisateur
WHERE utinfo = :utinfo;

-- C4: TC-001 must not create lending domain data as a side effect.
SELECT
    (SELECT COUNT(*) FROM resa WHERE id_user = u.id AND is_active = 1) AS active_reservations,
    (SELECT COUNT(*) FROM sortie WHERE id_user = u.id AND is_active = 1) AS active_checkouts
FROM utilisateur AS u
WHERE u.utinfo = :utinfo;
