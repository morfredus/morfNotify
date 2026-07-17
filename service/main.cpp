/*
 * morfNotify — demon de service
 * Copyright (C) 2026 morfredus
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Charge une configuration JSON, ouvre l'API HTTP de reception (POST /notify) et
 * annonce sa presence sur le LAN (morfBeacon). Point unique de diffusion des
 * notifications de l'ecosysteme morfSystem.
 */

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QTextStream>

#include <morfnotify/NotifyService.h>
#include <morfnotify/NotifierFactory.h>
#include <morfnotify/Version.h>

using morfnotify::NotifyConfig;

namespace {

QTextStream& out() { static QTextStream s(stdout); return s; }
QTextStream& err() { static QTextStream s(stderr); return s; }

QString findDefaultConfig() {
    const QString exeDir = QCoreApplication::applicationDirPath();
    const QStringList candidates = {
        QDir::current().filePath("morfnotify.json"),
        QDir(exeDir).filePath("morfnotify.json"),
        QDir(exeDir).filePath("config/morfnotify.json"),
#ifdef Q_OS_UNIX
        QStringLiteral("/etc/morfnotify/morfnotify.json"),
#endif
    };
    for (const QString& c : candidates)
        if (QFileInfo::exists(c))
            return c;
    return {};
}

// Config de repli : une destination 'log' (journal), prete a tester l'API.
NotifyConfig fallbackConfig() {
    NotifyConfig c;
    morfnotify::TargetDef log;
    log.name = QStringLiteral("journal");
    log.type = QStringLiteral("log");
    c.targets.push_back(log);
    return c;
}

bool loadConfig(const QString& path, NotifyConfig* outCfg, QString* error) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        *error = QStringLiteral("impossible d'ouvrir %1 : %2").arg(path, f.errorString());
        return false;
    }
    QJsonParseError pe{};
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &pe);
    if (pe.error != QJsonParseError::NoError || !doc.isObject()) {
        *error = QStringLiteral("JSON invalide dans %1 : %2").arg(path, pe.errorString());
        return false;
    }
    *outCfg = NotifyConfig::fromJson(doc.object());
    return true;
}

} // namespace

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("morfNotify"));
    QCoreApplication::setApplicationVersion(morfnotify::version());

    QCommandLineParser parser;
    parser.setApplicationDescription(
        QStringLiteral("morfNotify — point unique de diffusion des notifications "
                       "(API POST /notify, destinations enfichables)."));
    parser.addHelpOption();
    parser.addVersionOption();
    QCommandLineOption configOpt({"c", "config"},
        QStringLiteral("Fichier de configuration JSON."), QStringLiteral("chemin"));
    QCommandLineOption listOpt("list-types",
        QStringLiteral("Liste les types de destinations disponibles puis quitte."));
    parser.addOption(configOpt);
    parser.addOption(listOpt);
    parser.process(app);

    if (parser.isSet(listOpt)) {
        out() << "Types de destinations disponibles : "
              << morfnotify::NotifierFactory::knownTypes().join(", ") << '\n';
        return 0;
    }

    NotifyConfig config;
    QString configPath = parser.value(configOpt);
    if (configPath.isEmpty())
        configPath = findDefaultConfig();

    if (configPath.isEmpty()) {
        err() << "Aucune configuration trouvee : demarrage avec une destination "
                 "'log' (journal). Fournir --config pour de vraies destinations.\n";
        config = fallbackConfig();
    } else {
        QString error;
        if (!loadConfig(configPath, &config, &error)) {
            err() << "Erreur de configuration : " << error << '\n';
            return 2;
        }
        out() << "Configuration chargee : " << configPath << '\n';
    }

    morfnotify::NotifyService service(config);
    for (const QString& w : service.warnings())
        err() << "Avertissement : " << w << '\n';

    if (!service.start()) {
        err() << "Le serveur HTTP n'a pas pu ecouter sur le port "
              << config.httpPort << " (deja utilise ?).\n";
        return 3;
    }

    out() << "morfNotify v" << morfnotify::version() << " demarre : "
          << service.targetCount() << " destination(s), API http://"
          << config.bindAddress << ':' << service.httpPort()
          << "/  (POST /notify ; GET /targets /status /healthz)\n";
    out().flush();

    return app.exec();
}
