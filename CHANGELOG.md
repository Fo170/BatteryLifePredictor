# Changelog - BatteryLifePredictor

## [1.3.1] - 2026-10-17

### ✨ Added — macro de version `__<LIB>_H_VERSION__`

Chaque header déclare désormais sa version via une macro préprocesseur,
source **unique** (un projet hôte peut l'agréger, ex. page `/version`).

### 🏷️ Divers

- Version `1.3.0` → `1.3.1` (`library.json` + `library.properties`)

---

## [1.3.0] - 2026-10-17

### ✨ Added — modèle d'usure décomposé + cycles réels (Rainflow)

Ajouts **additifs** : la décomposition de l'usure et le comptage de cycles par
**Rainflow streaming** sur le bilan Ah net.

| API | Rôle |
|---|---|
| `getWear()` | Usure totale (0..1) |
| `getCalendarWear()` | Part **calendaire** de l'usure |
| `getCycleWear()` | Part **cycles** de l'usure |
| `wearTotal()` / `wearCalendar()` / `wearCycle()` | Variantes calculables à une date/microcycles donnés |
| `rainflowPush(deltaAh)` | Alimente le détecteur Rainflow (streaming, O(1) mémoire) |
| `rainflowFeed(...)` | Alimentation groupée du Rainflow |
| `addCycleDamage(dod)` | Ajoute une usure de cycle pour une profondeur de décharge donnée |
| `eolPct()` | Usure exprimée en % de la durée de vie restante |
| `getCapacityEol()` | Capacité de fin de vie (Ah) |
| `getCapacityNew()` | Capacité d'une batterie neuve (Ah) |

### Pourquoi
L'usure totale était jusque-là un cumul global, sans distinction entre
vieillissement **calendaire** (plancher) et usure par **cycles réels**. La
décomposition permet de diagnostiquer la cause dominante (calendaire vs
cyclage) et le Rainflow évite de compter un cycle par micro-variation de courant.

## [1.2.1] - 2026-09-23

Version initiale publiée (modèle d'usure calendaire + cycles, EOL, usure initiale).
