# Documentation de morfNotify (français)

Service autonome de diffusion de notifications (point unique de fan-out) pour
l'écosystème morfSystem. API HTTP `POST /notify`, destinations enfichables.

> 🇬🇧 English documentation: [`docs/en/`](../en/README.md) *(index, in progress)*.
> Retour au [README (français)](../../README.fr.md).

## Comprendre et intégrer

| Document | Contenu |
|---|---|
| [Architecture](ARCHITECTURE.md) | Les classes (`Notification`, `INotifier`, `NotifierRegistry`, `NotifyHttpServer`, `NotifyService`) et le fil d'exécution. |
| [Protocole HTTP](PROTOCOL.md) | Les routes (`POST /notify`, `/targets`, `/status`, `/healthz`) et le schéma JSON. |
| [Intégration](INTEGRATION.md) | **Ajouter une destination** ; brancher un producteur. |

## À la racine du projet

| Document | Contenu |
|---|---|
| [README](../../README.md) | Présentation générale (anglais). |
| [README (français)](../../README.fr.md) | Présentation générale (français). |
| [Journal des versions](../../CHANGELOG.md) | Historique des versions. |
| [Roadmap](../../ROADMAP.md) | Évolutions envisagées. |
| [Contribuer](../../CONTRIBUTING.md) | Guide de contribution. |
