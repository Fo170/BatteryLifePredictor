#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Simulation Monte Carlo — Prédiction de vie résiduelle batteries
Auteur : FOURNET Olivier
Licence : GPL-3.0
"""

import numpy as np
import matplotlib.pyplot as plt
from dataclasses import dataclass
from typing import Tuple, List
import json

# ============================================================
# PARAMÈTRES DES MODÈLES
# ============================================================

@dataclass
class BatteryParams:
    name: str
    N0: float           # Cycles théoriques à DOD → 0
    k_dod: float        # Coefficient de sensibilité au DOD
    Ea: float           # Énergie d'activation (kJ/mol)
    eta_energy: float   # Rendement énergétique
    cost_per_kwh: float # €/kWh installé
    maintenance_annual: float  # €/an
    dod_max_recommended: float  # DOD max recommandé

# Paramètres calibrés sur données fabricants
PARAMS_PB = BatteryParams(
    name="Plomb-acide (deep cycle)",
    N0=6000,
    k_dod=0.022,
    Ea=55.0,
    eta_energy=0.70,
    cost_per_kwh=150.0,
    maintenance_annual=100.0,
    dod_max_recommended=0.50
)

PARAMS_LFP = BatteryParams(
    name="LiFePO₄",
    N0=8000,
    k_dod=0.012,
    Ea=50.0,
    eta_energy=0.93,
    cost_per_kwh=350.0,
    maintenance_annual=0.0,
    dod_max_recommended=0.80
)

PARAMS_NMC = BatteryParams(
    name="NMC 532",
    N0=5000,
    k_dod=0.018,
    Ea=55.0,
    eta_energy=0.92,
    cost_per_kwh=400.0,
    maintenance_annual=0.0,
    dod_max_recommended=0.80
)

PARAMS_LTO = BatteryParams(
    name="LTO (Titanate)",
    N0=20000,
    k_dod=0.008,
    Ea=45.0,
    eta_energy=0.88,
    cost_per_kwh=800.0,
    maintenance_annual=0.0,
    dod_max_recommended=0.90
)

R = 8.314  # J/(mol·K)


# ============================================================
# MODÈLES DE DÉGRADATION
# ============================================================

def cycles_lifetime_dod(params: BatteryParams, dod: float) -> float:
    """Cycles à la rupture pour un DOD constant (loi exponentielle)."""
    return params.N0 * np.exp(-params.k_dod * dod)


def temp_correction_arrhenius(params: BatteryParams, T_celsius: float,
                               T_ref: float = 25.0) -> float:
    """Facteur de correction température (modèle Arrhenius)."""
    T = T_celsius + 273.15
    T_ref_K = T_ref + 273.15
    return np.exp(-params.Ea * 1000 / R * (1.0 / T - 1.0 / T_ref_K))


def sulfatation_factor(psoc_hours: float, eq_interval_days: float) -> float:
    """
    Facteur de pénalité/bonus lié à la sulfatation (plomb uniquement).
    psoc_hours : heures/jour passées sous 80% SOC
    eq_interval_days : intervalle entre égalisations
    """
    psoc_penalty = 1.0 + (psoc_hours / 24.0) * 0.30
    eq_bonus = max(0.5, 1.0 - (30.0 / eq_interval_days) * 0.30)
    return psoc_penalty * eq_bonus


def predict_lifetime_pb(dod: float, T: float, psoc_hours: float,
                        eq_interval: float) -> float:
    """Durée de vie en années — plomb-acide (intègre sulfatation)."""
    N = cycles_lifetime_dod(PARAMS_PB, dod)
    N *= temp_correction_arrhenius(PARAMS_PB, T)
    N /= sulfatation_factor(psoc_hours, eq_interval)
    return N / 365.0


def predict_lifetime_li(params: BatteryParams, dod: float, T: float) -> float:
    """Durée de vie en années — lithium (pas de sulfatation)."""
    N = cycles_lifetime_dod(params, dod)
    N *= temp_correction_arrhenius(params, T)
    return N / 365.0


# ============================================================
# SIMULATION MONTE CARLO
# ============================================================

def monte_carlo_lifetime(n_simulations: int = 10000,
                          dod_mean: float = 50.0,
                          dod_std: float = 15.0,
                          T_mean: float = 25.0,
                          T_std: float = 5.0,
                          psoc_mean: float = 8.0,
                          psoc_std: float = 4.0,
                          eq_mean: float = 30.0,
                          eq_std: float = 10.0) -> dict:
    """
    Simulation Monte Carlo de la durée de vie.
    Distributions : DOD ~ N(μ,σ), T ~ N(μ,σ), PSOC ~ N(μ,σ), EQ ~ N(μ,σ)
    """
    np.random.seed(42)

    # Tirages aléatoires
    dod_samples = np.clip(np.random.normal(dod_mean, dod_std, n_simulations), 10, 100)
    T_samples = np.clip(np.random.normal(T_mean, T_std, n_simulations), 5, 55)
    psoc_samples = np.clip(np.random.normal(psoc_mean, psoc_std, n_simulations), 0, 24)
    eq_samples = np.clip(np.random.normal(eq_mean, eq_std, n_simulations), 7, 90)

    results = {
        "plomb": [],
        "lfp": [],
        "nmc": [],
        "lto": []
    }

    for i in range(n_simulations):
        dod = dod_samples[i]
        T = T_samples[i]
        psoc = psoc_samples[i]
        eq = eq_samples[i]

        results["plomb"].append(predict_lifetime_pb(dod, T, psoc, eq))
        results["lfp"].append(predict_lifetime_li(PARAMS_LFP, dod, T))
        results["nmc"].append(predict_lifetime_li(PARAMS_NMC, dod, T))
        results["lto"].append(predict_lifetime_li(PARAMS_LTO, dod, T))

    for key in results:
        results[key] = np.array(results[key])

    return results


def print_statistics(results: dict):
    """Affiche les statistiques descriptives."""
    print("=" * 70)
    print("RÉSULTATS SIMULATION MONTE CARLO (n=10 000)")
    print("=" * 70)
    print(f"{'Technologie':<20} {'Moyenne':>10} {'Médiane':>10} {'P5':>8} {'P95':>8} {'σ':>8}")
    print("-" * 70)
    for tech, data in results.items():
        print(f"{tech.upper():<20} {np.mean(data):>9.1f} {np.median(data):>9.1f} "
              f"{np.percentile(data, 5):>7.1f} {np.percentile(data, 95):>7.1f} "
              f"{np.std(data):>7.2f}")
    print("-" * 70)
    print("(valeurs en années)")


def plot_monte_carlo(results: dict, output_path: str = "monte_carlo_lifetime.png"):
    """Génère l'histogramme des distributions de durée de vie."""
    fig, axes = plt.subplots(2, 2, figsize=(12, 10))
    fig.suptitle("Simulation Monte Carlo — Durée de vie des batteries (années)",
                 fontsize=14, fontweight='bold')

    colors = {"plomb": "#E74C3C", "lfp": "#3498DB", "nmc": "#9B59B6", "lto": "#2ECC71"}
    titles = {"plomb": "Plomb-acide", "lfp": "LiFePO₄", "nmc": "NMC 532", "lto": "LTO"}

    for ax, (tech, data) in zip(axes.flat, results.items()):
        ax.hist(data, bins=80, color=colors[tech], alpha=0.7, edgecolor='white', linewidth=0.5)
        ax.axvline(np.median(data), color='black', linestyle='--', linewidth=1.5,
                   label=f"Médiane = {np.median(data):.1f} ans")
        ax.axvline(np.percentile(data, 5), color='gray', linestyle=':', linewidth=1,
                   label=f"P5 = {np.percentile(data, 5):.1f} ans")
        ax.axvline(np.percentile(data, 95), color='gray', linestyle=':', linewidth=1,
                   label=f"P95 = {np.percentile(data, 95):.1f} ans")
        ax.set_title(titles[tech], fontsize=12)
        ax.set_xlabel("Durée de vie (années)")
        ax.set_ylabel("Fréquence")
        ax.legend(fontsize=8)
        ax.set_xlim(0, max(25, np.percentile(data, 99)))

    plt.tight_layout()
    plt.savefig(output_path, dpi=150, bbox_inches='tight')
    plt.close()
    print(f"✓ Graphique sauvegardé : {output_path}")


# ============================================================
# COURBES DE SENSIBILITÉ
# ============================================================

def plot_sensitivity_curves(output_path: str = "sensitivity_curves.png"):
    """Génère les courbes de sensibilité DOD et température."""
    fig, axes = plt.subplots(1, 2, figsize=(14, 6))
    fig.suptitle("Courbes de sensibilité — Durée de vie vs paramètres opérationnels",
                 fontsize=14, fontweight='bold')

    # --- Sous-plot 1 : DOD ---
    ax1 = axes[0]
    dod_range = np.linspace(10, 100, 200)
    T_ref = 25.0

    # Plomb (sans et avec sulfatation)
    pb_base = [predict_lifetime_pb(d, T_ref, 0, 7) for d in dod_range]
    pb_psoc = [predict_lifetime_pb(d, T_ref, 12, 30) for d in dod_range]
    pb_bad = [predict_lifetime_pb(d, T_ref, 20, 90) for d in dod_range]

    ax1.plot(dod_range, pb_base, color='#E74C3C', linewidth=2.5, label='Plomb (PSOC=0, EQ=7j)')
    ax1.plot(dod_range, pb_psoc, color='#E74C3C', linewidth=2, linestyle='--',
             label='Plomb (PSOC=12h, EQ=30j)')
    ax1.plot(dod_range, pb_bad, color='#E74C3C', linewidth=1.5, linestyle=':',
             label='Plomb (PSOC=20h, EQ=90j)')

    # Lithium
    lfp = [predict_lifetime_li(PARAMS_LFP, d, T_ref) for d in dod_range]
    nmc = [predict_lifetime_li(PARAMS_NMC, d, T_ref) for d in dod_range]
    lto = [predict_lifetime_li(PARAMS_LTO, d, T_ref) for d in dod_range]

    ax1.plot(dod_range, lfp, color='#3498DB', linewidth=2.5, label='LiFePO₄')
    ax1.plot(dod_range, nmc, color='#9B59B6', linewidth=2, label='NMC 532')
    ax1.plot(dod_range, lto, color='#2ECC71', linewidth=2, label='LTO')

    ax1.axhline(y=15, color='gray', linestyle='-.', linewidth=1, alpha=0.5, label='Cible 15 ans')
    ax1.set_xlabel("DOD moyen (%)", fontsize=11)
    ax1.set_ylabel("Durée de vie (années)", fontsize=11)
    ax1.set_title("Sensibilité au DOD (T = 25°C)", fontsize=12)
    ax1.legend(fontsize=8, loc='upper right')
    ax1.set_xlim(10, 100)
    ax1.set_ylim(0, 30)
    ax1.grid(True, alpha=0.3)

    # --- Sous-plot 2 : Température ---
    ax2 = axes[1]
    T_range = np.linspace(5, 50, 200)
    dod_ref = 50.0

    pb_T = [predict_lifetime_pb(dod_ref, t, 8, 30) for t in T_range]
    lfp_T = [predict_lifetime_li(PARAMS_LFP, dod_ref, t) for t in T_range]
    nmc_T = [predict_lifetime_li(PARAMS_NMC, dod_ref, t) for t in T_range]
    lto_T = [predict_lifetime_li(PARAMS_LTO, dod_ref, t) for t in T_range]

    ax2.plot(T_range, pb_T, color='#E74C3C', linewidth=2.5, label='Plomb (DOD=50%)')
    ax2.plot(T_range, lfp_T, color='#3498DB', linewidth=2.5, label='LiFePO₄ (DOD=50%)')
    ax2.plot(T_range, nmc_T, color='#9B59B6', linewidth=2, label='NMC 532 (DOD=50%)')
    ax2.plot(T_range, lto_T, color='#2ECC71', linewidth=2, label='LTO (DOD=50%)')

    ax2.axvline(x=25, color='gray', linestyle='--', linewidth=1, alpha=0.5)
    ax2.axvline(x=40, color='orange', linestyle='--', linewidth=1, alpha=0.5)
    ax2.text(26, 28, "25°C (ref)", fontsize=9, color='gray')
    ax2.text(41, 28, "40°C (Pb test)", fontsize=9, color='orange')

    ax2.set_xlabel("Température ambiante (°C)", fontsize=11)
    ax2.set_ylabel("Durée de vie (années)", fontsize=11)
    ax2.set_title("Sensibilité à la température (DOD = 50%)", fontsize=12)
    ax2.legend(fontsize=9, loc='upper right')
    ax2.set_xlim(5, 50)
    ax2.set_ylim(0, 30)
    ax2.grid(True, alpha=0.3)

    plt.tight_layout()
    plt.savefig(output_path, dpi=150, bbox_inches='tight')
    plt.close()
    print(f"✓ Graphique sauvegardé : {output_path}")


def plot_tco_sensitivity(output_path: str = "tco_sensitivity.png"):
    """Courbes de sensibilité TCO vs prix du lithium et durée de vie visée."""
    fig, axes = plt.subplots(1, 2, figsize=(14, 6))
    fig.suptitle("Analyse de sensibilité TCO — Plomb vs Lithium",
                 fontsize=14, fontweight='bold')

    # Paramètres fixes
    kwh_daily = 5.0
    days_autonomy = 3.0
    e_pb = (kwh_daily * days_autonomy) / 0.50
    e_li = (kwh_daily * days_autonomy) / 0.80

    # --- Sous-plot 1 : TCO vs prix lithium ---
    ax1 = axes[0]
    li_cost_range = np.linspace(100, 800, 100)
    pb_cost = 150.0
    pb_maint = 100.0
    pb_life = 5.5
    li_life = 16.0

    tco_pb = []
    tco_li = []
    for c_li in li_cost_range:
        inv_pb = e_pb * pb_cost
        inv_li = e_li * c_li
        rep_pb = (np.ceil(15 / pb_life) - 1) * inv_pb
        rep_li = (np.ceil(15 / li_life) - 1) * inv_li
        tco_pb.append(inv_pb + pb_maint * 15 + rep_pb)
        tco_li.append(inv_li + rep_li)

    ax1.plot(li_cost_range, tco_pb, color='#E74C3C', linewidth=2.5, label='Plomb (TCO 15 ans)')
    ax1.plot(li_cost_range, tco_li, color='#3498DB', linewidth=2.5, label='LiFePO₄ (TCO 15 ans)')

    # Seuil d'inversion
    diff = np.array(tco_pb) - np.array(tco_li)
    crossing = li_cost_range[np.argmin(np.abs(diff))]
    ax1.axvline(x=crossing, color='green', linestyle='--', linewidth=1.5)
    ax1.text(crossing + 10, max(tco_pb) * 0.9, f"Seuil d'inversion\n{crossing:.0f} €/kWh",
             fontsize=10, color='green')

    ax1.set_xlabel("Prix lithium (€/kWh)", fontsize=11)
    ax1.set_ylabel("TCO 15 ans (€)", fontsize=11)
    ax1.set_title("TCO vs prix du lithium", fontsize=12)
    ax1.legend(fontsize=10)
    ax1.grid(True, alpha=0.3)
    ax1.set_xlim(100, 800)

    # --- Sous-plot 2 : TCO vs durée de vie visée ---
    ax2 = axes[1]
    years_range = np.arange(5, 26, 1)
    li_cost = 350.0

    tco_pb_y = []
    tco_li_y = []
    for y in years_range:
        inv_pb = e_pb * pb_cost
        inv_li = e_li * li_cost
        rep_pb = (np.ceil(y / pb_life) - 1) * inv_pb
        rep_li = (np.ceil(y / li_life) - 1) * inv_li
        tco_pb_y.append(inv_pb + pb_maint * y + rep_pb)
        tco_li_y.append(inv_li + rep_li)

    ax2.plot(years_range, tco_pb_y, color='#E74C3C', linewidth=2.5, label='Plomb')
    ax2.plot(years_range, tco_li_y, color='#3498DB', linewidth=2.5, label='LiFePO₄')

    # Zone de rentabilité
    diff_y = np.array(tco_pb_y) - np.array(tco_li_y)
    crossing_y = years_range[np.argmin(np.abs(diff_y))]
    ax2.axvline(x=crossing_y, color='green', linestyle='--', linewidth=1.5)
    ax2.text(crossing_y + 0.5, max(tco_pb_y) * 0.9, f"Break-even\n{crossing_y} ans",
             fontsize=10, color='green')
    ax2.axvspan(crossing_y, 25, alpha=0.1, color='green', label='Zone lithium avantageux')

    ax2.set_xlabel("Durée de vie visée (années)", fontsize=11)
    ax2.set_ylabel("TCO cumulé (€)", fontsize=11)
    ax2.set_title("TCO vs horizon temporel", fontsize=12)
    ax2.legend(fontsize=10)
    ax2.grid(True, alpha=0.3)
    ax2.set_xlim(5, 25)

    plt.tight_layout()
    plt.savefig(output_path, dpi=150, bbox_inches='tight')
    plt.close()
    print(f"✓ Graphique sauvegardé : {output_path}")


# ============================================================
# EXÉCUTION
# ============================================================

if __name__ == "__main__":
    print("=" * 70)
    print("SIMULATION BATTERIES — MONTE CARLO & SENSIBILITÉ")
    print("=" * 70)

    # 1. Monte Carlo
    print("\n[1] Lancement simulation Monte Carlo (n=10 000)...")
    results = monte_carlo_lifetime(n_simulations=10000)
    print_statistics(results)
    plot_monte_carlo(results)

    # 2. Courbes de sensibilité
    print("\n[2] Génération des courbes de sensibilité...")
    plot_sensitivity_curves()

    # 3. TCO
    print("\n[3] Analyse de sensibilité TCO...")
    plot_tco_sensitivity()

    print("\n" + "=" * 70)
    print("SIMULATION TERMINÉE")
    print("=" * 70)
