# Contribuer à morfNotify

Merci de votre intérêt ! morfNotify est un **point de diffusion de notifications**
volontairement minimal : il doit rester **stable, sobre, et sans logique métier**.

## 1. Philosophie

- **Aucune décision.** morfNotify ne décide jamais qu'une notification doit
  partir : c'est le producteur qui envoie. Ne pas introduire de règle métier
  (« si tel service tombe, alors… ») dans le service.
- **API stable.** `POST /notify` est un contrat pour tout l'écosystème. On peut
  ajouter des destinations à volonté sans jamais changer l'API : c'est l'objectif.
- **Cœur découplé des destinations.** Le serveur HTTP, le registre et le service
  ne connaissent **aucune** destination en particulier. Tout vit dans une classe
  `INotifier`.
- **Livraison non bloquante.** Une destination pousse en arrière-plan
  (`delivered`/`failed`) ; jamais d'attente synchrone dans `POST /notify`.
- **Qt Core + Network uniquement.** Portable Windows / Linux x64 / Raspberry Pi.

## 2. Ajouter une destination

Voir [`docs/fr/INTEGRATION.md`](docs/fr/INTEGRATION.md). En résumé :

1. Créer `include/morfnotify/MaCible.h` + `src/MaCible.cpp`, héritant de
   `INotifier` (implémenter `send()`, émettre `delivered`/`failed`).
2. Ajouter une branche dans `NotifierFactory::create` et son nom dans
   `knownTypes()`.
3. L'ajouter aux sources dans `CMakeLists.txt`.

Aucune modification du registre, du serveur HTTP ni des producteurs.

## 3. Compiler et tester

```sh
cmake --preset mingw      # ou linux / linux-arm64
cmake --build --preset mingw
```

Exercer le chemin réel :

```sh
./build-mingw/service/morfnotify.exe        # destination 'log' de repli
curl -X POST http://127.0.0.1:8789/notify -H 'Content-Type: application/json' \
     -d '{"title":"Test","message":"Bonjour","level":"info","targets":["journal"]}'
```

## 4. Style

- C++17, conventions des projets frères (morfSensor, morfBeacon) : en-tête de
  licence SPDX, namespace `morfnotify`, commentaires en français expliquant le
  **pourquoi**.
- Fins de ligne : voir `.gitattributes` (LF dans le dépôt).
