# 📘 README – Datalogger pre meranie kvality okolia

## 🎯 Zámer projektu
Cieľom projektu je vytvoriť **datalogger**, ktorý bude zbierať údaje o prostredí z rôznych senzorov. Zaznamenávať sa budú tieto veličiny:

- Kvalita ovzdušia: **VOX, NOX**
- **Teplota**
- **Tlak**
- **Vlhkosť**
- **Nadmorská výška**
- **Časová pečiatka (timestamp)**

Všetky získané dáta budú pravidelne zapisované do **textového súboru na SD kartu**. Časová pečiatka bude zabezpečená z **RTC modulu DS3231**, ktorý bude **synchronizovaný pomocou rádiového RDS signálu**.

---

## 🧩 Hardvérové moduly pre prototyp

| Modul | Účel |
|-------|------|
| Arduino UNO | Riadiaca jednotka |
| ZS-042 (DS3231) | RTC modul – presný čas |
| BME280 | Meranie teploty, tlaku, vlhkosti a nadmorskej výšky |
| SGP41 | Meranie kvality ovzdušia (VOx, NOx) |
| HW-332 (SI4703) | RDS rádio modul pre získanie času |
| Logic Level Shifter | Prevádza úrovne 5V ↔ 3.3V (kompatibilita SI4703) |
| Pololu sdc02 | SD karta – ukladanie dát |

---

## 🛠️ Funkčný zámer kódu

### 1️⃣ Inicializácia RDS modulu  
- Prepnutie modulu **SI4703** do režimu **2-wire I2C komunikácie**

### 2️⃣ Kontrola prítomnosti zariadení na I2C zbernici  
- Vyhľadanie adries pripojených modulov (RTC, senzory, rádio)

### 2.1 Zistenie prítomnosti SD karty  
- Overenie inicializácie SD karty

### 2.2 Kontrola súboru pre zápis  
- Ak karta existuje, detekuje sa prítomnosť súboru (napr. `datalog.txt`)  
- Ak súbor neexistuje, vytvorí sa nový s hlavičkami dát

### 3️⃣ Kontrola času v RTC vs RDS  
- Porovnanie aktuálneho času z RTC a času získaného cez RDS

### 3.1 Aktualizácia RTC  
- Ak je čas z RDS presnejší, zapíše sa do RTC modulu DS3231

### 4️⃣ Periodické meranie dát  
- **Každých 10 sekúnd** sa načítajú údaje zo všetkých senzorov

### Stav projektu
Funkcne RDS cez SI4703.h lib
Funkcne BMS280 - Teplota Vlhost Tlak

###
TODO:
SD card read write
SGP41 gas sensor support

Intergracia celku
Poster

### 5️⃣ Spracovanie a zápis dát  
- Dáta sa spracujú, doplnia o timestamp a uložia do súboru na SD karte vo formáte:


