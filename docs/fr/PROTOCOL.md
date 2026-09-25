# Protocole HTTP - morfNotify

Retour à l'[index de la documentation](README.md).

---

API HTTP/1.1. Réponses en JSON UTF-8, en-tête `Access-Control-Allow-Origin: *`,
connexion fermée après chaque réponse. Port par défaut : **8789** (`http_port`).
Version du protocole : `morfnotify/1` (champ `proto` de `/status`).

## `POST /notify`

Reçoit une notification, la valide, la distribue. C'est **le** point d'entrée.

Corps (JSON) :

```json
{
  "title":   "morfSensor",
  "message": "Service arrêté",
  "level":   "warning",
  "targets": ["pixel", "dashboard"]
}
```

| Champ | Obligatoire | Sens |
|---|---|---|
| `message` | **oui** (non vide) | corps du message |
| `targets` | non (défaut : voir ci-dessous) | noms des destinations (tableau, ou une seule chaîne) |
| `title` | non | titre court |
| `level` | non (défaut `info`) | `info` \| `success` \| `warning` \| `error` (libre) |

**Destinations par défaut.** Sans `targets` (ou avec un tableau vide), morfNotify
route lui-même, dans l'ordre : `default_targets` de sa config, puis la liste
partagée du parc `/etc/morfsystem/alert-targets` (virgules ou lignes, relue à
chaque appel), puis `telegram`. La réponse porte alors `"defaulted": true`. Un
producteur qui ne peut pas lire la config du parc (un ESP32 comme MeteoHub) laisse
ainsi morfNotify choisir. Ajout rétrocompatible : `morfnotify/1` inchangé.

Réponses :

- **`202 Accepted`** - notification acceptée et **remise en arrière-plan** :
  ```json
  { "accepted": true, "queued": ["pixel","dashboard"], "unknown": [], "ts": 1784229145 }
  ```
  `queued` = destinations retenues et déclenchées ; `unknown` = noms demandés mais
  non configurés (signalés, **pas** une erreur). La livraison réelle est
  fire-and-forget : un `202` ne garantit pas la remise finale (voir les logs).
- **`400 Bad Request`** - JSON invalide, ou `message` manquant :
  `{ "error": "champ 'message' requis et non vide" }`.
- **`405 Method Not Allowed`** - autre méthode que `POST` sur `/notify`.

## `GET /targets`

Liste des destinations configurées.

```json
{ "count": 2, "targets": [ { "name": "pixel", "type": "webhook" },
                           { "name": "journal", "type": "log" } ], "ts": 1784229145 }
```

## `GET /status`

Compatible **morfBeacon** (lu tel quel par `beacon_status.py` du dashboard).

```json
{
  "app": "morfNotify", "host": "pi4fred", "version": "0.1.0",
  "proto": "morfnotify/1", "state": "ok", "uptime_s": 3600, "ts": 1784229145,
  "metrics": { "targets": 2, "received_total": 12,
               "dispatched_total": 20, "failures_total": 1 }
}
```

## `GET /healthz`

Sonde de vie : `{ "status": "ok" }`.

## Annonce réseau (morfBeacon)

En parallèle de l'API, morfNotify diffuse un heartbeat UDP `morfbeacon/1` sur le
port du parc (45454 par défaut), avec `app: "morfNotify"`, `status_port` égal au
port HTTP réel et la capacité **`notification`**. Un superviseur le découvre ainsi
sans configuration, et un producteur le trouve par cette capacité, jamais par son
nom (MeteoHub s'en sert pour l'alerte « accu de la sonde à changer »).
