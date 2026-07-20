# Architecture — morfNotify

Retour à l'[index de la documentation](README.md).

---

morfNotify est un **service Qt (Core + Network)**, sans interface graphique. Le
cœur ne connaît **aucune** destination en particulier : toute la connaissance
d'un canal de sortie est isolée derrière l'interface `INotifier`.

## Les pièces

```
Service (façade : câble tout à partir d'une ServiceConfig)
├── ModuleRegistry   -> collectionne les INotifier, DISTRIBUE, compte
│     └── INotifier (interface, QObject)   ◀── POINT D'EXTENSION
│            ├── LogNotifier      (journald + fichier)
│            └── WebhookNotifier  (POST HTTP ; format json ou ntfy)
├── HttpServer   -> API HTTP (POST /notify, /targets, /status, /healthz)
└── morfbeacon::Heartbeat -> annonce UDP "morfNotify" (découverte LAN)
        ▲ IMetricsProvider
        └── ModuleRegistry expose ses compteurs (reçu / distribué / échecs)
```

### `Notification` (struct)

Le **seul** objet « métier », volontairement pauvre : `title`, `message`, `level`
(`info`/`success`/`warning`/`error`, libre), `targets` (noms de destinations),
`ts`. `fromJson()` valide la forme (message non vide, au moins une cible) ;
morfNotify **ne l'interprète pas**, il la relaie.

### `INotifier` (interface, QObject)

Le **seul** point d'extension. Chaque destination en hérite et implémente
`send()` de façon **asynchrone** (fire-and-forget), en émettant `delivered` ou
`failed`. `name()` est ce que les producteurs mettent dans `targets` ; `type()`
est l'identifiant de fabrique.

### `ModuleFactory`

Fabrique un `INotifier` à partir d'un `ModuleDef` (config). Point d'extension
**compile-time** : une branche par type reconnu ; `knownTypes()` les liste.

### `ModuleRegistry` (QObject + `morfbeacon::IMetricsProvider`)

Détient les destinations, indexées par nom (uniques). `dispatch(notification)`
remet le message vers chaque cible demandée qui existe, et **rapporte les noms
inconnus** sans échouer. Tient des compteurs (reçu / distribué / échecs) exposés
via `/status`.

### `HttpServer` (QObject)

Serveur HTTP/1.1 minimal sur `QTcpServer. Contrairement à un serveur en lecture
seule, il lit un **corps de requête** (POST) : il attend `Content-Length` octets
avant de traiter. Route vers `POST /notify` (valide + distribue → `202` /`400`),
`GET /targets`, `GET /status`, `GET /healthz`.

### `Service` (façade)

L'unique objet manipulé par le démon. À partir d'une `ServiceConfig`, il construit
les destinations (via `ModuleFactory`), les enregistre, démarre le serveur HTTP
puis le heartbeat morfBeacon.

## Fil d'exécution

Tout tourne sur **le thread principal Qt** (boucle d'événements). `POST /notify`
déclenche `dispatch()` qui appelle `send()` sur chaque destination et **retourne
aussitôt** `202 Accepted` : la livraison (réseau, fichier) se fait en arrière-plan
via les signaux/slots Qt, sans faire attendre le producteur. Une destination lente
ou injoignable n'impacte donc jamais l'appelant ; son échec est journalisé
(`failed`).

## Dépendance morfBeacon (embarquée)

morfNotify réutilise `morfbeacon::Heartbeat` pour s'annoncer sur le LAN.
morfBeacon est **vendoré** dans `third_party/morf/beacon` (lié statiquement,
comme morfSensor / ComponentHub) : build autonome, sans dépôt externe.
Resynchroniser avec `scripts/sync-morf.(sh|ps1)`.

## Portabilité

Aucun code spécifique à une plateforme (`QTcpServer`, `QNetworkAccessManager`,
JSON Qt). Comportement identique sous Windows, Linux x64 et Raspberry Pi (ARM64).
