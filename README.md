# morfNotify

*Read in another language: **English** (this document) · [Français](README.fr.md).*

[![Version](https://img.shields.io/badge/version-0.2.1-blue)](CHANGELOG.md)
![C++](https://img.shields.io/badge/C%2B%2B-17-00599C?logo=cplusplus)
![Qt](https://img.shields.io/badge/Qt-6-41CD52?logo=qt)
![Build](https://img.shields.io/badge/CMake-3.21+-064F8C?logo=cmake)
![License](https://img.shields.io/badge/License-GPL--3.0--only-blue)

**Autonomous notification dispatch service for the morfSystem ecosystem.**

morfNotify **detects nothing, decides nothing, knows no project**. Its only job:
receive a message and deliver it to the requested destinations. It is the single
fan-out point for the ecosystem's notifications — any project (present or future)
can inform the user with a minimal integration, without knowing how delivery works.

## Philosophy

- **Producers own all the logic**: *when*, *what* and *to whom* to send.
- morfNotify **receives → validates → distributes → optionally logs**. Nothing more.
- It does **not** monitor services, analyse data, decide to send, or contain any
  business rule. It doesn't know SiteWatch, MeteoHub, morfSensor, morfCollector…

## API

Deliberately tiny and stable. A producer POSTs a message:

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
| `POST /notify` | receive + validate + dispatch (`202 Accepted`; `400` if invalid) |
| `GET /targets` | list configured destinations |
| `GET /status` | morfBeacon-compatible (app, version, uptime, metrics) |
| `GET /healthz` | `{ "status": "ok" }` |

`202` response: `{ "accepted": true, "queued": [...], "unknown": [...] }` — unknown
target names are reported, never fatal. Delivery is **fire-and-forget**: producers
never wait on a slow destination.

## Destinations (pluggable)

Independent of each other, added without changing the API. Built in:

- **`log`** — journal (journald) + optional file.
- **`webhook`** — HTTP POST to a URL. `format: "json"` (full notification) or
  `format: "ntfy"` (ntfy.sh push — e.g. a phone/Pixel).

Adding a destination (e-mail, MQTT, a dedicated screen…) = one `INotifier`
subclass + one line in the factory. Producers keep using the exact same API.

## Build

Only needs **Qt 6** (Core, Network). **morfBeacon is bundled** (vendored under
`third_party/morf/beacon`), so the build depends on no external repository.

```sh
cmake --preset mingw        # or linux / linux-arm64
cmake --build --preset mingw
```

## Run

```sh
# Instant demo, no config (a single 'log' destination):
./build-mingw/service/morfnotify.exe
curl http://127.0.0.1:8789/targets

# With a real configuration:
./build-mingw/service/morfnotify.exe --config config/morfnotify.json
```

See [`config/morfnotify.example.json`](config/morfnotify.example.json).

## Install as a service

```sh
# Any platform: Linux, Windows, Raspberry Pi
sudo ./service.py install      # build if needed, install, start
sudo ./service.py update       # rebuild, replace the binary, restart
sudo ./service.py uninstall    # deregister, keeping your configuration
./service.py status            # what the system says about it
```

One entry point everywhere. What this service is — its name, its directory,
its configurations — is declared in `service.json` beside it. The four install
steps live once for the whole parc; only the service manager differs by
platform.

The former `scripts/linux/` and `scripts/windows/` scripts still work,
unchanged.

## Documentation

French documentation in [`docs/fr/`](docs/fr/README.md): architecture, HTTP
protocol, **adding a destination**, integrating a producer.

## License

GPL-3.0-only — © 2026 morfredus (Frédéric Biron).
