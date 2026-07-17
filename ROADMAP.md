# Roadmap — morfNotify

Pistes envisagées, sans engagement de date. morfNotify doit rester **petit,
stable et sans logique métier** : chaque ajout se pèse à cette aune. L'API
(`POST /notify`) est un contrat : elle ne doit pas bouger quand on ajoute des
destinations.

## Nouvelles destinations

Le point d'extension `INotifier` + `NotifierFactory` est prêt pour :

- **E-mail** — envoi SMTP (`type: "smtp"`).
- **MQTT** — publication sur un broker domotique (`type: "mqtt"`).
- **Écrans dédiés** — cuisine, salon, atelier (webhook aujourd'hui ; driver
  dédié si besoin).
- **Slack / Discord / Telegram** — webhooks entrants (déjà possibles via `webhook`
  ; un format dédié pourrait simplifier).

Chaque ajout : une classe `INotifier`, une branche dans `NotifierFactory::create`,
une ligne dans `knownTypes()`. L'API et les producteurs ne changent pas.

## Service

- **Journalisation structurée** des envois (fichier tournant / JSON lines).
- **Réessais** configurables sur échec de livraison (webhook injoignable).
- **Files par destination** avec limite de débit (éviter d'inonder un canal).
- **Endpoint `/metrics`** au format Prometheus.
- **Hot-reload** de la configuration (SIGHUP) sans redémarrer.
- **Authentification optionnelle** de `POST /notify` (jeton) si le service est
  exposé hors du LAN.

## Filtrage (sans logique métier)

- Table de correspondance **niveau → destinations par défaut** dans la config
  (ex. `error` → `pixel` + `mail`), en gardant le principe : le producteur peut
  toujours surcharger `targets`. À n'introduire que si ça ne fait pas rentrer de
  logique métier dans le service.
