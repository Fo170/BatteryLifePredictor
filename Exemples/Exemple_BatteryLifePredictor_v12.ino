/**
 * @file Exemple_BatteryLifePredictor_v12.ino
 * @brief Exemple complet v1.2.1 — Kalman 1D + Simulation + Rainflow
 * @author FOURNET Olivier
 * @license GPL-3.0
 * @version 1.2.1
 */

#include <BatteryModels.h>
#include <BatteryKalman.h>
#include <Coulomb.h>
#include <EEPROM.h>
#include <BatteryLifePredictor.h>

// ============================================================
// CONFIGURATION
// ============================================================

#define BATTERY_TECH        TECH_LIFEPO4
#define CELL_COUNT          4
#define CAPACITY_AH         100.0f
#define R_INTERNAL_MOHM     5.0f

#define PIN_VOLTAGE         A0
#define PIN_CURRENT         A1
#define PIN_TEMP            A2

#define VOLTAGE_DIVIDER     0.0156f
#define CURRENT_SENSITIVITY 0.066f
#define TEMP_OFFSET         500.0f
#define TEMP_SENSITIVITY    10.0f

// ============================================================
// OBJETS
// ============================================================

BatteryModel    model(BATTERY_TECH, CELL_COUNT, CAPACITY_AH, R_INTERNAL_MOHM);
Coulomb         coulomb;
SoCData         socData;
KalmanState2D   kalmanState;
BatteryKalman   kalman(&socData, &kalmanState, &model, &coulomb);
BatteryLifePredictor life;

unsigned long lastUpdateMs = 0;
const unsigned long UPDATE_INTERVAL_MS = 100;

// ============================================================
// SETUP
// ============================================================

void setup() {
    Serial.begin(115200);
    while (!Serial && millis() < 3000);
    delay(500);

    Serial.println(F("========================================"));
    Serial.println(F("BatteryLifePredictor v1.2.1"));
    Serial.println(F("Kalman1D + Simulation + Rainflow"));
    Serial.println(F("========================================"));

    Serial.print(F("Technologie : "));
    Serial.println(model.getTechnologyName());
    Serial.print(F("Capacité : "));
    Serial.print(model.getNominalCapacity());
    Serial.println(F(" Ah"));

    life.begin(&model, &kalman, CAPACITY_AH);

    Serial.println(F("\n--- Commandes série ---"));
    Serial.println(F("  'r' = forcer Rainflow"));
    Serial.println(F("  'e' = égalisation"));
    Serial.println(F("  'c' = effacer régression"));
    Serial.println(F("  's' = état complet"));
    Serial.println(F("  'sim1' = simuler 1 an LiFePO4 @ 50% DOD"));
    Serial.println(F("  'sim2' = simuler 1 an Plomb @ 50% DOD, 35°C"));
    Serial.println(F("  'sim3' = simuler profil IEC 61427 (5 macro-cycles)"));
    Serial.println(F("  'k' = afficher état Kalman 1D"));
    Serial.println(F("\nFormat CSV toutes les 10s :"));
    Serial.println(F("time_ms,V,I,T,SOC,SOC_Kalman,SOH,EFC,Miner,Vie_ans,Sulf,DOD_excess,Reg_R2,Days_EOL,Kalman_EOL,Rain_n\n"));
}

// ============================================================
// LOOP
// ============================================================

void loop() {
    handleSerial();

    unsigned long now = millis();
    if (now - lastUpdateMs < UPDATE_INTERVAL_MS) return;
    lastUpdateMs = now;

    float rawV = analogRead(PIN_VOLTAGE);
    float rawI = analogRead(PIN_CURRENT);
    float rawT = analogRead(PIN_TEMP);

    float voltage = rawV * VOLTAGE_DIVIDER;
    float current = (rawI * (5.0f / 1024.0f) - 2.5f) / CURRENT_SENSITIVITY;
    float temperature = (rawT * (5.0f / 1024.0f) * 1000.0f - TEMP_OFFSET) / TEMP_SENSITIVITY;

    coulomb.addMeasurement(current);
    kalman.update(voltage, current, temperature);
    life.update(voltage, current, temperature, UPDATE_INTERVAL_MS / 3600000.0f);

    ChargeState state = model.detectChargeState(voltage, current);
    if (state == State_FLOAT) {
        life.triggerFullCharge();
        static unsigned long lastEq = 0;
        if (now - lastEq > 30UL * 24 * 3600 * 1000) {
            life.triggerEqualization();
            lastEq = now;
            Serial.println(F("[EVENT] Égalisation auto"));
        }
    }

    static unsigned long lastPrint = 0;
    if (now - lastPrint >= 10000) {
        lastPrint = now;
        printCSV(voltage, current, temperature);
    }

    static unsigned long lastSave = 0;
    if (now - lastSave >= 3600000) {
        lastSave = now;
        BatteryLifeState st = life.getState();
        EEPROM.put(0, st);
        Serial.println(F("[EVENT] État sauvegardé (EEPROM)"));
    }
}

// ============================================================
// AFFICHAGE CSV
// ============================================================

void printCSV(float voltage, float current, float temperature) {
    float soc_ocv = model.ocvToSoc(voltage, temperature);
    float soc_kalman = kalman.getSoC();

    Serial.print(millis());
    Serial.print(F(",")); Serial.print(voltage, 2);
    Serial.print(F(",")); Serial.print(current, 2);
    Serial.print(F(",")); Serial.print(temperature, 1);
    Serial.print(F(",")); Serial.print(soc_ocv, 1);
    Serial.print(F(",")); Serial.print(soc_kalman, 1);
    Serial.print(F(",")); Serial.print(life.getSOH(), 1);
    Serial.print(F(",")); Serial.print(life.getEFC(), 2);
    Serial.print(F(",")); Serial.print(life.getMinerDamage(), 4);
    Serial.print(F(",")); Serial.print(life.getRemainingLifeYears(), 1);
    Serial.print(F(",")); Serial.print(life.getSulfatationLevel(), 3);
    Serial.print(F(",")); Serial.print(life.getDODExcessAccumulated(), 1);
    Serial.print(F(",")); Serial.print(life.getCapacityRegressionR2(), 3);
    Serial.print(F(",")); Serial.print(life.getDaysToEOL(), 1);
    Serial.print(F(",")); Serial.print(life.getKalmanDaysToEOL(), 1);
    Serial.print(F(",")); Serial.print(life.getRainflowCount());
    Serial.println();
}

// ============================================================
// COMMANDES SÉRIE
// ============================================================

void handleSerial() {
    if (!Serial.available()) return;

    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    cmd.toLowerCase();

    if (cmd == "r") {
        life.runRainflow();
        printRainflow();
    } else if (cmd == "e") {
        life.triggerEqualization();
        Serial.println(F("[CMD] Égalisation déclenchée"));
    } else if (cmd == "c") {
        life.clearRegression();
        Serial.println(F("[CMD] Régression effacée"));
    } else if (cmd == "s") {
        printFullState();
    } else if (cmd == "k") {
        printKalman1D();
    } else if (cmd == "sim1") {
        runSimulationLiFePO4();
    } else if (cmd == "sim2") {
        runSimulationPlomb();
    } else if (cmd == "sim3") {
        runSimulationIEC61427();
    } else if (cmd.length() > 0) {
        Serial.print(F("[CMD] Inconnu : "));
        Serial.println(cmd);
    }
}

// ============================================================
// SIMULATIONS
// ============================================================

void runSimulationLiFePO4() {
    Serial.println(F("\n--- SIMULATION : LiFePO4 1 an @ 50% DOD, 25°C ---"));

    // Créer un nouveau prédicteur pour la simulation
    BatteryModel simModel(TECH_LIFEPO4, 4, 100.0f, 5.0f);
    BatteryLifePredictor sim;
    sim.begin(&simModel, nullptr, 100.0f);

    float dods[365];
    for (int i = 0; i < 365; i++) {
        dods[i] = 50.0f + random(-10, 10); // 50% ± 10%
    }

    uint16_t n = sim.simulateProfile(dods, nullptr, 365, 24.0f);

    Serial.print(F("Cycles simulés : ")); Serial.println(n);
    Serial.print(F("EFC : ")); Serial.print(sim.getEFC(), 2); Serial.println();
    Serial.print(F("SOH : ")); Serial.print(sim.getSOH(), 1); Serial.println(F(" %"));
    Serial.print(F("Miner : ")); Serial.print(sim.getMinerDamage(), 4); Serial.println();
    Serial.print(F("Vie estimée : ")); Serial.print(sim.getRemainingLifeYears(), 1); Serial.println(F(" ans"));
    Serial.print(F("DOD moyen : ")); Serial.print(sim.getAverageDOD(), 1); Serial.println(F(" %"));
    Serial.println();
}

void runSimulationPlomb() {
    Serial.println(F("\n--- SIMULATION : Plomb 1 an @ 50% DOD, 35°C ---"));

    BatteryModel simModel(TECH_FLOODED, 6, 100.0f, 10.0f);
    BatteryLifePredictor sim;
    sim.begin(&simModel, nullptr, 100.0f);

    float dods[365];
    float temps[365];
    for (int i = 0; i < 365; i++) {
        dods[i] = 50.0f + random(-10, 10);
        temps[i] = 35.0f;
    }

    uint16_t n = sim.simulateProfile(dods, temps, 365, 24.0f);

    Serial.print(F("Cycles simulés : ")); Serial.println(n);
    Serial.print(F("EFC : ")); Serial.print(sim.getEFC(), 2); Serial.println();
    Serial.print(F("SOH : ")); Serial.print(sim.getSOH(), 1); Serial.println(F(" %"));
    Serial.print(F("Miner : ")); Serial.print(sim.getMinerDamage(), 4); Serial.println();
    Serial.print(F("Sulfatation : ")); Serial.print(sim.getSulfatationLevel(), 3); Serial.println();
    Serial.print(F("Vie estimée : ")); Serial.print(sim.getRemainingLifeYears(), 1); Serial.println(F(" ans"));
    Serial.println();
}

void runSimulationIEC61427() {
    Serial.println(F("\n--- SIMULATION : IEC 61427 (5 macro-cycles, 40°C) ---"));

    BatteryModel simModel(TECH_FLOODED, 6, 100.0f, 10.0f);
    BatteryLifePredictor sim;
    sim.begin(&simModel, nullptr, 100.0f);

    uint16_t n = sim.simulateIEC61427(5, 40.0f);

    Serial.print(F("Micro-cycles simulés : ")); Serial.println(n);
    Serial.print(F("EFC : ")); Serial.print(sim.getEFC(), 2); Serial.println();
    Serial.print(F("SOH : ")); Serial.print(sim.getSOH(), 1); Serial.println(F(" %"));
    Serial.print(F("Miner : ")); Serial.print(sim.getMinerDamage(), 4); Serial.println();
    Serial.print(F("Sulfatation : ")); Serial.print(sim.getSulfatationLevel(), 3); Serial.println();
    Serial.println();
}

// ============================================================
// AFFICHAGE ÉTATS
// ============================================================

void printRainflow() {
    uint8_t n = life.getRainflowCount();
    Serial.println(F("\n--- Cycles Rainflow ---"));
    Serial.print(F("Total : ")); Serial.println(n);

    RainflowCycle rc;
    for (uint8_t i = 0; i < n; i++) {
        if (life.getRainflowCycle(i, rc)) {
            Serial.print(F("  ")); Serial.print(i + 1);
            Serial.print(F(" : amp=")); Serial.print(rc.amplitude, 1);
            Serial.print(F("%, mean=")); Serial.print(rc.mean, 1);
            Serial.print(F("%, t=")); Serial.print(rc.timestamp, 1);
            Serial.println(F("h"));
        }
    }

    uint16_t bins[5] = {0};
    life.getRainflowHistogram(bins, 5);
    Serial.println(F("\nHistogramme :"));
    Serial.print(F("  0-20% : ")); Serial.println(bins[0]);
    Serial.print(F("  20-40%: ")); Serial.println(bins[1]);
    Serial.print(F("  40-60%: ")); Serial.println(bins[2]);
    Serial.print(F("  60-80%: ")); Serial.println(bins[3]);
    Serial.print(F("  80-100%: ")); Serial.println(bins[4]);
    Serial.println();
}

void printKalman1D() {
    Serial.println(F("\n--- Kalman 1D (dérive capacité) ---"));
    Serial.print(F("Initialisé : ")); Serial.println(life.getKalmanSlope() != 0.0f ? "OUI" : "NON");
    Serial.print(F("Pente a : ")); Serial.print(life.getKalmanSlope(), 6); Serial.println(F(" %/jour"));
    Serial.print(F("Offset b : ")); Serial.print(life.getKalmanOffset(), 2); Serial.println(F(" %"));
    Serial.print(F("σ(a) : ")); Serial.print(life.getKalmanSlopeUncertainty(), 6); Serial.println(F(" %/jour"));
    Serial.print(F("Jours avant EOL : ")); Serial.print(life.getKalmanDaysToEOL(), 1); Serial.println();
    Serial.print(F("Capacité dans 365j : ")); Serial.print(life.getKalmanPredictedCapacity(365), 1); Serial.println(F(" %"));
    Serial.print(F("Capacité dans 1825j : ")); Serial.print(life.getKalmanPredictedCapacity(1825), 1); Serial.println(F(" %"));
    Serial.println();
}

void printFullState() {
    Serial.println(F("\n========== ÉTAT COMPLET =========="));
    Serial.print(F("Technologie : ")); Serial.println(life.getTechnologyName());
    Serial.print(F("SOH : ")); Serial.print(life.getSOH(), 1); Serial.println(F(" %"));
    Serial.print(F("EFC : ")); Serial.print(life.getEFC(), 2); Serial.println(F(" cycles"));
    Serial.print(F("Miner : ")); Serial.print(life.getMinerDamage(), 4); Serial.println();
    Serial.print(F("Vie résiduelle : ")); Serial.print(life.getRemainingLifeYears(), 1); Serial.println(F(" ans"));
    Serial.print(F("Sulfatation : ")); Serial.print(life.getSulfatationLevel(), 3); Serial.println();
    Serial.print(F("DOD moyen : ")); Serial.print(life.getAverageDOD(), 1); Serial.println(F(" %"));
    Serial.print(F("DOD max : ")); Serial.print(life.getMaxDOD(), 1); Serial.println(F(" %"));
    Serial.print(F("Cycles complets : ")); Serial.println(life.getTotalCycles());
    Serial.print(F("Micro-cycles : ")); Serial.println(life.getTotalMicrocycles());
    Serial.print(F("Heures PSOC : ")); Serial.println(life.getHoursInPSOC());

    Serial.print(F("\nRégression :\n"));
    Serial.print(F("  Points : ")); Serial.println(life.getRegressionPointCount());
    Serial.print(F("  Pente brute : ")); Serial.print(life.getCapacitySlopePercentPerDay(), 4); Serial.println(F(" %/jour"));
    Serial.print(F("  R² : ")); Serial.print(life.getCapacityRegressionR2(), 3); Serial.println();
    Serial.print(F("  Jours EOL (brut) : ")); Serial.print(life.getDaysToEOL(), 1); Serial.println();

    Serial.print(F("\nKalman 1D :\n"));
    Serial.print(F("  Pente lissée : ")); Serial.print(life.getKalmanSlope(), 6); Serial.println(F(" %/jour"));
    Serial.print(F("  σ(a) : ")); Serial.print(life.getKalmanSlopeUncertainty(), 6); Serial.println();
    Serial.print(F("  Jours EOL (Kalman) : ")); Serial.print(life.getKalmanDaysToEOL(), 1); Serial.println();

    Serial.print(F("\nRainflow : ")); Serial.print(life.getRainflowCount()); Serial.println(F(" cycles"));
    Serial.println(F("==================================\n"));
}
