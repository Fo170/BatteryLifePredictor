# Documentation complète — Norme IEC 61427 : Méthodes d'essai pour accumulateurs de stockage d'énergie

**Version :** 1.0  
**Date :** 2026-08-08  
**Auteur :** FOURNET Olivier  
**Licence :** GPL-3.0  
**Référentiel :** https://github.com/Fo170/LLM_Pilots

---

## Table des matières

1. [Introduction et champ d'application](#1-introduction-et-champ-dapplication)
2. [Structure de la norme IEC 61427](#2-structure-de-la-norme-iec-61427)
3. [Protocoles d'essai détaillés](#3-protocoles-dessai-détaillés)
4. [Méthodes de comptage de cycles](#4-méthodes-de-comptage-de-cycles)
5. [Spécificités par technologie de batterie](#5-spécificités-par-technologie-de-batterie)
6. [Gestion du DOD excédentaire](#6-gestion-du-dod-excédentaire)
7. [Formules et modèles mathématiques](#7-formules-et-modèles-mathématiques)
8. [Exemples pratiques de dimensionnement](#8-exemples-pratiques-de-dimensionnement)
9. [Annexes](#9-annexes)

---

## 1. Introduction et champ d'application

### 1.1 Objet de la norme

La norme internationale **IEC 61427** définit les méthodes d'essai générales pour les accumulateurs (batteries rechargeables) utilisés dans les systèmes de stockage d'énergie renouvelable, notamment :

- Systèmes photovoltaïques (PV) autonomes
- Systèmes PV connectés au réseau (grid-tied)
- Systèmes hybrides PV + stockage
- Systèmes éoliens avec stockage

### 1.2 Objectifs des essais

Les essais IEC 61427 visent à :

1. **Vérifier la durabilité** des accumulateurs sous des conditions de cyclage représentatives du service réel
2. **Quantifier la dégradation** de la capacité au fil des cycles
3. **Établir une correspondance** entre cycles d'essai laboratoire et années de service sur le terrain
4. **Permettre la comparaison** objective entre différentes technologies et fabricants

### 1.3 Technologies couvertes

La norme s'applique aux technologies suivantes :

| Technologie | Désignation IEC | Partie applicable |
|-------------|-----------------|-------------------|
| Plomb-acide (VRLA, AGM, Gel, Inondée) | IEC 61427-1 | Partie 1 (autonome) |
| Lithium-ion (LiFePO₄, NMC, NCA, LTO) | IEC 61427-1 / 61427-2 | Les deux parties |
| Nickel-Métal Hydrure (NiMH) | IEC 61427-1 | Partie 1 |
| Nickel-Cadmium (NiCd) | IEC 61427-1 | Partie 1 |
| Sodium-Soufre (NaS) | IEC 61427-2 | Partie 2 |
| Redox Flow (Vanadium, Zinc-Brome) | IEC 61427-2 | Partie 2 |

---

## 2. Structure de la norme IEC 61427

### 2.1 IEC 61427-1 : Systèmes photovoltaïques autonomes

**Titre complet :** *Secondary cells and batteries containing alkaline or other non-acid electrolytes — Secondary sealed cells and batteries for portable applications — Part 1: Nickel-cadmium*

> **Note :** En pratique, la partie 1 couvre tous les accumulateurs pour systèmes PV autonomes, y compris le plomb et le lithium.

**Champ d'application :**
- Installations PV sans connexion au réseau électrique
- Besoin de stockage pour la nuit et les jours sans soleil
- Cycles journaliers réguliers avec périodes de sous-charge (PSOC)

### 2.2 IEC 61427-2 : Systèmes connectés au réseau

**Titre complet :** *Secondary cells and batteries for renewable energy storage — General requirements and methods of test — Part 2: Grid connected applications*

**Champ d'application :**
- Installations PV ou éoliennes avec stockage et injection réseau
- Services auxiliaires (régulation de fréquence, lissage de puissance)
- Cycles plus rapides et moins profonds que les systèmes autonomes
- Possibilité de recharge depuis le réseau

### 2.3 Différences clés entre les deux parties

| Critère | IEC 61427-1 (Autonome) | IEC 61427-2 (Réseau) |
|---------|------------------------|----------------------|
| Profil de charge | Macro-cycles lents (1 cycle = 1 an) | Micro-cycles rapides (fréquence) |
| DOD typique | 50–80 % | 20–50 % |
| Taux de charge | 0,1C–0,5C | 0,5C–2C |
| Température d'essai | 25 °C (Li) / 40 °C (Pb) | 25 °C |
| Durée d'essai | 1–3 ans équivalent | 5–10 ans équivalent |
| Critère de fin | Capacité < 80 % nominale | Capacité < 80 % nominale |

---

## 3. Protocoles d'essai détaillés

### 3.1 Préconditionnement

Avant tout essai de cyclage, les batteries doivent subir un préconditionnement :

1. **Formation** : 3 à 5 cycles complets (charge C/10 → décharge C/10) pour stabiliser les électrodes
2. **Mesure de capacité initiale (C₀)** : 3 mesures consécutives, moyenne = référence
3. **Stabilisation thermique** : 16 h à température d'essai avant le premier cycle

### 3.2 Protocole de cyclage IEC 61427-1 (macro-cycles)

Le protocole standard se compose de **macro-cycles** (aussi appelés "cycles de vie" ou "cycles IEC").

#### 3.2.1 Composition d'un macro-cycle

Un macro-cycle = **Phase A + Phase B**

**Phase A — Cycles peu profonds à bas SOC :**
- 50 cycles de charge/décharge
- Charge : C/10 jusqu'à SOC 100 %
- Décharge : C/10 jusqu'à SOC 60 % (DOD = 40 %)
- Pause : 1 h entre charge et décharge
- Température : selon spécification fabricant

**Phase B — Cycles peu profonds à haut SOC :**
- 100 cycles de charge/décharge
- Charge : C/10 jusqu'à SOC 100 %
- Décharge : C/10 jusqu'à SOC 80 % (DOD = 20 %)
- Pause : 1 h entre charge et décharge

> **1 macro-cycle = 150 micro-cycles ≈ 1 an de service PV réel**

#### 3.2.2 Mesures intermédiaires

Tous les **5 macro-cycles** (soit tous les 750 micro-cycles) :
- Mesure de capacité à C/10
- Mesure de résistance interne (impédance à 1 kHz)
- Contrôle visuel (gonflement, fuite, corrosion)

#### 3.2.3 Critère d'arrêt

L'essai s'arrête lorsque :
- La capacité mesurée est inférieure à **80 % de C₀**
- OU après 20 macro-cycles (limite protocolaire)
- OU en cas de défaillance de sécurité (fuite, surchauffe, court-circuit)

### 3.3 Protocole de cyclage IEC 61427-2 (micro-cycles)

Pour les applications connectées au réseau, le protocole utilise des **micro-cycles** plus rapides :

| Paramètre | Valeur standard |
|-----------|-----------------|
| Taux de charge | 1C |
| Taux de décharge | 1C |
| DOD | 80 % |
| Pause | 30 min |
| Température | 25 °C ± 2 °C |
| Nombre de cycles entre mesures | 100 |
| Critère de fin | Capacité < 80 % C₀ |

### 3.4 Conditions environnementales

| Paramètre | Plomb-acide | Lithium-ion | NiMH |
|-----------|-------------|-------------|------|
| Température d'essai | 40 °C ± 3 °C | 25 °C ± 2 °C | 25 °C ± 2 °C |
| Humidité relative | 45–75 % | 45–75 % | 45–75 % |
| Ventilation | Forcée si > 35 °C | Naturelle | Naturelle |

> **Pourquoi 40 °C pour le plomb ?** La corrosion des grilles positives et la sulfatation sont fortement accélérées par la chaleur. Tester à 40 °C simule les conditions réelles d'une installation PV en zone chaude et accélère le vieillissement de manière représentative.

---

## 4. Méthodes de comptage de cycles

### 4.1 Méthode EFC (Equivalent Full Cycles)

#### 4.1.1 Principe

La méthode EFC convertit des cycles partiels en cycles pleins équivalents par simple proportionnalité :

$$\text{EFC} = \sum_{i=1}^{n} \frac{\text{DOD}_i}{100\%}$$

Où :
- $\text{DOD}_i$ = profondeur de décharge du cycle $i$ (en %)
- $n$ = nombre total de cycles partiels

#### 4.1.2 Exemple de calcul

| Jour | DOD | Équivalent |
|------|-----|------------|
| 1 | 80 % | 0,80 |
| 2 | 50 % | 0,50 |
| 3 | 30 % | 0,30 |
| 4 | 100 % | 1,00 |
| **Total** | | **2,60 EFC** |

#### 4.1.3 Limites de la méthode EFC

| Problème | Description |
|----------|-------------|
| Linéarité incorrecte | La relation DOD-cycles n'est pas linéaire |
| Pas d'effet mémoire | Ignore l'historique et la séquence |
| Pas de correction température | Même DOD à 10 °C ou 40 °C = même EFC |
| Pas de PSOC | Ignore le temps passé en charge partielle |

> **Verdict :** La méthode EFC est acceptable pour les **garanties commerciales simplifiées** mais insuffisante pour la **prédiction de vie résiduelle**.

### 4.2 Méthode Rainflow (comptage des gouttes de pluie)

#### 4.2.1 Principe algorithmique

L'algorithme Rainflow décompose un profil de charge/décharge irrégulier en cycles élémentaires avec leur amplitude réelle.

**Étapes :**

1. **Détection des extrema** : identifier tous les pics et creux du signal SOC(t)
2. **Initialisation** : faire "couler" une goutte depuis chaque pic vers le bas
3. **Règles d'arrêt** :
   - La goutte s'arrête si elle rencontre une paroi plus abrupte
   - La goutte s'arrête si elle croise le trajet d'une goutte issue d'un pic supérieur
4. **Comptage** : chaque trajet complet (pic → creux → pic) = 1 cycle ; trajet interrompu = 0,5 cycle

#### 4.2.2 Pseudo-code

```
FONCTION Rainflow(signal_SOC):
    extrema = Extraire_extrema(signal_SOC)
    cycles = []
    POUR chaque extremum e DANS extrema:
        SI e est un pic:
            goutte = Simuler_chute(e, extrema)
            SI goutte atteint un creux c ET remonte vers un pic p:
                amplitude = |p - c|
                cycles.AJOUTER({amplitude, moyenne = (p+c)/2})
            SINON:
                cycles.AJOUTER({amplitude = |e - creux|, demi-cycle})
    RETOURNER cycles
```

#### 4.2.3 Avantages

- Capture les **micro-cycles** de régulation de fréquence
- Distribue correctement les amplitudes dans un histogramme
- Standard de l'industrie automobile (véhicules électriques)

### 4.3 Règle de Miner (cumul des dommages)

#### 4.3.1 Formulation

$$D = \sum_{j} \frac{n_j}{N_j}$$

Où :
- $D$ = dommage cumulé (sans unité)
- $n_j$ = nombre de cycles réellement effectués au niveau de contrainte $j$
- $N_j$ = nombre de cycles à la rupture pour le niveau de contrainte $j$

**Critère de fin de vie :**
- $D < 1$ : batterie en vie utile
- $D \geq 1$ : fin de vie théorique atteinte

#### 4.3.2 Extension avec température

Pour le plomb, la règle de Miner doit être enrichie :

$$D = \sum_{j} \sum_{k} \frac{n_{j,k}}{N_j(T_k)}$$

Où $T_k$ est la température de la classe $k$ et $N_j(T_k)$ est la durée de vie à cette température.

#### 4.3.3 Limitations connues

| Limitation | Description | Solution |
|------------|-------------|----------|
| Séquence indifférente | L'ordre des cycles n'affecte pas D | Utiliser un modèle de mémoire |
| Pas d'effet PSOC | Ignore le temps en charge partielle | Ajouter un terme de sulfatation |
| Seuil unique | D = 1 est arbitraire | Calibrer avec données réelles |

### 4.4 Méthode de l'énergie de travail ultime (WUE)

Une extension récente de la règle de Miner intègre l'énergie :

$$D = \sum_{j} \frac{E_j}{E_{\text{ult},j}}$$

Où $E_j$ est l'énergie délivrée dans les cycles de classe $j$ et $E_{\text{ult},j}$ est l'énergie totale que la batterie peut délivrer à ce niveau de contrainte avant rupture.

---

## 5. Spécificités par technologie de batterie

### 5.1 Plomb-acide (Pb)

#### 5.1.1 Types de plomb-acide

| Type | Électrolyte | Égalisation | Maintenance | Durée de vie @ 50 % DOD |
|------|-------------|-------------|-------------|------------------------|
| Inondée (Flooded) | Liquide libre | Oui, mensuelle | Haute (niveau d'eau) | 1 500–2 500 cycles |
| VRLA AGM | Absorbé dans fibre de verre | Non | Faible | 1 000–1 500 cycles |
| VRLA Gel | Gelifié | Non | Faible | 1 200–1 800 cycles |
| Tubulaire OPzS | Inondée, plaques tubulaires | Oui | Haute | 2 500–4 000 cycles |
| Tubulaire OPzV | Gel, plaques tubulaires | Non | Faible | 2 000–3 500 cycles |

#### 5.1.2 Courbe de vie DOD

$$N_{\text{Pb}}(\text{DOD}) = N_0 \cdot e^{-k_{\text{Pb}} \cdot \text{DOD}}$$

Avec :
- $N_0$ ≈ 6 000 cycles (théorique à DOD → 0)
- $k_{\text{Pb}}$ ≈ 0,022 (coefficient de sensibilité)

| DOD | Cycles estimés (deep cycle) |
|-----|----------------------------|
| 20 % | ~5 500 |
| 30 % | ~4 500 |
| 50 % | ~3 000 |
| 60 % | ~2 000 |
| 80 % | ~1 500 |
| 100 % | ~500–800 |

#### 5.1.3 Phénomènes de dégradation

**a) Sulfatation**
- Formation de cristaux de PbSO₄ de grande taille lors des PSOC prolongés
- Irréversible sans charge d'égalisation
- Accélérée par la température et la sous-charge

**b) Corrosion des grilles positives**
- Oxydation du plomb métallique en PbO₂
- Réaction : Pb + 2H₂O → PbO₂ + 4H⁺ + 4e⁻
- Doublée tous les 15 °C au-dessus de 25 °C

**c) Stratification de l'électrolyte**
- Concentration d'acide sulfurique en bas de la cellule
- Réduit la capacité effective de 10–30 %
- Corrigeable par charge d'égalisation ou agitation

**d) Perte d'eau (inondées uniquement)**
- Électrolyse de l'eau en H₂ et O₂ lors de la surcharge
- Nécessite un remplissage régulier en eau déminéralisée

#### 5.1.4 Règle de Van't Hoff pour le plomb

$$\frac{N(T_2)}{N(T_1)} = 2^{-(T_2 - T_1)/15}$$

Où $T$ est en °C. Chaque augmentation de 15 °C au-dessus de 25 °C divise la durée de vie par 2.

#### 5.1.5 Rendements

| Type de rendement | Valeur typique |
|-------------------|----------------|
| Rendement coulombique | ~85 % |
| Rendement énergétique (Wh) | ~70 % |
| Rendement à C/10 | ~75 % |
| Rendement à C/5 | ~65 % |

### 5.2 Lithium Fer Phosphate (LiFePO₄ / LFP)

#### 5.2.1 Caractéristiques

| Paramètre | Valeur |
|-----------|--------|
| Tension nominale | 3,2 V/cellule |
| Tension de charge | 3,6–3,65 V/cellule |
| Tension de décharge min | 2,5 V/cellule |
| Densité énergétique | 90–160 Wh/kg |
| Densité de puissance | 200–400 W/kg |
| Rendement coulombique | > 99 % |
| Rendement énergétique | 90–95 % |

#### 5.2.2 Courbe de vie DOD

$$N_{\text{LFP}}(\text{DOD}) = N_0 \cdot e^{-k_{\text{LFP}} \cdot \text{DOD}}$$

Avec :
- $N_0$ ≈ 8 000–10 000 cycles
- $k_{\text{LFP}}$ ≈ 0,012

| DOD | Cycles estimés |
|-----|---------------|
| 20 % | ~8 000 |
| 50 % | ~6 000 |
| 80 % | ~4 000–6 000 |
| 100 % | ~3 000–4 000 |

#### 5.2.3 Phénomènes de dégradation

**a) Perte de lithium (LLI — Loss of Lithium Inventory)**
- Réaction parasite : Li⁺ + e⁻ + électrolyte → produits de décomposition
- Réduit la quantité de lithium mobile
- Accélérée par la température et la tension de charge élevée

**b) Perte de sites actifs (LAM — Loss of Active Material)**
- Dégradation mécanique des particules d'électrode
- Fissuration et pulvérisation des grains
- Liée aux contraintes mécaniques lors des cycles

**c) Dendrites**
- Formation de dépôts métalliques de lithium sur l'anode
- Peuvent percer le séparateur et causer un court-circuit interne
- Minimisées par le BMS (limitation du courant de charge en fin de charge)

#### 5.2.4 Facteur de température

$$\frac{N(T_2)}{N(T_1)} = \left(\frac{T_1 + 273}{T_2 + 273}ight)^{n} \cdot e^{\frac{E_a}{R}\left(\frac{1}{T_2} - \frac{1}{T_1}ight)}$$

Avec $E_a$ ≈ 50–70 kJ/mol (énergie d'activation) et $n$ ≈ 1,5.

> Le lithium est **beaucoup moins sensible** à la température que le plomb. Il reste stable jusqu'à 45–50 °C.

### 5.3 Lithium NMC (Nickel-Manganèse-Cobalt)

#### 5.3.1 Caractéristiques

| Paramètre | NMC 111 | NMC 532 | NMC 622 | NMC 811 |
|-----------|---------|---------|---------|---------|
| Tension nominale | 3,7 V | 3,7 V | 3,7 V | 3,7 V |
| Densité énergétique | 150 Wh/kg | 180 Wh/kg | 200 Wh/kg | 250 Wh/kg |
| Cycles @ 80 % DOD | 1 000 | 1 500 | 2 000 | 2 500 |
| Stabilité thermique | Bonne | Bonne | Moyenne | À surveiller |
| Coût | Élevé | Moyen | Moyen | Bas (cobalt réduit) |

#### 5.3.2 Dégradation spécifique

**Dégradation des cathodes riches en nickel (NMC 811) :**
- Instabilité du Ni⁴⁺ en surface
- Libération d'oxygène à haute température (> 60 °C)
- Transition de phase H2 → H3 dans la structure cristalline

**Migration du manganese :**
- Dissolution du Mn²⁺ dans l'électrolyte
- Dépôt sur l'anode → augmentation de l'impédance

### 5.4 Lithium NCA (Nickel-Cobalt-Aluminium)

| Paramètre | Valeur |
|-----------|--------|
| Tension nominale | 3,6 V/cellule |
| Densité énergétique | 200–260 Wh/kg |
| Cycles @ 80 % DOD | 1 000–1 500 |
| Utilisation principale | Véhicules électriques (Tesla) |

**Dégradation :**
- Instabilité structurale de la cathode à haute tension (> 4,3 V)
- Génération de gaz (CO₂, CO) par décomposition de l'électrolyte
- Croissance de la couche SEI (Solid Electrolyte Interphase)

### 5.5 Lithium Titanate (LTO)

| Paramètre | Valeur |
|-----------|--------|
| Tension nominale | 2,4 V/cellule |
| Densité énergétique | 70–80 Wh/kg |
| Cycles @ 80 % DOD | **15 000–25 000** |
| Taux de charge max | **10C** |
| Température opération | -30 °C à +55 °C |

**Avantages :**
- Aucune formation de dendrites
- Pas de couche SEI (pas de graphite)
- Charge ultra-rapide possible

**Inconvénients :**
- Coût élevé
- Densité énergétique faible
- Tension faible (nécessite plus de cellules en série)

### 5.6 Nickel-Métal Hydrure (NiMH)

| Paramètre | Valeur |
|-----------|--------|
| Tension nominale | 1,2 V/cellule |
| Densité énergétique | 60–120 Wh/kg |
| Cycles @ 80 % DOD | 500–1 000 |
| Rendement énergétique | ~70 % |

**Dégradation :**
- Corrosion de l'électrode positive (NiOOH)
- Ségrégation de l'alliage de l'électrode négative (MH)
- Autodécharge élevée (10–20 %/mois)
- Effet mémoire (nécessite des cycles de conditionnement)

### 5.7 Nickel-Cadmium (NiCd)

| Paramètre | Valeur |
|-----------|--------|
| Tension nominale | 1,2 V/cellule |
| Densité énergétique | 40–60 Wh/kg |
| Cycles @ 80 % DOD | 1 000–2 000 |
| Température | -40 °C à +50 °C |

**Dégradation :**
- Effet mémoire prononcé
- Cristallisation du cadmium (dendrites)
- Toxicité du cadmium (restriction RoHS)

### 5.8 Sodium-Soufre (NaS)

| Paramètre | Valeur |
|-----------|--------|
| Tension nominale | 2,0 V/cellule |
| Température opération | **300–350 °C** (fondu) |
| Densité énergétique | 150–240 Wh/kg |
| Cycles | 4 500 |
| Utilisation | Stockage stationnaire MW-scale |

**Dégradation :**
- Corrosion du contenant en alumine par le soufre fondu
- Fuite du sodium (réaction violente avec l'eau)
- Consommation énergétique pour le maintien à température

### 5.9 Redox Flow (Vanadium)

| Paramètre | Valeur |
|-----------|--------|
| Tension nominale | 1,2–1,4 V/cellule |
| Densité énergétique | 20–40 Wh/kg |
| Cycles | **> 15 000** (durée de vie du séparateur) |
| DOD | 100 % sans dégradation |
| Scalabilité | Puissance et énergie découplées |

**Dégradation :**
- Contamination croisée des électrolytes (V²⁺/V³⁺ et V⁴⁺/V⁵⁺)
- Précipitation du V₂O₅ à haute température
- Dégradation du séparateur (membrane ionique)

---

## 6. Gestion du DOD excédentaire

### 6.1 Définition

Le **DOD excédentaire** est la fraction de décharge au-delà d'un seuil de référence défini par le fabricant ou l'opérateur.

$$\text{DOD}_{\text{exc}} = \max(0, \text{DOD}_{\text{réel}} - \text{DOD}_{\text{ref}})$$

### 6.2 Méthodes de conservation

#### 6.2.1 Comptage par fenêtre glissante

```
FENÊTRE = 30 jours glissants
POUR chaque cycle DANS fenêtre:
    SI DOD > DOD_ref:
        DOD_exc_cumulé += DOD - DOD_ref
    SINON:
        DOD_exc_cumulé = 0  (réinitialisation)

SI DOD_exc_cumulé > SEUIL_ALERTE:
    Déclencher maintenance préventive
```

#### 6.2.2 Comptage avec amortissement exponentiel

$$\text{DOD}_{\text{exc,pond}}(t) = \sum_{i} \text{DOD}_{\text{exc},i} \cdot e^{-(t - t_i)/\tau}$$

Avec $\tau$ = constante de temps de mémoire (ex. 90 jours pour le plomb, 30 jours pour le lithium).

#### 6.2.3 Intégration dans le BMS

Le BMS avancé conserve un registre de DOD excédentaire avec :
- Horodatage de chaque événement
- Température ambiante
- Taux de charge/décharge (C-rate)
- État de charge initial (SOC₀)

### 6.3 Impact sur la garantie

| Fabricant | Seuil DOD garanti | Pénalité DOD excédentaire |
|-----------|-------------------|----------------------------|
| A (Plomb) | 50 % | Perte de garantie si > 80 % récurrent |
| B (LFP) | 80 % | Réduction de 20 % de la garantie si > 95 % |
| C (NMC) | 80 % | Réduction de 10 %/an si > 90 % |

---

## 7. Formules et modèles mathématiques

### 7.1 Modèle de dégradation empirique (Arrhenius-like)

$$\frac{dQ}{dN} = -A \cdot \left(\frac{\text{DOD}}{100}ight)^{m} \cdot e^{-E_a/(R \cdot T)}$$

Où :
- $Q$ = capacité relative (%)
- $N$ = nombre de cycles
- $A$ = constante de dégradation (technologie-dépendante)
- $m$ = exposant de sensibilité au DOD
- $E_a$ = énergie d'activation (J/mol)
- $R$ = constante des gaz parfaits (8,314 J/mol·K)
- $T$ = température absolue (K)

### 7.2 Valeurs des paramètres par technologie

| Technologie | A | m | Ea (kJ/mol) |
|-------------|---|---|-------------|
| Plomb-acide | 5×10⁻⁵ | 2,2 | 55 |
| LiFePO₄ | 2×10⁻⁵ | 1,5 | 50 |
| NMC 532 | 3×10⁻⁵ | 1,8 | 55 |
| NCA | 4×10⁻⁵ | 2,0 | 60 |
| LTO | 5×10⁻⁶ | 1,2 | 45 |
| NiMH | 8×10⁻⁵ | 2,0 | 50 |

### 7.3 Modèle de sulfatation (plomb uniquement)

$$\frac{dS}{dt} = k_s \cdot (1 - \text{SOC})^{\alpha} \cdot e^{-E_s/(R \cdot T)}$$

Où :
- $S$ = fraction de surface sulfatée
- $k_s$ = constante de sulfatation
- $\alpha$ = exposant de sensibilité au SOC (typ. 1,5)
- $E_s$ = énergie d'activation de la sulfatation (≈ 45 kJ/mol)

### 7.4 Modèle de croissance SEI (lithium)

$$\frac{dR_{\text{SEI}}}{dt} = \frac{k_{\text{SEI}}}{R_{\text{SEI}}} \cdot e^{-E_{\text{SEI}}/(R \cdot T)}$$

Où $R_{\text{SEI}}$ est la résistance de la couche SEI et $k_{\text{SEI}}$ ≈ 10⁻¹⁵ m²/s.

### 7.5 Formule de capacité résiduelle

$$Q_{\text{res}}(N, T, \text{DOD}) = Q_0 \cdot \left(1 - \frac{N}{N_{\text{ref}}(T, \text{DOD})}ight)^{\beta}$$

Avec $\beta$ ≈ 0,8–1,2 (facteur de forme de la courbe de dégradation).

---

## 8. Exemples pratiques de dimensionnement

### 8.1 Exemple 1 : Installation PV autonome au plomb

**Données :**
- Consommation journalière : 5 kWh
- Jours d'autonomie : 3
- Tension système : 48 V
- DOD max : 50 %
- Température ambiante : 35 °C

**Calcul :**

1. Énergie stockée nécessaire :
   $$E_{\text{stock}} = \frac{5 \times 3}{0,50} = 30 \text{ kWh}$$

2. Capacité en Ah :
   $$C = \frac{30{,}000}{48} = 625 \text{ Ah}$$

3. Correction température (Van't Hoff) :
   $$N(35°C) = N(25°C) \times 2^{-(35-25)/15} = N(25°C) \times 0,63$$

4. Cycles attendus @ 50 % DOD et 35 °C :
   $$N = 3{,}000 \times 0,63 = 1{,}890 \text{ cycles}$$

5. Durée de vie estimée :
   $$\text{Durée} = \frac{1{,}890}{365} = 5{,}2 \text{ ans}$$

### 8.2 Exemple 2 : Installation PV autonome au lithium LFP

**Données identiques, DOD max : 80 %**

1. Énergie stockée :
   $$E_{\text{stock}} = \frac{5 \times 3}{0,80} = 18{,}75 \text{ kWh}$$

2. Capacité en Ah :
   $$C = \frac{18{,}750}{48} = 391 \text{ Ah}$$

3. Cycles @ 80 % DOD (peu de correction température à 35 °C) :
   $$N \approx 4{,}000 \text{ cycles}$$

4. Durée de vie :
   $$\text{Durée} = \frac{4{,}000}{365} = 11 \text{ ans}$$

### 8.3 Exemple 3 : Comptage Miner sur profil mixte

**Profil d'utilisation (plomb, 25 °C) :**

| Phase | Cycles | DOD | N(DOD) |
|-------|--------|-----|--------|
| Hiver | 180 | 70 % | 1 800 |
| Été | 180 | 40 % | 4 500 |
| Total annuel | 360 | — | — |

**Calcul Miner :**

$$D = \frac{180}{1{,}800} + \frac{180}{4{,}500} = 0{,}10 + 0{,}04 = 0{,}14 \text{ /an}$$

**Durée de vie estimée :**

$$\text{Durée} = \frac{1{,}0}{0{,}14} = 7{,}1 \text{ ans}$$

---

## 9. Annexes

### Annexe A : Glossaire

| Terme | Définition |
|-------|------------|
| **DOD** | Depth of Discharge — Profondeur de décharge (%) |
| **SOC** | State of Charge — État de charge (%) |
| **SOH** | State of Health — État de santé (%) |
| **EFC** | Equivalent Full Cycle — Cycle plein équivalent |
| **PSOC** | Partial State of Charge — État de charge partiel |
| **C-rate** | Taux de charge/décharge (1C = capacité nominale en 1h) |
| **SEI** | Solid Electrolyte Interphase — Couche interphase |
| **VRLA** | Valve Regulated Lead Acid — Plomb à soupape |
| **AGM** | Absorbent Glass Mat — Fibre de verre absorbante |
| **OPzS** | Opaque Positive tubular, Stationary — Plaque tubulaire stationnaire |

### Annexe B : Conversion des unités

| De | Vers | Formule |
|----|------|---------|
| Wh | Ah @ V | Ah = Wh / V |
| kWh | MJ | MJ = kWh × 3,6 |
| °C | K | K = °C + 273,15 |
| C-rate | A | A = C-rate × C_nom (Ah) |

### Annexe C : Références normatives

| Norme | Titre |
|-------|-------|
| IEC 61427-1:2013 | Accumulateurs pour applications PV autonomes |
| IEC 61427-2:2015 | Accumulateurs pour applications connectées au réseau |
| IEC 62619:2017 | Exigences de sécurité pour batteries lithium industrielles |
| IEC 62620:2014 | Batteries lithium pour applications industrielles |
| IEC 60896-21 | Batteries plomb-acide stationnaires — Méthodes d'essai |
| UL 1973 | Batteries pour applications stationnaires et véhicules légers |

### Annexe D : Checklist de validation d'un essai IEC 61427

- [ ] Préconditionnement complet (3–5 cycles)
- [ ] Mesure de C₀ documentée
- [ ] Température stabilisée 16 h avant essai
- [ ] Taux de charge/décharge conforme
- [ ] Pauses respectées entre cycles
- [ ] Mesures de capacité tous les 5 macro-cycles
- [ ] Enregistrement de la température en continu
- [ ] Contrôle visuel à chaque mesure intermédiaire
- [ ] Documentation de la fin de vie (cause + capacité finale)
- [ ] Rapport d'essai avec courbes de dégradation

---

## Licence

Ce document est distribué sous licence **GPL-3.0**.

**Auteur :** FOURNET Olivier <olivier.fournet@free.fr>  
**Dépôt :** https://github.com/Fo170/LLM_Pilots

---

*Document généré le 8 août 2026. Les valeurs numériques sont des ordres de grandeur basés sur les données publiques des fabricants et les publications scientifiques. Toujours se référer aux spécifications exactes du fabricant pour un dimensionnement précis.*
