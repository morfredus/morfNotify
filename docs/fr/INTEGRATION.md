# Intégration — morfNotify

Retour à l'[index de la documentation](README.md).

---

Deux intégrations : **brancher un producteur** (envoyer une notification) et
**ajouter une destination** (nouveau canal de sortie).

## A. Brancher un producteur

N'importe quel projet informe l'utilisateur avec un simple `POST /notify`. Il
décide **quand**, **quoi** et **à qui** ; morfNotify distribue.

### En ligne de commande

```sh
curl -X POST http://127.0.0.1:8789/notify \
  -H 'Content-Type: application/json' \
  -d '{"title":"morfSensor","message":"Capteur déconnecté","level":"error","targets":["pixel"]}'
```

### En Python (stdlib, non bloquant si lancé à part)

```python
import json, urllib.request

def notify(title, message, level, targets, url="http://127.0.0.1:8789/notify"):
    body = json.dumps({"title": title, "message": message,
                       "level": level, "targets": targets}).encode()
    req = urllib.request.Request(url, data=body,
                                 headers={"Content-Type": "application/json"})
    try:
        urllib.request.urlopen(req, timeout=2)
    except Exception:
        pass   # une notification ratée ne doit pas casser le producteur
```

### En C++ / Qt

`QNetworkAccessManager::post()` vers `/notify` avec le JSON en corps. Le producteur
n'a **aucune** logique d'envoi à connaître : il ne sait pas ce qu'est « pixel » ni
comment on l'atteint.

> Bonne pratique : le producteur ne doit jamais **bloquer** ni **échouer** à cause
> d'une notification. En cas d'erreur (service absent, timeout), on ignore.

## B. Ajouter une destination

Le point d'extension est `INotifier`. Exemple : une destination fictive `demo`.

### 1. La classe (`include/morfnotify/DemoNotifier.h`)

```cpp
#pragma once
#include "morfnotify/INotifier.h"

namespace morfnotify {
class DemoNotifier : public INotifier {
    Q_OBJECT
public:
    explicit DemoNotifier(const QString& name, QObject* parent = nullptr)
        : INotifier(name, QStringLiteral("demo"), parent) {}
    void send(const Notification& n) override;
};
} // namespace morfnotify
```

### 2. L'implémentation (`src/DemoNotifier.cpp`)

```cpp
#include "morfnotify/DemoNotifier.h"
#include <QDebug>

namespace morfnotify {
void DemoNotifier::send(const Notification& n) {
    // ... remettre le message par le moyen voulu (asynchrone de préférence) ...
    qInfo() << "demo:" << n.title << n.message;
    emit delivered(name());              // ou emit failed(name(), "raison");
}
} // namespace morfnotify
```

### 3. L'enregistrer (`src/ModuleFactory.cpp`)

```cpp
#include "morfnotify/DemoNotifier.h"
// ...
if (type == QLatin1String("demo"))
    return new DemoNotifier(def.name, parent);
```

Et ajouter `"demo"` à `knownTypes()`.

### 4. Le déclarer dans CMake

Ajouter `src/DemoNotifier.cpp` et son en-tête aux sources de la cible `morfNotify`.
Si le canal a besoin d'une dépendance (SMTP, MQTT…), la mettre derrière une option
CMake.

### 5. L'activer dans la config

```json
{ "name": "console", "type": "demo" }
```

Rien d'autre à toucher : les producteurs peuvent aussitôt viser `"console"` dans
leurs `targets`. **L'API n'a pas changé** — c'est tout l'intérêt du point de
diffusion unique.

## Destination e-mail SMTP

La destination fournie `email` se configure comme une destination normale :

```json
{
  "name": "email",
  "type": "email",
  "smtp_host": "smtp.example.com",
  "smtp_port": 587,
  "security": "starttls",
  "username": "user@example.com",
  "password": "app-password",
  "from": "morfSystem <morf@example.com>",
  "to": ["toi@example.com"],
  "subject_prefix": "morfSystem"
}
```

Les producteurs, dont morfDashboard, n'ont pas à connaître SMTP : ils
continuent simplement à viser `"email"` dans `targets`.

## Destination Telegram

La destination fournie `telegram` utilise l'API Telegram Bot `sendMessage` :

```json
{
  "name": "telegram",
  "type": "telegram",
  "bot_token": "123456789:AA...remplacer...",
  "chat_id": "123456789"
}
```

Un producteur choisit le canal au moment de l'appel :

```json
{ "title": "morfDashboard", "message": "morfSensor defaillant", "level": "error", "targets": ["telegram"] }
```

Pour envoyer sur plusieurs canaux, viser plusieurs destinations :
`"targets": ["email", "telegram"]`.
