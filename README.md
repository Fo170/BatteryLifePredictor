# BatteryLifePredictor

> **Prédiction de vie résiduelle des batteries** — Header-only C++ pour Arduino / ESP32 / ARM  
> Version **1.2.1** | Licence **GPL-3.0** | Auteur **FOURNET Olivier**

---

## Table des matières

- [Vue d'ensemble](#vue-densemble)
- [Fonctionnalités](#fonctionnalités)
- [Architecture](#architecture)
- [Installation](#installation)
- [Démarrage rapide](#démarrage-rapide)
- [API complète](#api-complète)
  - [Initialisation](#initialisation)
  - [Boucle principale](#boucle-principale)
  - [Prédiction de vie](#prédiction-de-vie)
  - [Comptage EFC](#comptage-efc)
  - [Règle de Miner](#règle-de-miner)
  - [Sulfatation (plomb)](#sulfatation-plomb)
  - [Rainflow](#rainflow)
  - [Régression linéaire](#régression-linéaire)
  - [Kalman 1D](#kalman-1d)
  - [Mode simulation](#mode-simulation)
  - [Persistance](#persistance)
- [Technologies supportées](#technologies-supportées)
- [Configuration compile-time](#configuration-compile-time)
- [Exemples](#exemples)
- [Documentation IEC 61427](#documentation-iec-61427)
- [Intégration avec BatteryModels & BatteryKalman](#intégration-avec-batterymodels--batterykalman)
- [Benchmark mémoire](#benchmark-mémoire)
- [Changelog](#changelog)
- [Licence](#licence)

---

## Vue d'ensemble

`BatteryLifePredictor` est une librairie **header-only** qui calcule la **vie résiduelle** d'une batterie en combinant plusieurs modèles mathématiques :

| Modèle | Usage |
|--------|-------|
| **DOD exponentiel** | Cycles nominaux selon la profondeur de décharge |
| **Arrhenius / Van't Hoff** | Correction thermique |
| **Sulfatation** | Pénalité PSOC spécifique au plomb |
| **EFC** | Comptage de cycles équivalents pleins |
| **Miner** | Cumul des dommages fractionnaires |
| **Rainflow** | Décomposition du profil SOC en cycles élémentaires |
| **Régression linéaire** | Prédiction EOL depuis la dérive de capacité |
| **Kalman 1D** | Lissage robuste de la dérive de capacité |
| **Simulation** | Validation sans matériel physique |

**Compatible** : Arduino AVR (Uno, Nano, Mega), ESP32, STM32, Teensy, Raspberry Pi Pico.

---

## Fonctionnalités

- ✅ **20 technologies** de batteries (plomb, lithium, NiMH, NaS, LTO, Redox Flow...)
- ✅ **Intégration native** avec [BatteryModels](https://github.com/Fo170/BatteryModels) et [BatteryKalman](https://github.com/Fo170/BatteryKalman)
- ✅ **Modèle de sulfatation** pour le plomb (temps PSOC + égalisation)
- ✅ **Rainflow simplifié** : détection de cycles depuis le profil SOC
- ✅ **Régression linéaire** : prédiction EOL depuis la dérive de capacité
- ✅ **Kalman 1D** : lissage optimal de la pente de dégradation
- ✅ **Mode simulation rapide** : profils IEC 61427 et personnalisés
- ✅ **Persistance EEPROM/FRAM** : état complet sauvegardable (~300 octets)
- ✅ **Header-only** : zéro dépendance externe
- ✅ **PROGMEM** : table de paramètres en flash sur AVR

---

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    VOTRE SKETCH ARDUINO                       │
├─────────────────────────────────────────────────────────────┤
│  BatteryModel        →  OCV, Thévenin, thermique, Peukert   │
│  BatteryKalman       →  EKF 2D (SoC, capacité, vieillissement)│
│  Coulomb             →  Compteur coulombs                   │
│  BatteryLifePredictor→  Vie résiduelle, EFC, Miner, Rainflow │
└─────────────────────────────────────────────────────────────┘
         ↑                              ↑
    Capteurs (V,I,T)              Prédictions (SOH, EOL, EFC)
```

**Flux de données** :
1. `BatteryKalman` fusionne OCV + Coulomb + thermique → **SoC précis**
2. `BatteryLifePredictor` détecte les cycles depuis les variations de SoC
3. Accumule EFC, Miner, sulfatation, historique SOC
4. Régression + Kalman 1D → **prédiction robuste de la date de fin de vie**

---

## Installation

### Méthode 1 — Arduino IDE (manuelle)

1. Téléchargez `BatteryLifePredictor.h`
2. Placez-le dans le dossier `libraries/BatteryLifePredictor/` de votre sketchbook
3. Redémarrez l'IDE Arduino

### Méthode 2 — PlatformIO

Ajoutez dans `platformio.ini` :

```ini
lib_deps =
    https://github.com/Fo170/BatteryModels.git
    https://github.com/Fo170/BatteryKalman.git
```

Puis copiez `BatteryLifePredictor.h` dans `lib/BatteryLifePredictor/`.

### Dépendances

| Librairie | Obligatoire | Rôle |
|-----------|-------------|------|
| `BatteryModels.h` | **Oui** | Modèle physique, OCV, détection état de charge |
| `BatteryKalman.h` | **Oui à la compilation** (optionnel à l'exécution) | EKF 2D, estimation capacité. `update()` appelle ses méthodes : le header ne compile pas sans lui, même avec `nullptr` |
| `Coulomb.h` | Oui (via `BatteryKalman.h`) | Compteur coulombs |

---

## Démarrage rapide

```cpp
#include <BatteryModels.h>
#include <BatteryKalman.h>
#include <Coulomb.h>
#include <BatteryLifePredictor.h>

BatteryModel    model(TECH_LIFEPO4, 4, 100.0f, 5.0f);
Coulomb         coulomb;
SoCData         socData;
KalmanState2D   kalmanState;
BatteryKalman   kalman(&socData, &kalmanState, &model, &coulomb);
BatteryLifePredictor life;

void setup() {
    Serial.begin(115200);
    life.begin(&model, &kalman, 100.0f);  // 100 Ah nominaux
}

void loop() {
    float voltage = analogRead(A0) * 0.0156f;
    float current = (analogRead(A1) * (5.0f/1024.0f) - 2.5f) / 0.066f;
    float temperature = (analogRead(A2) * (5.0f/1024.0f) * 1000.0f - 500.0f) / 10.0f;

    coulomb.addMeasurement(current);
    kalman.update(voltage, current, temperature);

    // Mise à jour prédicteur (dt en heures)
    life.update(voltage, current, temperature, 0.1f / 3600.0f);

    // Affichage toutes les 10 secondes
    static unsigned long lastPrint = 0;
    if (millis() - lastPrint >= 10000) {
        lastPrint = millis();
        Serial.print("SOH: "); Serial.print(life.getSOH(), 1);
        Serial.print("% | Vie: "); Serial.print(life.getRemainingLifeYears(), 1);
        Serial.println(" ans");
    }
}
```

---

## API complète

### Initialisation

```cpp
void begin(BatteryModel* model, BatteryKalman* kalman = nullptr, float capacityInitial = 0.0f);
```

| Paramètre | Description |
|-----------|-------------|
| `model` | Pointeur vers l'objet `BatteryModel` (obligatoire) |
| `kalman` | Pointeur vers `BatteryKalman` (optionnel, `nullptr` si absent) |
| `capacityInitial` | Capacité nominale initiale en Ah. `0` = auto depuis `model` |

```cpp
void reset();           // Réinitialise l'état interne (garde capacity_initial)
```

### Boucle principale

```cpp
void update(float voltage, float current, float temperature, float dt_hours);
```

À appeler à chaque itération (typiquement 1–10 Hz). `dt_hours` est le pas de temps depuis le dernier appel.

```cpp
void triggerEqualization();   // Réduit la sulfatation de 70% (plomb)
void triggerFullCharge();     // Réinitialise le compteur PSOC
```

### Prédiction de vie

```cpp
float getRemainingLifeYears(float dodAssumed = 0.0f);
```
Retourne la durée de vie estimée en **années**. Si `dodAssumed = 0`, utilise le DOD moyen historique.

```cpp
float getRemainingCycles(float dod, float temperature);
```
Cycles restants à un DOD et température donnés.

```cpp
float getSOH();                        // State of Health (%)
float getRemainingCapacityPercent();   // Alias de getSOH()
```

### Comptage EFC

```cpp
float getEFC();                        // Cycles équivalents pleins cumulés
uint32_t getTotalCycles();             // Cycles complets (DOD ≥ 80%)
uint32_t getTotalMicrocycles();        // Tous les cycles partiels détectés
```

### Règle de Miner

```cpp
float getMinerDamage();                // Dommage cumulé (1.0 = fin de vie)
```

### Sulfatation (plomb)

```cpp
float getSulfatationLevel();           // Niveau de sulfatation 0–1
float getSulfatationFactor();          // Facteur multiplicateur de dégradation
uint32_t getHoursInPSOC();             // Heures cumulées sous 80% SOC
uint32_t getHoursSinceEqualization();  // Heures depuis dernière égalisation
```

### Rainflow

```cpp
void runRainflow();                    // Force l'analyse des cycles
uint8_t getRainflowCount();            // Nombre de cycles détectés
bool getRainflowCycle(uint8_t index, RainflowCycle& out);
void getRainflowHistogram(uint16_t* bins, uint8_t nBins);
```

**Structure `RainflowCycle`** :
```cpp
struct RainflowCycle {
    float amplitude;    // Amplitude DOD (%)
    float mean;         // SOC moyen (%)
    float timestamp;    // Heure de détection (h depuis boot)
};
```

### Régression linéaire

```cpp
float getCapacitySlopePercentPerDay();  // Pente brute de dégradation (%/jour)
float getCapacityRegressionR2();        // Coefficient de détermination
float getDaysToEOL();                   // Jours avant 80% capacité (brut)
float getEOLDay();                      // Jour absolu de fin de vie
uint8_t getRegressionPointCount();      // Nombre de points collectés
void addRegressionPoint(float days, float capacityPct);  // Injection manuelle
void clearRegression();                 // Reset des points
```

### Kalman 1D

```cpp
float getKalmanSlope();                 // Pente lissée (%/jour)
float getKalmanOffset();                // Offset lissé (%)
float getKalmanSlopeUncertainty();      // Écart-type σ(a) (%/jour)
float getKalmanDaysToEOL();             // Jours EOL (robuste)
float getKalmanPredictedCapacity(float daysFromNow);  // Capacité future prédite
void resetKalman1D();                   // Réinitialise le filtre
```

### Mode simulation

```cpp
// Profil personnalisé
uint16_t simulateProfile(const float* dodArray, const float* tempArray,
                          uint16_t n, float dt_hours);

// Protocole IEC 61427
uint16_t simulateIEC61427(uint16_t nMacroCycles, float temperature);

// Cycle unitaire
void simulateOneCycle(float socStart, float socEnd, float temperature, float dt_hours);
```

**Exemple simulation** :
```cpp
float dods[365];
for (int i = 0; i < 365; i++) dods[i] = 50.0f + random(-10, 10);

life.simulateProfile(dods, nullptr, 365, 24.0f);
Serial.print("Vie estimée: ");
Serial.print(life.getRemainingLifeYears());
Serial.println(" ans");
```

### Persistance

```cpp
const BatteryLifeState& getState();     // Récupère l'état complet
void setState(const BatteryLifeState& state);  // Restaure un état
static size_t getStateSize();           // Taille en octets (~300)
```

**Sauvegarde EEPROM** :
```cpp
#include <EEPROM.h>

// Sauvegarde
BatteryLifeState s = life.getState();
EEPROM.put(0, s);

// Restauration
BatteryLifeState s;
EEPROM.get(0, s);
life.setState(s);
```

---

## Technologies supportées

| ID | Constante | Type | $N_0$ | $k_{DOD}$ | Sulfatation | $
|----|-----------|------|-------|-----------|-------------|
| 0 | `TECH_UNKNOWN` | — | 4000 | 0.020 | ❌ |
| 1 | `TECH_FLOODED` | Plomb inondé | 5500 | 0.022 | ✅ |
| 2 | `TECH_AGM` | VRLA AGM | 5000 | 0.023 | ✅ |
| 3 | `TECH_GEL` | VRLA Gel | 5200 | 0.022 | ✅ |
| 4 | `TECH_LIFEPO4` | **LiFePO₄** | **8000** | **0.012** | ❌ |
| 5 | `TECH_LION` | Lithium-ion | 5000 | 0.018 | ❌ |
| 6 | `TECH_LIPO` | Li-Po | 4500 | 0.020 | ❌ |
| 7 | `TECH_LTO` | Titanate | 20000 | 0.008 | ❌ |
| 8 | `TECH_LMNO` | Li-Mn-Ni | 4000 | 0.015 | ❌ |
| 9 | `TECH_NMC` | NMC | 5000 | 0.018 | ❌ |
| 10 | `TECH_NCA` | NCA | 4500 | 0.019 | ❌ |
| 11 | `TECH_NIFE` | Ni-Fe | 3000 | 0.025 | ❌ |
| 12 | `TECH_SODIUM` | Na-ion | 6000 | 0.014 | ❌ |
| 13 | `TECH_NIMH` | Ni-MH | 2500 | 0.020 | ❌ |
| 14 | `TECH_ALKALINE` | Alcaline | — | — | ❌ |
| 15 | `TECH_LCO` | Li-Co | 3500 | 0.022 | ❌ |
| 16 | `TECH_NIZN` | Ni-Zn | 3000 | 0.020 | ❌ |
| 17 | `TECH_NICD` | Ni-Cd | 2000 | 0.025 | ❌ |
| 18 | `TECH_PBC` | Plomb-carbone | 6000 | 0.020 | ✅ |
| 19 | `TECH_SUPERCAP` | Supercondensateur | 100000 | 0.001 | ❌ |

---

## Configuration compile-time

Définissez ces macros **avant** l'inclusion du header pour personnaliser :

```cpp
#define BLP_MAX_DOD_HISTORY 64          // Historique DOD (défaut: 48)
#define BLP_MAX_SOC_HISTORY 64          // Buffer Rainflow (défaut: 32)
#define BLP_MAX_REGRESSION_POINTS 32    // Points régression (défaut: 16)
#define BLP_DOD_EXCESS_THRESHOLD 75.0f  // Seuil DOD excédentaire (défaut: 80%)
#define BLP_SULFATATION_TAU_DAYS 20.0f // Constante temps sulfatation (défaut: 30j)
#define BLP_EOL_CAPACITY_PCT 75.0f     // Fin de vie à 75% (défaut: 80%)

// Bruits du Kalman 1D
#define BLP_KALMAN1D_Q_SLOPE 1e-6f
#define BLP_KALMAN1D_Q_BIAS 1e-5f
#define BLP_KALMAN1D_R_MEASURE 0.5f

#include <BatteryLifePredictor.h>
```

**Impact mémoire** :

| Paramètre | Octets supplémentaires |
|-----------|----------------------|
| `BLP_MAX_DOD_HISTORY` | +8 octets / unité |
| `BLP_MAX_SOC_HISTORY` | +8 octets / unité |
| `BLP_MAX_REGRESSION_POINTS` | +8 octets / unité |

---

## Exemples

### Exemple 1 — Prédiction basique

```cpp
// BatteryKalman.h (et Coulomb.h) doivent être inclus avant BatteryLifePredictor.h :
// update() appelle des méthodes de BatteryKalman (même si on passe nullptr).
#include <BatteryModels.h>
#include <Coulomb.h>
#include <BatteryKalman.h>
#include <BatteryLifePredictor.h>

BatteryModel model(TECH_LIFEPO4, 4, 100.0f, 5.0f);
BatteryLifePredictor life;

void setup() {
    Serial.begin(115200);
    life.begin(&model, nullptr, 100.0f);  // nullptr : pas de Kalman utilisé à l'exécution
}

void loop() {
    // Simulation d'un cycle journalier
    life.simulateOneCycle(100.0f, 50.0f, 25.0f, 24.0f);

    Serial.print("EFC: "); Serial.print(life.getEFC(), 2);
    Serial.print(" | SOH: "); Serial.print(life.getSOH(), 1);
    Serial.print("% | Vie: "); Serial.print(life.getRemainingLifeYears(), 1);
    Serial.println(" ans");

    delay(1000); // 1 cycle/simulation par seconde
}
```

### Exemple 2 — Intégration complète avec Kalman

Voir `Exemple_BatteryLifePredictor_v12.ino` pour un sketch complet avec :
- Lecture capteurs V/I/T
- Fusion Kalman 2D (SoC + capacité)
- Mise à jour prédicteur en temps réel
- Commandes série (`r`, `e`, `c`, `s`, `k`, `sim1`, `sim2`, `sim3`)
- Sauvegarde EEPROM horaire

### Exemple 3 — Simulation IEC 61427

```cpp
BatteryModel model(TECH_FLOODED, 6, 100.0f, 10.0f);
BatteryLifePredictor life;
life.begin(&model, nullptr, 100.0f);

// Simuler 10 macro-cycles IEC à 40°C (~10 ans de service PV)
life.simulateIEC61427(10, 40.0f);

Serial.print("SOH après 10 ans: ");
Serial.print(life.getSOH(), 1);
Serial.println("%");
```

### Exemple 4 — Rainflow et histogramme

```cpp
// Après plusieurs jours de fonctionnement réel
life.runRainflow();

uint16_t bins[5];
life.getRainflowHistogram(bins, 5);

Serial.println("Distribution des cycles:");
Serial.print("  0-20% DOD: "); Serial.println(bins[0]);
Serial.print(" 20-40% DOD: "); Serial.println(bins[1]);
Serial.print(" 40-60% DOD: "); Serial.println(bins[2]);
Serial.print(" 60-80% DOD: "); Serial.println(bins[3]);
Serial.print("80-100% DOD: "); Serial.println(bins[4]);
```

### Exemple 5 — Kalman 1D et prédiction EOL

```cpp
// Après 30 jours de collecte
Serial.print("Pente dégradation: ");
Serial.print(life.getKalmanSlope(), 6);
Serial.println(" %/jour");

Serial.print("Incertitude: ±");
Serial.print(life.getKalmanSlopeUncertainty(), 6);
Serial.println(" %/jour");

Serial.print("Jours avant fin de vie: ");
Serial.print(life.getKalmanDaysToEOL(), 0);
Serial.println(" jours");

Serial.print("Capacité dans 1 an: ");
Serial.print(life.getKalmanPredictedCapacity(365), 1);
Serial.println("%");
```

---

## Documentation IEC 61427

Le dossier `documentation_IEC_61427/` contient la documentation de référence sur la norme **IEC 61427** (méthodes d'essai des accumulateurs pour le stockage d'énergie renouvelable), utilisée pour le mode simulation :

| Fichier | Contenu |
|---------|---------|
| `documentation_IEC_61427.md` | Synthèse complète de la norme : protocoles d'essai (macro/micro-cycles), méthodes de comptage (EFC, Rainflow, Miner), modèles mathématiques (Arrhenius, sulfatation), dimensionnement PV |
| `simulation_monte_carlo_batteries.py` | Simulation Monte Carlo (Python, numpy/matplotlib) qui génère les graphiques PNG |
| `monte_carlo_lifetime.png`, `sensitivity_curves.png`, `tco_sensitivity.png` | Courbes générées : dispersion de vie, sensibilité des paramètres, coût total de possession |
| `IEC 61477.docx` | Version Word du document de synthèse |

Ce dossier est un **support hors librairie** : il documente et valide les hypothèses des modèles, mais n'est pas inclus dans le code Arduino (voir `simulateIEC61427()` en [Mode simulation](#mode-simulation)).

---

## Intégration avec BatteryModels & BatteryKalman

### Schéma de câblage conceptuel

```
Capteur tension ──┐
                  ├──→ BatteryKalman ──→ SoC fusionné ──→ BatteryLifePredictor
Capteur courant ──┤      (EKF 2D)           │                (cycles, EFC,
                  │                         │                 Miner, Rainflow,
Capteur temp ─────┘                         │                 Kalman 1D)
                                            ↓
                                      BatteryModels
                                      (OCV, Thévenin,
                                       détection état charge)
```

### Ordre d'appel dans `loop()`

```cpp
void loop() {
    // 1. Lecture capteurs
    float V = readVoltage();
    float I = readCurrent();
    float T = readTemperature();

    // 2. Coulomb counting
    coulomb.addMeasurement(I);

    // 3. Kalman 2D (fusion)
    kalman.update(V, I, T);

    // 4. Prédicteur de vie
    float dt_hours = 0.1f / 3600.0f;  // 100ms
    life.update(V, I, T, dt_hours);

    // 5. Actions BMS
    if (model.detectChargeState(V, I) == State_FLOAT) {
        life.triggerFullCharge();
    }
}
```

---

## Benchmark mémoire

| Plateforme | Flash (text) | RAM (bss+data) | Stack max |
|------------|-------------|----------------|-----------|
| Arduino Uno (AVR) | ~4,2 Ko | ~320 octets | ~80 octets |
| Arduino Mega 2560 | ~4,2 Ko | ~320 octets | ~80 octets |
| ESP32 | ~6,8 Ko | ~340 octets | ~120 octets |
| STM32F103 | ~5,1 Ko | ~330 octets | ~100 octets |

*Mesuré avec `BLP_MAX_DOD_HISTORY=48`, `BLP_MAX_SOC_HISTORY=32`, `BLP_MAX_REGRESSION_POINTS=16`.*

---

## Changelog

### v1.2.1 (2026-08-12)
- **Correction horloge interne** : accumulation fractionnaire des secondes dans `timestamp_hours`. Avant, `update()` à dt < 0,5 s (boucle > 2 Hz) perdait le temps et la régression/Kalman 1D ne convergeaient jamais en temps réel.
- **Correction compteurs d'heures** PSOC / égalisation : plus de perte par troncature sur les petits `dt_hours`.
- **Correction cadences par instance** : `lastRegHours`/`lastSimReg` ne sont plus des `static` partagés entre toutes les instances (une nouvelle simulation redémarre sa propre cadence de points).
- **Correction divers** : `_temp` initialisé à 25 °C, garde de `begin()`/`update()` sans modèle, correction d'une erreur de compilation dans l'exemple (`F("...")`).

### v1.2.0 (2026-08-08)
- Ajout **Kalman 1D** sur la dérive de capacité
- Ajout **mode simulation rapide** (`simulateProfile`, `simulateIEC61427`, `simulateOneCycle`)
- Magic number mis à jour `0x424C5032`

### v1.1.0 (2026-08-08)
- Ajout **Rainflow simplifié**
- Ajout **régression linéaire** capacité → EOL

### v1.0.0 (2026-08-08)
- Version initiale
- EFC, Miner, sulfatation plomb, correction température Arrhenius/Van't Hoff

---

## Licence

```
BatteryLifePredictor — Prédiction de vie résiduelle des batteries
Copyright (C) 2026  FOURNET Olivier <olivier.fournet@free.fr>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
```

**Auteur** : FOURNET Olivier  
**Email** : olivier.fournet@free.fr  
  
**Intégration** : Compatible [BatteryModels](https://github.com/Fo170/BatteryModels) & [BatteryKalman](https://github.com/Fo170/BatteryKalman)
