/**
 * @file BatteryLifePredictor.h
 * @brief Prédiction de vie résiduelle des batteries — v1.2.1
 * @author FOURNET Olivier (olivier.fournet@free.fr)
 * @license GPL-3.0
 * @version 1.2.1
 * @date 2026-08-12
 * 
 * Ajouts v1.2.0 :
 *  - Kalman 1D sur la dérive de capacité (lissage régression, prédiction EOL robuste)
 *  - Mode simulation rapide (profils de cycles prédéfinis, validation sans matériel)
 * 
 * Corrections v1.2.1 :
 *  - Horloge interne fiable : accumulation fractionnaire des secondes dans
 *    `timestamp_hours` (avant, les appels `update()` à dt < 0,5 s perdaient
 *    le temps et la régression/Kalman 1D ne convergeaient jamais en réel).
 *  - Compteurs d'heures PSOC/égalisation sans perte par troncature.
 *  - Cadences de régression/Kalman par instance (`_lastRegHours`, `_lastSimReg`
 *    ne sont plus des `static` partagés entre toutes les instances).
 *  - `_temp` initialisé à 25 °C ; garde d'`update()`/`begin()` sans modèle.
 * 
 * Intégration : BatteryModels (Fo170) + BatteryKalman (Fo170)
 * Compatible : Arduino AVR, ESP32, ARM
 */

#ifndef BATTERY_LIFE_PREDICTOR_H
#define BATTERY_LIFE_PREDICTOR_H

#include <Arduino.h>

// ============================================================
// CONFIGURATION COMPILE-TIME
// ============================================================

#ifndef BLP_MAX_DOD_HISTORY
#define BLP_MAX_DOD_HISTORY 48
#endif

#ifndef BLP_MAX_SOC_HISTORY
#define BLP_MAX_SOC_HISTORY 32
#endif

#ifndef BLP_MAX_REGRESSION_POINTS
#define BLP_MAX_REGRESSION_POINTS 16
#endif

#ifndef BLP_DOD_EXCESS_THRESHOLD
#define BLP_DOD_EXCESS_THRESHOLD 80.0f
#endif

#ifndef BLP_SULFATATION_TAU_DAYS
#define BLP_SULFATATION_TAU_DAYS 30.0f
#endif

#ifndef BLP_EOL_CAPACITY_PCT
#define BLP_EOL_CAPACITY_PCT 80.0f
#endif

#ifndef BLP_KALMAN1D_Q_SLOPE
#define BLP_KALMAN1D_Q_SLOPE 1e-6f    // Process noise pente (%/jour)²
#endif

#ifndef BLP_KALMAN1D_Q_BIAS
#define BLP_KALMAN1D_Q_BIAS 1e-5f     // Process noise offset (%)
#endif

#ifndef BLP_KALMAN1D_R_MEASURE
#define BLP_KALMAN1D_R_MEASURE 0.5f   // Bruit de mesure capacité (%²)
#endif

// ============================================================
// STRUCTURES
// ============================================================

struct BatteryLifeParams {
    float N0;
    float k_dod;
    float Ea_kJmol;
    float eta_energy;
    float dod_max_rec;
    bool  has_sulfatation;
    float sulf_k;
    float sulf_alpha;
    float temp_vanthoff;
};

struct RainflowCycle {
    float amplitude;
    float mean;
    float timestamp;
};

/**
 * @brief État du Kalman 1D sur la dérive de capacité
 * État : [pente a (%/jour), offset b (%)]
 */
struct Kalman1DState {
    float a;        // Pente estimée (%/jour)
    float b;        // Offset estimée (%)
    float P[4];     // Matrice de covariance 2x2 [P00, P01, P10, P11]
    uint8_t initialized;
};

/**
 * @brief État persistant complet
 */
struct BatteryLifeState {
    uint32_t magic;               // 0x424C5032 ('BLP2')
    uint16_t version;             // 3

    // --- Comptage classique ---
    float    efc_total;
    float    miner_damage;
    float    dod_excess_acc;
    float    sulfatation_level;
    float    last_soc;
    float    last_dod;
    uint32_t total_cycles;
    uint32_t total_microcycles;
    uint32_t hours_in_psoc;
    uint32_t hours_since_eq;
    uint32_t timestamp_hours;
    float    capacity_initial;
    float    capacity_current;

    // --- Historique DOD ---
    uint8_t  dod_history_count;
    uint8_t  reserved[3];
    float    dod_history[BLP_MAX_DOD_HISTORY];
    float    dod_history_time[BLP_MAX_DOD_HISTORY];

    // --- Historique SOC pour Rainflow ---
    uint8_t  soc_history_count;
    float    soc_history[BLP_MAX_SOC_HISTORY];
    float    soc_history_time[BLP_MAX_SOC_HISTORY];

    // --- Régression capacité ---
    uint8_t  reg_count;
    float    reg_time_days[BLP_MAX_REGRESSION_POINTS];
    float    reg_capacity_pct[BLP_MAX_REGRESSION_POINTS];

    // --- Kalman 1D dérive capacité ---
    Kalman1DState kalman1d;

    // --- Résultats Rainflow ---
    uint8_t  rainflow_count;
    RainflowCycle rainflow_cycles[BLP_MAX_DOD_HISTORY / 2];
};

// ============================================================
// PARAMÈTRES PAR TECHNOLOGIE (PROGMEM)
// ============================================================

static const BatteryLifeParams BLP_PARAMS[] PROGMEM = {
    {4000.0f, 0.020f, 50.0f, 0.70f, 50.0f, false, 0.0f, 0.0f, 0.0f},   // 0 UNKNOWN
    {5500.0f, 0.022f, 55.0f, 0.70f, 50.0f, true,  0.015f, 1.5f, 15.0f}, // 1 FLOODED
    {5000.0f, 0.023f, 55.0f, 0.70f, 50.0f, true,  0.018f, 1.5f, 15.0f}, // 2 AGM
    {5200.0f, 0.022f, 55.0f, 0.72f, 50.0f, true,  0.016f, 1.5f, 15.0f}, // 3 GEL
    {8000.0f, 0.012f, 50.0f, 0.93f, 80.0f, false, 0.0f, 0.0f, 0.0f},   // 4 LIFEPO4
    {5000.0f, 0.018f, 55.0f, 0.92f, 80.0f, false, 0.0f, 0.0f, 0.0f},   // 5 LION
    {4500.0f, 0.020f, 55.0f, 0.92f, 80.0f, false, 0.0f, 0.0f, 0.0f},   // 6 LIPO
    {20000.0f,0.008f, 45.0f, 0.88f, 90.0f, false, 0.0f, 0.0f, 0.0f},   // 7 LTO
    {4000.0f, 0.015f, 50.0f, 0.92f, 80.0f, false, 0.0f, 0.0f, 0.0f},   // 8 LMNO
    {5000.0f, 0.018f, 55.0f, 0.92f, 80.0f, false, 0.0f, 0.0f, 0.0f},   // 9 NMC
    {4500.0f, 0.019f, 60.0f, 0.92f, 80.0f, false, 0.0f, 0.0f, 0.0f},   // 10 NCA
    {3000.0f, 0.025f, 50.0f, 0.65f, 60.0f, false, 0.0f, 0.0f, 0.0f},   // 11 NIFE
    {6000.0f, 0.014f, 45.0f, 0.90f, 80.0f, false, 0.0f, 0.0f, 0.0f},   // 12 SODIUM
    {2500.0f, 0.020f, 50.0f, 0.70f, 60.0f, false, 0.0f, 0.0f, 0.0f},   // 13 NIMH
    {0.0f,    0.0f,   0.0f,  0.0f,  0.0f,  false, 0.0f, 0.0f, 0.0f},   // 14 ALKALINE
    {3500.0f, 0.022f, 55.0f, 0.92f, 80.0f, false, 0.0f, 0.0f, 0.0f},   // 15 LCO
    {3000.0f, 0.020f, 50.0f, 0.75f, 60.0f, false, 0.0f, 0.0f, 0.0f},   // 16 NIZN
    {2000.0f, 0.025f, 50.0f, 0.70f, 60.0f, false, 0.0f, 0.0f, 0.0f},   // 17 NICD
    {6000.0f, 0.020f, 55.0f, 0.75f, 50.0f, true,  0.012f, 1.5f, 15.0f},// 18 PBC
    {100000.0f,0.001f,30.0f, 0.95f, 100.0f,false, 0.0f, 0.0f, 0.0f},   // 19 SUPERCAP
};

static const float BLP_R = 8.314f;

// ============================================================
// CLASSE PRINCIPALE
// ============================================================

class BatteryLifePredictor {
public:
    BatteryLifePredictor() : _model(nullptr), _kalman(nullptr), _params(nullptr) {
        resetState();
    }

    void begin(class BatteryModel* model, class BatteryKalman* kalman = nullptr,
               float capacityInitial = 0.0f) {
        if (!model) return;
        _model = model;
        _kalman = kalman;
        _tech = (uint8_t)model->getTechnology();
        if (_tech < 20) {
            _params = &BLP_PARAMS[_tech];
        } else {
            _params = &BLP_PARAMS[0];
        }
        resetState();
        if (capacityInitial > 0.0f) {
            _state.capacity_initial = capacityInitial;
            _state.capacity_current = capacityInitial;
        } else {
            _state.capacity_initial = model->getNominalCapacity();
            _state.capacity_current = model->getNominalCapacity();
        }
    }

    void reset() {
        float cap_init = _state.capacity_initial;
        resetState();
        _state.capacity_initial = cap_init;
        _state.capacity_current = cap_init;
    }

    // ============================================================
    // UPDATE PRINCIPAL
    // ============================================================

    void update(float voltage, float current, float temperature, float dt_hours) {
        if (!_model || dt_hours <= 0.0f || dt_hours > 1.0f) return;

        accumulateTime(dt_hours);
        _temp = temperature;

        float soc = _model->ocvToSoc(voltage, temperature);
        if (_kalman) {
            float soc_kalman = _kalman->getSoC();
            soc = 0.7f * soc_kalman + 0.3f * soc;
        }
        soc = constrain(soc, 0.0f, 100.0f);

        pushSocHistory(soc, (float)_state.timestamp_hours / 3600.0f);

        float dod = 0.0f;
        if (_state.last_soc >= 0.0f) {
            float delta_soc = _state.last_soc - soc;
            if (delta_soc > 0.5f) {
                dod = delta_soc;
                _state.total_microcycles++;

                uint8_t idx = _state.dod_history_count % BLP_MAX_DOD_HISTORY;
                _state.dod_history[idx] = dod;
                _state.dod_history_time[idx] = (float)_state.timestamp_hours / 3600.0f;
                _state.dod_history_count++;

                _state.efc_total += (dod / 100.0f);

                float excess = max(0.0f, dod - BLP_DOD_EXCESS_THRESHOLD);
                _state.dod_excess_acc += excess;

                updateMiner(dod, temperature);

                if (_params && pgm_read_byte(&_params->has_sulfatation)) {
                    updateSulfatation(soc, dt_hours);
                }

                if (dod >= 80.0f) {
                    _state.total_cycles++;
                }
            }
        }
        _state.last_soc = soc;
        _state.last_dod = dod;

        if (_kalman) {
            float aging = _model->getCapacityFactorAging();
            _state.capacity_current = _state.capacity_initial * aging;
        } else {
            _state.capacity_current = _state.capacity_initial * 
                (1.0f - 0.2f * (_state.efc_total / getNominalCyclesAtDOD(50.0f, temperature)));
            _state.capacity_current = max(_state.capacity_current, _state.capacity_initial * 0.5f);
        }

        float nowHours = (float)_state.timestamp_hours / 3600.0f;
        if (nowHours - _lastRegHours >= 24.0f) {
            _lastRegHours = nowHours;
            float capPct = (_state.capacity_current / _state.capacity_initial) * 100.0f;
            pushRegressionPoint(_lastRegHours / 24.0f, capPct);
            updateKalman1D(_lastRegHours / 24.0f, capPct);
        }

        if (_state.total_microcycles % 6 == 0 && _state.total_microcycles > 0) {
            runRainflow();
        }
    }

    // ============================================================
    // MODE SIMULATION RAPIDE
    // ============================================================

    /**
     * @brief Simule un profil de cycles prédéfini sans matériel
     * @param dodArray Tableau des DOD successifs (%)
     * @param tempArray Tableau des températures (°C), peut être nullptr (25°C par défaut)
     * @param n Nombre de cycles à simuler
     * @param dt_hours Intervalle entre cycles (heures)
     * @return Nombre de cycles simulés
     * 
     * Exemple : simuler 365 jours de cycles journaliers
     *   float dods[365] = {50,45,60,55,...};
     *   life.simulateProfile(dods, nullptr, 365, 24.0f);
     */
    uint16_t simulateProfile(const float* dodArray, const float* tempArray,
                              uint16_t n, float dt_hours) {
        if (!dodArray || n == 0) return 0;
        uint16_t simulated = 0;

        for (uint16_t i = 0; i < n; i++) {
            float dod = constrain(dodArray[i], 1.0f, 100.0f);
            float temp = (tempArray != nullptr) ? tempArray[i] : 25.0f;

            // Simuler une décharge puis une recharge complète
            float soc_start = 100.0f;
            float soc_end = max(0.0f, soc_start - dod);

            // Injection dans le modèle
            simulateOneCycle(soc_start, soc_end, temp, dt_hours);
            simulated++;
        }
        return simulated;
    }

    /**
     * @brief Simule un seul cycle (décharge + recharge)
     * @param socStart SOC initial (%)
     * @param socEnd SOC final après décharge (%)
     * @param temperature Température (°C)
     * @param dt_hours Durée du cycle (heures)
     */
    void simulateOneCycle(float socStart, float socEnd, float temperature, float dt_hours) {
        socStart = constrain(socStart, 0.0f, 100.0f);
        socEnd = constrain(socEnd, 0.0f, socStart);

        accumulateTime(dt_hours);
        _temp = temperature;

        float dod = socStart - socEnd;
        if (dod > 0.5f) {
            _state.total_microcycles++;

            uint8_t idx = _state.dod_history_count % BLP_MAX_DOD_HISTORY;
            _state.dod_history[idx] = dod;
            _state.dod_history_time[idx] = (float)_state.timestamp_hours / 3600.0f;
            _state.dod_history_count++;

            _state.efc_total += (dod / 100.0f);

            float excess = max(0.0f, dod - BLP_DOD_EXCESS_THRESHOLD);
            _state.dod_excess_acc += excess;

            updateMiner(dod, temperature);

            if (_params && pgm_read_byte(&_params->has_sulfatation)) {
                // Simuler PSOC pendant la décharge
                float psoc_frac = (socEnd < 80.0f) ? 0.5f : 0.0f;
                updateSulfatation(socEnd, dt_hours * psoc_frac);
            }

            if (dod >= 80.0f) {
                _state.total_cycles++;
            }
        }

        // Mise à jour capacité simulée
        _state.capacity_current = _state.capacity_initial * 
            (1.0f - 0.2f * (_state.efc_total / getNominalCyclesAtDOD(50.0f, temperature)));
        _state.capacity_current = max(_state.capacity_current, _state.capacity_initial * 0.5f);

        _state.last_soc = socEnd;
        _state.last_dod = dod;

        // Mise à jour régression et Kalman 1D si assez de temps passé
        float nowHours = (float)_state.timestamp_hours / 3600.0f;
        if (nowHours - _lastSimReg >= 24.0f) {
            _lastSimReg = nowHours;
            float capPct = (_state.capacity_current / _state.capacity_initial) * 100.0f;
            pushRegressionPoint(_lastSimReg / 24.0f, capPct);
            updateKalman1D(_lastSimReg / 24.0f, capPct);
        }
    }

    /**
     * @brief Simule un profil IEC 61427 (macro-cycles)
     * @param nMacroCycles Nombre de macro-cycles à simuler
     * @param temperature Température d'essai (°C)
     * @return Nombre de micro-cycles simulés
     */
    uint16_t simulateIEC61427(uint16_t nMacroCycles, float temperature) {
        uint16_t totalMicro = 0;
        for (uint16_t m = 0; m < nMacroCycles; m++) {
            // Phase A : 50 cycles @ DOD 40% (SOC 100→60)
            for (uint8_t i = 0; i < 50; i++) {
                simulateOneCycle(100.0f, 60.0f, temperature, 0.5f);
                totalMicro++;
            }
            // Phase B : 100 cycles @ DOD 20% (SOC 100→80)
            for (uint8_t i = 0; i < 100; i++) {
                simulateOneCycle(100.0f, 80.0f, temperature, 0.5f);
                totalMicro++;
            }
        }
        return totalMicro;
    }

    // ============================================================
    // ACTIONS BMS
    // ============================================================

    void triggerEqualization() {
        if (_params && pgm_read_byte(&_params->has_sulfatation)) {
            _state.sulfatation_level *= 0.3f;
            _state.hours_since_eq = 0;
        }
    }

    void triggerFullCharge() {
        _state.hours_in_psoc = 0;
    }

    // ============================================================
    // GETTERS — PRÉDICTION DE VIE
    // ============================================================

    float getRemainingLifeYears(float dodAssumed = 0.0f) const {
        if (!_params) return 0.0f;
        float dod = (dodAssumed > 0.0f) ? dodAssumed : getAverageDOD();
        if (dod < 1.0f) dod = 10.0f;
        float cycles_left = getRemainingCycles(dod, _temp);
        return cycles_left / 365.0f;
    }

    float getRemainingCycles(float dod, float temperature) const {
        if (!_params) return 0.0f;
        float N_total = getNominalCyclesAtDOD(dod, temperature);
        float N_consumed = _state.efc_total * (100.0f / dod);
        if (pgm_read_byte(&_params->has_sulfatation)) {
            float sulf_factor = 1.0f + _state.sulfatation_level * 0.5f;
            N_consumed *= sulf_factor;
        }
        float remaining = N_total - N_consumed;
        return max(remaining, 0.0f);
    }

    float getRemainingCapacityPercent() const {
        if (_state.capacity_initial <= 0.0f) return 100.0f;
        float pct = (_state.capacity_current / _state.capacity_initial) * 100.0f;
        return constrain(pct, 0.0f, 100.0f);
    }

    float getSOH() const { return getRemainingCapacityPercent(); }
    float getEFC() const { return _state.efc_total; }
    float getMinerDamage() const { return _state.miner_damage; }
    float getSulfatationLevel() const { return _state.sulfatation_level; }
    float getDODExcessAccumulated() const { return _state.dod_excess_acc; }
    uint32_t getTotalCycles() const { return _state.total_cycles; }
    uint32_t getTotalMicrocycles() const { return _state.total_microcycles; }
    uint32_t getHoursInPSOC() const { return _state.hours_in_psoc; }
    uint32_t getHoursSinceEqualization() const { return _state.hours_since_eq; }

    float getAverageDOD() const {
        uint8_t n = min(_state.dod_history_count, (uint8_t)BLP_MAX_DOD_HISTORY);
        if (n == 0) return 50.0f;
        float sum = 0.0f;
        for (uint8_t i = 0; i < n; i++) sum += _state.dod_history[i];
        return sum / (float)n;
    }

    float getMaxDOD() const {
        uint8_t n = min(_state.dod_history_count, (uint8_t)BLP_MAX_DOD_HISTORY);
        if (n == 0) return 0.0f;
        float max_dod = 0.0f;
        for (uint8_t i = 0; i < n; i++) {
            if (_state.dod_history[i] > max_dod) max_dod = _state.dod_history[i];
        }
        return max_dod;
    }

    const BatteryLifeState& getState() const { return _state; }

    void setState(const BatteryLifeState& state) {
        if (state.magic == 0x424C5032 && state.version == 3) {
            _state = state;
        }
    }

    static size_t getStateSize() { return sizeof(BatteryLifeState); }

    const char* getTechnologyName() const {
        if (_model) return _model->getTechnologyName();
        return "UNKNOWN";
    }

    // ============================================================
    // GETTERS — CALCULS INTERMÉDIAIRES
    // ============================================================

    float getNominalCyclesAtDOD(float dod, float temperature) const {
        if (!_params) return 0.0f;
        float N0 = pgm_read_float(&_params->N0);
        float k = pgm_read_float(&_params->k_dod);
        float Ea = pgm_read_float(&_params->Ea_kJmol);
        float N = N0 * expf(-k * dod);
        float T = temperature + 273.15f;
        float T_ref = 298.15f;
        float temp_factor = expf(-Ea * 1000.0f / BLP_R * (1.0f / T - 1.0f / T_ref));
        N *= temp_factor;
        if (pgm_read_byte(&_params->has_sulfatation) && temperature > 25.0f) {
            float vh = pgm_read_float(&_params->temp_vanthoff);
            if (vh > 0.0f) N *= powf(0.5f, (temperature - 25.0f) / vh);
        }
        return max(N, 1.0f);
    }

    float getSulfatationFactor() const {
        if (!_params || !pgm_read_byte(&_params->has_sulfatation)) return 1.0f;
        float psoc_penalty = 1.0f + (_state.hours_in_psoc / 24.0f) * 0.30f;
        float eq_bonus = max(0.5f, 1.0f - (30.0f / max((float)_state.hours_since_eq / 24.0f, 1.0f)) * 0.30f);
        return psoc_penalty * eq_bonus;
    }

    // ============================================================
    // RAINFLOW
    // ============================================================

    uint8_t getRainflowCount() const {
        return _state.rainflow_count;
    }

    bool getRainflowCycle(uint8_t index, RainflowCycle& out) const {
        if (index >= _state.rainflow_count) return false;
        out = _state.rainflow_cycles[index];
        return true;
    }

    void runRainflow() {
        if (_state.soc_history_count < 4) return;

        _state.rainflow_count = 0;
        uint8_t n = min(_state.soc_history_count, (uint8_t)BLP_MAX_SOC_HISTORY);

        float extrema[BLP_MAX_SOC_HISTORY];
        float extrema_time[BLP_MAX_SOC_HISTORY];
        uint8_t ext_count = 0;

        for (uint8_t i = 1; i < n - 1; i++) {
            float prev = _state.soc_history[i - 1];
            float curr = _state.soc_history[i];
            float next = _state.soc_history[i + 1];
            if ((curr > prev && curr > next) || (curr < prev && curr < next)) {
                extrema[ext_count] = curr;
                extrema_time[ext_count] = _state.soc_history_time[i];
                ext_count++;
                if (ext_count >= BLP_MAX_SOC_HISTORY) break;
            }
        }

        if (ext_count < 3) return;

        for (uint8_t i = 0; i < ext_count - 2 && _state.rainflow_count < (BLP_MAX_DOD_HISTORY / 2); i++) {
            float p1 = extrema[i];
            float v = extrema[i + 1];
            float p2 = extrema[i + 2];

            bool isValley = (v < p1 && v < p2);
            bool isPeak = (v > p1 && v > p2);

            if (isValley || isPeak) {
                float amplitude = fabsf(p1 - v) + fabsf(p2 - v);
                amplitude = min(amplitude, fabsf(p1 - p2));
                float mean = (p1 + p2) / 2.0f;

                if (amplitude >= 2.0f) {
                    uint8_t rc = _state.rainflow_count;
                    _state.rainflow_cycles[rc].amplitude = amplitude;
                    _state.rainflow_cycles[rc].mean = mean;
                    _state.rainflow_cycles[rc].timestamp = extrema_time[i + 1];
                    _state.rainflow_count++;
                }
            }
        }
    }

    void getRainflowHistogram(uint16_t* bins, uint8_t nBins) const {
        if (!bins || nBins == 0) return;
        for (uint8_t i = 0; i < nBins; i++) bins[i] = 0;
        float binWidth = 100.0f / (float)nBins;
        for (uint8_t i = 0; i < _state.rainflow_count; i++) {
            uint8_t idx = (uint8_t)(_state.rainflow_cycles[i].amplitude / binWidth);
            if (idx >= nBins) idx = nBins - 1;
            bins[idx]++;
        }
    }

    // ============================================================
    // RÉGRESSION LINÉAIRE
    // ============================================================

    float getCapacitySlopePercentPerDay() const {
        float a, b, r2;
        if (!computeRegression(a, b, r2)) return 0.0f;
        return a;
    }

    float getCapacityRegressionR2() const {
        float a, b, r2;
        computeRegression(a, b, r2);
        return r2;
    }

    float getDaysToEOL() const {
        float a, b, r2;
        if (!computeRegression(a, b, r2)) return -1.0f;
        if (a >= 0.0f) return -1.0f;
        float days = (BLP_EOL_CAPACITY_PCT - b) / a;
        return max(days, 0.0f);
    }

    float getEOLDay() const {
        float days = getDaysToEOL();
        if (days < 0.0f) return -1.0f;
        float currentDay = 0.0f;
        if (_state.reg_count > 0) {
            currentDay = _state.reg_time_days[_state.reg_count - 1];
        }
        return currentDay + days;
    }

    uint8_t getRegressionPointCount() const {
        return _state.reg_count;
    }

    void addRegressionPoint(float days, float capacityPct) {
        pushRegressionPoint(days, capacityPct);
        updateKalman1D(days, capacityPct);
    }

    void clearRegression() {
        _state.reg_count = 0;
        memset(_state.reg_time_days, 0, sizeof(_state.reg_time_days));
        memset(_state.reg_capacity_pct, 0, sizeof(_state.reg_capacity_pct));
        resetKalman1D();
    }

    // ============================================================
    // KALMAN 1D — DÉRIVE CAPACITÉ
    // ============================================================

    /**
     * @brief Pente lissée par Kalman 1D (%/jour) — plus robuste que régression brute
     */
    float getKalmanSlope() const {
        if (!_state.kalman1d.initialized) return 0.0f;
        return _state.kalman1d.a;
    }

    /**
     * @brief Offset lissé par Kalman 1D (%)
     */
    float getKalmanOffset() const {
        if (!_state.kalman1d.initialized) return 100.0f;
        return _state.kalman1d.b;
    }

    /**
     * @brief Incertitude sur la pente (écart-type, %/jour)
     */
    float getKalmanSlopeUncertainty() const {
        if (!_state.kalman1d.initialized) return 999.0f;
        return sqrtf(max(_state.kalman1d.P[0], 0.0f));
    }

    /**
     * @brief Jours avant EOL estimés par Kalman 1D
     */
    float getKalmanDaysToEOL() const {
        if (!_state.kalman1d.initialized) return -1.0f;
        float a = _state.kalman1d.a;
        float b = _state.kalman1d.b;
        if (a >= -1e-6f) return -1.0f;
        float currentDay = 0.0f;
        if (_state.reg_count > 0) {
            currentDay = _state.reg_time_days[_state.reg_count - 1];
        }
        float days = (BLP_EOL_CAPACITY_PCT - (a * currentDay + b)) / a;
        return max(days, 0.0f);
    }

    /**
     * @brief Prédiction de capacité à un jour futur par Kalman 1D
     * @param daysFromNow Jours dans le futur
     * @return Capacité prédite (%)
     */
    float getKalmanPredictedCapacity(float daysFromNow) const {
        if (!_state.kalman1d.initialized) return 100.0f;
        float currentDay = 0.0f;
        if (_state.reg_count > 0) {
            currentDay = _state.reg_time_days[_state.reg_count - 1];
        }
        float t = currentDay + daysFromNow;
        float cap = _state.kalman1d.a * t + _state.kalman1d.b;
        return constrain(cap, 0.0f, 100.0f);
    }

    /**
     * @brief Réinitialise le Kalman 1D
     */
    void resetKalman1D() {
        _state.kalman1d.a = 0.0f;
        _state.kalman1d.b = 100.0f;
        _state.kalman1d.P[0] = 1.0f;
        _state.kalman1d.P[1] = 0.0f;
        _state.kalman1d.P[2] = 0.0f;
        _state.kalman1d.P[3] = 10.0f;
        _state.kalman1d.initialized = 0;
    }

private:
    class BatteryModel* _model;
    class BatteryKalman* _kalman;
    const BatteryLifeParams* _params;
    BatteryLifeState _state;
    float _temp;
    uint8_t _tech;

    // Accumulateurs fractionnaires (hors BatteryLifeState : non persistés)
    float _secondsCarry;    // Retenue de secondes pour timestamp_hours
    float _fracEqHours;     // Retenue d'heures depuis dernière égalisation
    float _fracPsocHours;   // Retenue d'heures en PSOC
    float _lastRegHours;    // Dernier point de régression (voie update)
    float _lastSimReg;      // Dernier point de régression (voie simulation)

    void resetState() {
        memset(&_state, 0, sizeof(BatteryLifeState));
        _state.magic = 0x424C5032;
        _state.version = 3;
        _state.last_soc = -1.0f;
        _temp = 25.0f;
        _secondsCarry = 0.0f;
        _fracEqHours = 0.0f;
        _fracPsocHours = 0.0f;
        _lastRegHours = 0.0f;
        _lastSimReg = 0.0f;
        resetKalman1D();
    }

    // Accumule dt (heures) dans timestamp_hours (secondes entières) sans perte
    // de fraction pour les petits dt (boucles > 1 Hz).
    void accumulateTime(float dt_hours) {
        _secondsCarry += dt_hours * 3600.0f;
        if (_secondsCarry >= 1.0f) {
            uint32_t whole = (uint32_t)_secondsCarry;
            _state.timestamp_hours += whole;
            _secondsCarry -= (float)whole;
        }
    }

    // Ajoute whole heures entières à un compteur, garde la fraction.
    void addHours(uint32_t& counter, float& frac, float dt_hours) {
        frac += dt_hours;
        if (frac >= 1.0f) {
            uint32_t whole = (uint32_t)frac;
            counter += whole;
            frac -= (float)whole;
        }
    }

    void updateMiner(float dod, float temperature) {
        if (!_params) return;
        float N_ref = getNominalCyclesAtDOD(dod, temperature);
        if (N_ref > 0.0f) _state.miner_damage += (1.0f / N_ref);
    }

    void updateSulfatation(float soc, float dt_hours) {
        if (!_params) return;
        if (soc < 80.0f) {
            addHours(_state.hours_in_psoc, _fracPsocHours, dt_hours);
        }
        addHours(_state.hours_since_eq, _fracEqHours, dt_hours);
        float sulf_k = pgm_read_float(&_params->sulf_k);
        float sulf_a = pgm_read_float(&_params->sulf_alpha);
        float soc_norm = (100.0f - soc) / 100.0f;
        float growth = sulf_k * powf(soc_norm, sulf_a) * dt_hours;
        _state.sulfatation_level += growth;
        _state.sulfatation_level = min(_state.sulfatation_level, 1.0f);
        float decay = expf(-dt_hours / (BLP_SULFATATION_TAU_DAYS * 24.0f));
        _state.sulfatation_level *= decay;
    }

    void pushSocHistory(float soc, float time_hours) {
        uint8_t idx = _state.soc_history_count % BLP_MAX_SOC_HISTORY;
        _state.soc_history[idx] = soc;
        _state.soc_history_time[idx] = time_hours;
        _state.soc_history_count++;
    }

    void pushRegressionPoint(float days, float capacityPct) {
        if (_state.reg_count >= BLP_MAX_REGRESSION_POINTS) {
            for (uint8_t i = 0; i < BLP_MAX_REGRESSION_POINTS - 1; i++) {
                _state.reg_time_days[i] = _state.reg_time_days[i + 1];
                _state.reg_capacity_pct[i] = _state.reg_capacity_pct[i + 1];
            }
            _state.reg_count = BLP_MAX_REGRESSION_POINTS - 1;
        }
        _state.reg_time_days[_state.reg_count] = days;
        _state.reg_capacity_pct[_state.reg_count] = constrain(capacityPct, 0.0f, 100.0f);
        _state.reg_count++;
    }

    bool computeRegression(float& outA, float& outB, float& outR2) const {
        uint8_t n = _state.reg_count;
        if (n < 3) return false;

        float sumX = 0.0f, sumY = 0.0f, sumXY = 0.0f, sumX2 = 0.0f, sumY2 = 0.0f;
        for (uint8_t i = 0; i < n; i++) {
            float x = _state.reg_time_days[i];
            float y = _state.reg_capacity_pct[i];
            sumX += x;
            sumY += y;
            sumXY += x * y;
            sumX2 += x * x;
            sumY2 += y * y;
        }
        float denom = n * sumX2 - sumX * sumX;
        if (fabsf(denom) < 1e-6f) return false;

        outA = (n * sumXY - sumX * sumY) / denom;
        outB = (sumY * sumX2 - sumX * sumXY) / denom;

        float ssTot = sumY2 - (sumY * sumY) / n;
        float ssRes = sumY2 - outB * sumY - outA * sumXY;
        if (ssTot > 1e-6f) {
            outR2 = 1.0f - (ssRes / ssTot);
            outR2 = constrain(outR2, 0.0f, 1.0f);
        } else {
            outR2 = 0.0f;
        }
        return true;
    }

    // ============================================================
    // KALMAN 1D — Implémentation
    // ============================================================

    void updateKalman1D(float t_days, float capacityPct) {
        Kalman1DState& k = _state.kalman1d;

        // Initialisation au premier point
        if (!k.initialized) {
            if (_state.reg_count >= 2) {
                // Initialiser avec les 2 premiers points
                float x0 = _state.reg_time_days[0];
                float y0 = _state.reg_capacity_pct[0];
                float x1 = _state.reg_time_days[1];
                float y1 = _state.reg_capacity_pct[1];
                float dt = x1 - x0;
                if (fabsf(dt) > 0.1f) {
                    k.a = (y1 - y0) / dt;
                    k.b = y0 - k.a * x0;
                } else {
                    k.a = -0.01f; // Dégradation par défaut ~1%/100j
                    k.b = capacityPct;
                }
            } else {
                k.a = -0.01f;
                k.b = capacityPct;
            }
            k.P[0] = 1.0f;   // Var(a)
            k.P[1] = 0.0f;   // Cov(a,b)
            k.P[2] = 0.0f;   // Cov(b,a)
            k.P[3] = 10.0f;  // Var(b)
            k.initialized = 1;
            return;
        }

        // --- Prédiction ---
        // État : [a, b] — modèle process : a et b constants (dégradation lente)
        // x_k|k-1 = F * x_k-1|k-1 avec F = I
        // P_k|k-1 = F * P * F^T + Q
        float Q[4] = {
            BLP_KALMAN1D_Q_SLOPE,  0.0f,
            0.0f, BLP_KALMAN1D_Q_BIAS
        };
        k.P[0] += Q[0];
        k.P[1] += Q[1];
        k.P[2] += Q[2];
        k.P[3] += Q[3];

        // --- Mesure ---
        // z = H * x + v avec H = [t, 1]
        float z = capacityPct;
        float H[2] = {t_days, 1.0f};
        float R = BLP_KALMAN1D_R_MEASURE;

        // Innovation
        float y = z - (H[0] * k.a + H[1] * k.b);

        // S = H * P * H^T + R
        float S = H[0] * (k.P[0] * H[0] + k.P[1] * H[1]) +
                  H[1] * (k.P[2] * H[0] + k.P[3] * H[1]) + R;
        if (fabsf(S) < 1e-6f) S = 1e-6f;

        // Gain K = P * H^T / S
        float K[2];
        K[0] = (k.P[0] * H[0] + k.P[1] * H[1]) / S;
        K[1] = (k.P[2] * H[0] + k.P[3] * H[1]) / S;

        // Mise à jour état
        k.a += K[0] * y;
        k.b += K[1] * y;

        // Mise à jour covariance P = (I - K*H) * P
        float I_KH[4] = {
            1.0f - K[0] * H[0], -K[0] * H[1],
            -K[1] * H[0], 1.0f - K[1] * H[1]
        };
        float Pnew[4];
        Pnew[0] = I_KH[0] * k.P[0] + I_KH[1] * k.P[2];
        Pnew[1] = I_KH[0] * k.P[1] + I_KH[1] * k.P[3];
        Pnew[2] = I_KH[2] * k.P[0] + I_KH[3] * k.P[2];
        Pnew[3] = I_KH[2] * k.P[1] + I_KH[3] * k.P[3];
        k.P[0] = Pnew[0]; k.P[1] = Pnew[1];
        k.P[2] = Pnew[2]; k.P[3] = Pnew[3];
    }
};

#endif // BATTERY_LIFE_PREDICTOR_H
