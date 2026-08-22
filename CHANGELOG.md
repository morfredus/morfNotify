# Journal des versions - morfNotify

Le format s'inspire de [Keep a Changelog](https://keepachangelog.com/fr/1.1.0/)
et du [versionnage sémantique](https://semver.org/lang/fr/).

## [0.4.2] - 2026-08-21

### Ajouté

- Enregistrement des compilations au niveau CMake (record_compile) : la durée de compile est signalée à morfAnalytics quel que soit le déclencheur (cmake --build direct, morf upgrade, déploiement morfDeploy).

## [0.4.1] - 2026-08-21

### Modifié

- Resynchroniser la copie vendorée de morfdeploy vers 0.17.3.

## [0.4.0] - 2026-08-20

### Ajouté

- Mise à jour de la copie vendorée de morfDeploy 0.14.0 pour le packaging avec
  provenance vérifiée.

## [0.3.4] - 2026-08-14

### Corrigé

- **Troncature des grandes réponses HTTP** dans `HttpServer::reply()`. Resynchronisation
  du correctif issu du patron `morfTemplateService` : la méthode fermait la connexion
  sans drainer le tampon d'écriture, ce qui coupait toute réponse dépassant la taille
  du tampon socket (~20 Ko). On attend désormais que `bytesToWrite()` retombe à zéro
  avant `disconnectFromHost()`.
- Resynchronisation de la copie vendorée de **morfBeacon** (`third_party/morf/beacon`)
  en 0.6.1 : même classe de bug corrigée dans son `StatusServer` (grande réponse
  `/status` coupée faute de drainage du tampon d'écriture).

## [0.3.3] - 2026-08-14

### Corrigé

- Description de l'unité systemd : remplacement du tiret cadratin par un tiret
  simple, conformément à la règle de ponctuation du parc.

## [0.3.2] - 2026-08-14

### Modifié

- Resynchronisation de la copie vendorée de **morfBeacon**
  (`third_party/morf/beacon`) en 0.6.0, alignée sur le dépôt source
  (`IMetricsProvider.h`, `StatusServer.cpp`). Aucun changement de comportement ;
  la copie statique reste en phase avec le parc.

## [0.3.1] - 2026-07-28

### Modifié

- **Copie vendorée de morfdeploy resynchronisée** depuis morfTools : prise en
  charge de l'état persistant `/var/lib` dans le déploiement (`manifest.state_dir()`,
  substitution `__STATE_DIR__`, chemin d'état affiché à l'installation). Aucun
  changement de comportement du service lui-même.

## [0.3.0] - 2026-07-28

### Modifié

- **Configuration regroupée sous `/etc/morfsystem/<service>`.** Tout le parc
  partage désormais un point d'entrée UNIQUE dans `/etc` (`/etc/morfsystem/`),
  qui contient le fichier partagé `morfsystem.json` et un sous-dossier par
  service, au lieu d'un `/etc/<service>` par service à la racine de `/etc`. Sous
  Windows : `%ProgramData%\morfsystem\<service>`. Les données restent sous
  `/opt/<service>`. L'ancien `/etc/<service>` est adopté à l'installation
  (`migrate_from`).


## [0.2.2] - 2026-07-26
### Ajouté

- **Déclaration de l'API dans `/status`.** Le service annonce désormais ses
  routes métier (`POST /notify`, `GET /targets`) via le point unique
  `fillAnnouncedDetail` + `morfbeacon::describeService` - la même source que
  morfAnalytics et morfMonitor. Étant sans interface web, il n'émet que le bloc
  `api`, jamais de `web_ui`. Un superviseur du parc peut ainsi cartographier son
  API sans la connaître à l'avance. Le heartbeat reste inchangé (il annonce des
  capacités, pas l'API).

## [0.2.1] - 2026-07-22
### Modifié

- **Installation, mise à jour et désinstallation par `./service.py`** - point
  d'entrée unique multiplateforme (morfdeploy), en remplacement des scripts
  `install-service.sh`/`.ps1`. Le binaire de ce service est inchangé ; seul son
  mode de déploiement évolue.
- **La configuration vit désormais dans `/etc/morfnotify`** (convention Linux),
  séparée du binaire dans `/opt/morfnotify`. Le déplacement est déclaré : la config
  existante est adoptée, jamais écrasée.
- **Enrichissement à la mise à jour** : une clé introduite par une nouvelle
  version est ajoutée avec sa valeur par défaut, sans jamais toucher vos réglages.

## [0.2.0] - 2026-07-21

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
  son cycle de vie propre - elle n'a ni `startAll()` ni `stopAll()`, une
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

## [0.1.1] - 2026-07-19

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

## [0.1.0] - 2026-07-16

### Ajouté

- **Service autonome de diffusion de notifications** : point unique de fan-out
  pour l'écosystème morfSystem. Ne détecte rien, ne décide de rien, ne connaît
  aucun projet - il reçoit, valide et distribue.
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
