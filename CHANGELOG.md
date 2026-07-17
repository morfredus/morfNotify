# Journal des versions — morfNotify

Le format s'inspire de [Keep a Changelog](https://keepachangelog.com/fr/1.1.0/)
et du [versionnage sémantique](https://semver.org/lang/fr/).

## [0.1.0] — 2026-07-16

### Ajouté

- **Service autonome de diffusion de notifications** : point unique de fan-out
  pour l'écosystème morfSystem. Ne détecte rien, ne décide de rien, ne connaît
  aucun projet — il reçoit, valide et distribue.
- **API HTTP** : `POST /notify` (title, message, level, targets → `202 Accepted`,
  `400` si invalide), `GET /targets`, `GET /status` (compatible morfBeacon),
  `GET /healthz`.
- **Point d'extension `INotifier`** (QObject asynchrone) + fabrique
  `NotifierFactory` : ajouter une destination = une classe + une ligne.
- **Destinations fournies** : `log` (journald + fichier), `webhook` (POST HTTP,
  format `json` ou `ntfy` pour push type Pixel).
- **Livraison fire-and-forget** : le producteur n'attend jamais une destination
  lente ; les noms de cibles inconnus sont signalés sans être une erreur.
- **Annonce LAN via morfBeacon** (embarqué/vendoré dans `third_party/morf/beacon`,
  lié statiquement) : découverte automatique par le parc.
- **Démon `morfnotify`** (config JSON, `--config`, `--list-types`, repli sur une
  destination `log` si aucune config).
- **Service systemd** : `install-service.sh`, `update-service.sh`,
  `morfnotify.service` (installation dans `/opt/morfnotify`).
- Documentation FR (architecture, protocole, intégration).
