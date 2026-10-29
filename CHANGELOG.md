# Changelog - BatteryLifePredictor

## [1.5.0] - 2026-10-24

### ✨ Added — recalage de l'usure initiale sur une mesure

| API | Rôle |
|---|---|
| `setWearInit(float w)` | Fixe l'usure initiale à partir d'une capacité **mesurée** |
| `getKalmanEOLReason()` | Motif de non-calcul de l'EOL |
| `kalmanEOLReasonStr()` | Le même motif, en texte |

### 🐛 Corrigé — le SOH affiché était en grande partie une constante

Jusqu'à **96,5 % du SOH affiché** provenait de la constante
`BLP_INITIAL_WEAR_PCT` et non d'une mesure : la prédiction ne reflétait donc pas
l'état réel de la batterie.

`setWearInit()` recale l'usure initiale sur une capacité mesurée **sans double
comptage** du calendaire et des cycles — c'est le piège de ce recalage : ajouter
l'usure mesurée à celle déjà accumulée la compte deux fois.

### 🐛 Corrigé — « -1 jours »

Un EOL **non calculable** s'affichait `-1 jours`, valeur indiscernable d'un EOL
dépassé. `getKalmanEOLReason()` distingue désormais les deux cas.

---

## [1.4.0] - 2026-10-24

### ✨ Added — la capacité de référence devient modifiable

`_capacityNew` était fixé **une seule fois**, dans `begin()`, et jamais
modifiable ensuite : après apprentissage de la capacité réelle par le projet,
la prédiction continuait de raisonner sur la capacité nominale d'origine.

| API | Rôle |
|---|---|
| `setCapacityNew(float ah)` | Recalage de la capacité de référence après apprentissage |
| `setWearCycle(float w)` | Correction d'usure par une mesure |
| `getCapacityCurrent()` | Capacité « vue usure », désormais publique |

### 🛡️ Robustesse

`setCapacityNew()` est **borné** à `[1 ; 100 000]` Ah et **refuse** une valeur
non finie : la capacité sert de dénominateur en aval, une valeur nulle ou `NaN`
produisait une division par zéro.

### 🔗 Dépendances rendues explicites

Le header se contentait de **déclarer** `class BatteryModel;` et
`class BatteryKalman;` devant `begin()`, en comptant sur l'appelant pour les
inclure **avant**. Un `#include <BatteryLifePredictor.h>` seul échouait donc :

```
BatteryLifePredictor.h:255:31: error: invalid use of incomplete type 'class BatteryModel'
```

L'ordre d'inclusion de `main.cpp` masquait le défaut. Les deux `#include` sont
désormais explicites dans le header, et `library.json` déclare les dépendances
(`>=2.0.0`) pour que le LDF les installe.

**Aucun changement de formule.** Miner, Arrhenius, van't Hoff et Rainflow sont
intacts ; `BatteryLifeState` garde son layout et son magic `0x424C5032`, donc la
persistance LittleFS `/life.bin` reste **compatible**.

---

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
