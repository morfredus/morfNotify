# Journal des versions — morfNotify

Le format s'inspire de [Keep a Changelog](https://keepachangelog.com/fr/1.1.0/)
et du [versionnage sémantique](https://semver.org/lang/fr/).

## [Non publié]

## [0.2.1] – 2026-07-22
### Modifié

- **Installation, mise à jour et désinstallation par `./service.py`** — point
  d'entrée unique multiplateforme (morfdeploy), en remplacement des scripts
  `install-service.sh`/`.ps1`. Le binaire de ce service est inchangé ; seul son
  mode de déploiement évolue.
- **La configuration vit désormais dans `/etc/morfnotify`** (convention Linux),
  séparée du binaire dans `/opt/morfnotify`. Le déplacement est déclaré : la config
  existante est adoptée, jamais écrasée.
- **Enrichissement à la mise à jour** : une clé introduite par une nouvelle
  version est ajoutée avec sa valeur par défaut, sans jamais toucher vos réglages.

## [0.2.0] – 2026-07-21

### Modifié

- **Briques d'infrastructure alignées sur les noms du gabarit.** morfNotify
  précède `morfTemplateService` : il a donné naissance au gabarit, avec
  morfSensor, et en avait gardé ses propres noms. Cette divergence empêchait
  toute comparaison mécanique avec l'amont et compliquait la maintenance du
  squelette.

  | Avant | Après |
  | --- | --- |
  | `NotifyHttpServer` | `HttpServer` |
  | `NotifyService` | `Service` |
  | `NotifyConfig` | `ServiceConfig` |
  | `NotifierRegistry` | `ModuleRegistry` |
  | `NotifierFactory` | `ModuleFactory` |
  | `TargetDef` | `ModuleDef` |

  Les fichiers correspondants sont renommés à l'identique.

- **Le métier n'est pas touché.** `INotifier`, `Notification`, `EmailNotifier`,
  `TelegramNotifier`, `WebhookNotifier` et `LogNotifier` gardent leurs noms :
  ce sont les points d'extension propres à morfNotify. `ModuleRegistry`
  conserve ses méthodes métier (`dispatch`, `targetNames`, `targetsJson`) et
  son cycle de vie propre — elle n'a ni `startAll()` ni `stopAll()`, une
  livraison n'étant pas un module que l'on démarre. C'est précisément le genre
  de divergence que l'architecture autorise.

- **Aucun changement fonctionnel ni de contrat.** Le format du fichier de
  configuration est inchangé (clés `targets`, `http_port`, `beacon`…), l'API
  HTTP est inchangée, et le binaire se comporte à l'identique. Le renommage
  porte sur des identifiants C++ et les noms de fichiers, rien d'autre.

  Cette harmonisation ne remet pas en cause le principe de l'écosystème :
  `morfTemplateService` reste un **gabarit de création**, pas un framework
  d'exécution. Chaque service demeure propriétaire de son implémentation et
  libre de la faire évoluer. Seuls les noms convergent, pas le code.

## [0.1.1] – 2026-07-19

### Modifié

- **Copie vendorée de morfBeacon resynchronisée en 0.2.0** (champ `capabilities`
  du heartbeat). Ajout purement additif et facultatif ; ce projet n'annonce
  aucune capacité et son comportement est strictement inchangé. La
  resynchronisation évite que la copie embarquée ne dérive de l'amont.


### Corrigé

- **La mise à jour ne livrait jamais les nouveaux paramètres de configuration.**
  `update-service.sh` ne recopiait que le binaire et laissait `morfnotify.json`
  intact, par souci de préserver les réglages locaux. Conséquence : un paramètre
  introduit après l'installation restait absent indéfiniment, et la fonction
  correspondante ne s'activait jamais **sans que rien ne le signale**. La mise à
  jour **complète** désormais la configuration (`scripts/linux/merge-config.py`) :
  les valeurs déjà en place ne sont jamais modifiées, les clés manquantes sont
  ajoutées puis listées, et une sauvegarde précède toute écriture. Option
  `--no-config` pour laisser la configuration strictement intacte.
- **La configuration absente n'était pas recréée.** Après une installation
  partielle ou une suppression du dossier, la mise à jour laissait le service
  démarrer sans configuration. Elle est désormais recopiée depuis l'exemple.
- **L'unité systemd n'était pas rafraîchie.** Une modification du fichier
  `.service` dans le dépôt ne parvenait jamais à `/etc/systemd/system` : le
  service continuait de tourner avec l'ancienne définition.

## [0.1.0] — 2026-07-16

### Ajouté

- **Service autonome de diffusion de notifications** : point unique de fan-out
  pour l'écosystème morfSystem. Ne détecte rien, ne décide de rien, ne connaît
  aucun projet — il reçoit, valide et distribue.
- **API HTTP** : `POST /notify` (title, message, level, targets → `202 Accepted`,
  `400` si invalide), `GET /targets`, `GET /status` (compatible morfBeacon),
  `GET /healthz`.
- **Point d'extension `INotifier`** (QObject asynchrone) + fabrique
  `ModuleFactory` : ajouter une destination = une classe + une ligne.
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
