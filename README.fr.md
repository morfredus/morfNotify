# morfNotify

*Lire dans une autre langue : [English](README.md) · **Français** (ce document).*

[![Version](https://img.shields.io/badge/version-0.2.1-blue)](CHANGELOG.md)
![C++](https://img.shields.io/badge/C%2B%2B-17-00599C?logo=cplusplus)
![Qt](https://img.shields.io/badge/Qt-6-41CD52?logo=qt)
![Build](https://img.shields.io/badge/CMake-3.21+-064F8C?logo=cmake)
![License](https://img.shields.io/badge/License-GPL--3.0--only-blue)

**Service autonome de diffusion de notifications pour l'écosystème morfSystem.**

morfNotify **ne détecte rien, ne décide de rien, ne connaît aucun projet**. Sa
seule responsabilité : recevoir un message et le distribuer vers les destinations
demandées. C'est le **point unique de diffusion** des notifications de
l'écosystème — n'importe quel projet (actuel ou futur) peut informer l'utilisateur
avec une intégration minimale, sans connaître les mécanismes d'envoi.

## Philosophie

- **Toute la logique appartient au projet appelant** : *quand*, *quoi* et *à qui*.
- morfNotify **reçoit → valide → distribue → journalise éventuellement**. Rien de plus.
- Il ne surveille aucun service, n'analyse aucune donnée, ne décide jamais qu'une
  notification doit partir, et ne connaît ni SiteWatch, ni MeteoHub, ni morfSensor,
  ni morfCollector…

## API

Volontairement réduite et stable. Un producteur POSTe un message :

```sh
curl -X POST http://localhost:8789/notify \
  -H 'Content-Type: application/json' \
  -d '{
        "title":   "morfSensor",
        "message": "Service arrêté",
        "level":   "warning",
        "targets": ["pixel", "dashboard"]
      }'
```

| Route | Rôle |
|---|---|
| `POST /notify` | reçoit + valide + distribue (`202 Accepted` ; `400` si invalide) |
| `GET /targets` | liste des destinations configurées |
| `GET /status` | compatible morfBeacon (app, version, uptime, metrics) |
| `GET /healthz` | `{ "status": "ok" }` |

Réponse `202` : `{ "accepted": true, "queued": [...], "unknown": [...] }` — les noms
de destinations inconnus sont signalés, jamais bloquants. La livraison est
**fire-and-forget** : le producteur n'attend jamais une destination lente.

## Destinations (enfichables)

Indépendantes les unes des autres, ajoutées sans toucher à l'API. Fournies :

- **`log`** — journal (journald) + fichier optionnel.
- **`webhook`** — POST HTTP vers une URL. `format: "json"` (notification complète)
  ou `format: "ntfy"` (push ntfy.sh — ex. un téléphone/Pixel).
- **`email`** — envoi SMTP (`starttls`, `ssl` ou `none`) vers un ou plusieurs
  destinataires.
- **`telegram`** — notification Telegram via Bot API (`bot_token` + `chat_id`).

Ajouter une destination (e-mail, MQTT, écran dédié…) = une sous-classe `INotifier`
+ une ligne dans la fabrique. Les producteurs continuent d'utiliser exactement la
même API.

## Compiler

Nécessite seulement **Qt 6** (Core, Network). **morfBeacon est embarqué** (vendoré
dans `third_party/morf/beacon`) : aucune dépendance à un dépôt externe.

```sh
cmake --preset mingw        # ou linux / linux-arm64
cmake --build --preset mingw
```

## Lancer

```sh
# Démo immédiate, sans config (une seule destination 'log') :
./build-mingw/service/morfnotify.exe
curl http://127.0.0.1:8789/targets

# Avec une vraie configuration :
./build-mingw/service/morfnotify.exe --config config/morfnotify.json
```

Voir [`config/morfnotify.example.json`](config/morfnotify.example.json).

## Installer en service

```sh
# Toutes plateformes : Linux, Windows, Raspberry Pi
sudo ./service.py install      # compile si besoin, installe, demarre
sudo ./service.py update       # recompile, remplace le binaire, redemarre
sudo ./service.py uninstall    # desinscrit, en conservant votre configuration
./service.py status            # ce que le systeme en dit
```

Un seul point d'entree partout. Ce qu'est ce service — son nom, son dossier,
ses configurations — est declare dans `service.json` a cote. Les quatre etapes
d'installation vivent une seule fois pour tout le parc ; seul le gestionnaire
de services change selon la plateforme.

Les anciens scripts `scripts/linux/` et `scripts/windows/` fonctionnent
toujours, inchanges.

## Documentation

- [Architecture](docs/fr/ARCHITECTURE.md) — les classes et le fil d'exécution.
- [Protocole HTTP](docs/fr/PROTOCOL.md) — routes et schéma JSON.
- [Intégration](docs/fr/INTEGRATION.md) — **ajouter une destination** ; brancher
  un producteur.

## Licence

GPL-3.0-only — © 2026 morfredus (Frédéric Biron).
