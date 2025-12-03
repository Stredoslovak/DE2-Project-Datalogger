 **Vysoké učení technické v Brně, Fakulta elektrotechniky a komunikačních technologií, Ústav radioelektroniky, 2025/2026**

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
| Logic Level Shifter | Prevádza úrovne 5V ↔ 3.3V (kompatibilita SI4703) |
| Pololu sdc02 | SD karta – ukladanie dát |

---
## Popis jednotlivých komponentů
-Arduino UNO

Arduino UNO slouží jako řídicí jednotka celého projektu. Tento mikrokontroler (ATmega328P) zajišťuje komunikaci se všemi senzory přes sběrnici I2C a stará se o ukládání naměřených dat na SD kartu.

<img width="1024" height="768" alt="image" src="https://github.com/user-attachments/assets/dffe9fd4-97cb-4da4-9068-b9b7f81f4ee3" />
datasheet- https://docs.arduino.cc/resources/datasheets/A000066-datasheet.pdf

---

-ZS-042 (DS3231)

Tento modul zajišťuje přesné časování celého systému. Komunikuje přes sběrnici I2C a umožňuje přidávat k naměřeným datům přesná časová razítka (datum a čas). Díky záložní baterii se čas nevynuluje ani při výpadku proudu nebo restartu Arduina.


<img width="800" height="600" alt="image" src="https://github.com/user-attachments/assets/989f3ca6-8efb-4fee-b2f6-5c820e1465d5" />


datasheet- https://www.analog.com/media/en/technical-documentation/data-sheets/ds3231.pdf

---

-BME280

Tento senzor slouží k měření teploty, vlhkosti a tlaku. Díky vysoké citlivosti tlakového senzoru dokáže s dobrou přesností vypočítat i aktuální nadmořskou výšku. Má nízkou spotřebu a snadnou komunikaci s I2C

<img width="990" height="990" alt="image" src="https://github.com/user-attachments/assets/f7b9bc40-25b1-4549-8f40-c33fb1505c3b" />


datasheet- https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bme280-ds002.pdf

---

-SGP41

SGP41 je pokročilý senzor kvality vzduchu. Je navržen speciálně pro detekci dvou hlavních typů znečištění v interiérech: těkavých organických látek (VOC) a oxidů dusíku (NOx).

<img width="1024" height="768" alt="image" src="https://github.com/user-attachments/assets/3434e50e-890d-4abc-9461-612b514b67aa" />


datasheet- https://sensirion.com/media/documents/5FE8673C/61E96F50/Sensirion_Gas_Sensors_Datasheet_SGP41.pdf

---

-Logic Level Shifter

Logic Level Shifter je modul pro bezpečnou komunikaci mezi součástkami s různým napětím. Protože Arduino Uno pracuje s 5V logiku, zatímco některé senzory vyžadují 3,3 V, tento převodník slouží jako most. Zajišťuje, aby 5V signál z Arduina nezničil citlivější 3,3V součástky.

<img width="645" height="338" alt="image" src="https://github.com/user-attachments/assets/686055a1-fd39-4cec-8fec-7dcfb30c42e3" />

---

-SD karta

V našem zařízení funguje tento modul jako datalogger (zapisovač dat). Všechny hodnoty naměřené senzory se v pravidelných intervalech ukládají do textového souboru  přímo na kartu. 

<img width="600" height="480" alt="image" src="https://github.com/user-attachments/assets/172bfc9b-6083-4c04-88ee-1f411062a510" />

zdroj- https://www.pololu.com/product/2587

---
-
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

## Stav projektu
Funkcne RDS cez SI4703.h lib
Funkcne BMS280 - Teplota Vlhost Tlak

### TO DO list:
SD card read write
SGP41 gas sensor support
- [x] SGP41 gas sensor support
- [ ] SD card read write
- [ ] Intergracia celku
- [ ] Poster



### 5️⃣ Spracovanie a zápis dát  
- Dáta sa spracujú, doplnia o timestamp a uložia do súboru na SD karte vo formáte:


