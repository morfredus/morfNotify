/*
 * morfNotify — exemple de demonstration
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Demarre le service avec une destination 'log' (journal), puis expose l'API.
 * A tester avec :
 *
 *   curl -X POST http://localhost:8789/notify \
 *        -H 'Content-Type: application/json' \
 *        -d '{"title":"Demo","message":"Bonjour","level":"info","targets":["journal"]}'
 *   curl http://localhost:8789/targets
 *   curl http://localhost:8789/status
 */

#include <QCoreApplication>

#include <morfnotify/NotifyService.h>
#include <morfnotify/NotifyConfig.h>
#include <morfnotify/Version.h>

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);

    morfnotify::NotifyConfig cfg;
    cfg.httpPort         = 8789;
    cfg.beaconIntervalMs = 5000;

    morfnotify::TargetDef log;
    log.name   = QStringLiteral("journal");
    log.type   = QStringLiteral("log");
    log.params = QJsonObject{ {"type", "log"}, {"name", "journal"} };
    cfg.targets.push_back(log);

    morfnotify::NotifyService service(cfg);
    if (!service.start()) {
        qWarning("API HTTP non demarree (port %u occupe ?)", cfg.httpPort);
        return 1;
    }

    qInfo("morfNotify demo v%s : %d destination(s) ; "
          "POST http://localhost:%u/notify",
          qUtf8Printable(morfnotify::version()), service.targetCount(), service.httpPort());

    return app.exec();
}
