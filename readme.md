[DE2_Project_subory.txt](https://github.com/user-attachments/files/23917001/DE2_Project_subory.txt)[DE2_Project_subory.txt](https://github.com/user-attachments/files/23916997/DE2_Project_subory.txt) **Vysoké učení technické v Brně, Fakulta elektrotechniky a komunikačních technologií, Ústav radioelektroniky, 2025/2026**

# 📘 README – Datalogger pre meranie kvality okolia

## 👥 Členové týmu

 - Dominik Gazda
 - Martin Dzuruš
 - Tomáš Hedbávný
 - Daniel Gomba

## 🎯 Zámer projektu

Kvalita vzduchu, teplota či vlhkost mají vliv na naše zdraví, náladu a produktivitu s rostoucími klimatickými změnami není od věci zaznamenávat přesné údaje o našem okolí v reálném čase. Náš projekt umožňuje automatické sledování environmentálních parametrů a jejich ukládání, což nám dává informace o kvalitě našeho vnitřního a vnějšího prostředí díky kterým jsme informováni o určitých podmínkách na které se můžeme připravit nebo je řešit.

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

<img width="800" height="600" alt="image" src="https://github.com/user-attachments/assets/dffe9fd4-97cb-4da4-9068-b9b7f81f4ee3" />
datasheet- https://docs.arduino.cc/resources/datasheets/A000066-datasheet.pdf

---

-ZS-042 (DS3231)

Tento modul zajišťuje přesné časování celého systému. Komunikuje přes sběrnici I2C a umožňuje přidávat k naměřeným datům přesná časová razítka (datum a čas). Díky záložní baterii se čas nevynuluje ani při výpadku proudu nebo restartu Arduina.


<img width="800" height="600" alt="image" src="https://github.com/user-attachments/assets/989f3ca6-8efb-4fee-b2f6-5c820e1465d5" />


datasheet- https://www.analog.com/media/en/technical-documentation/data-sheets/ds3231.pdf

---

-BME280

Tento senzor slouží k měření teploty, vlhkosti a tlaku. Díky vysoké citlivosti tlakového senzoru dokáže s dobrou přesností vypočítat i aktuální nadmořskou výšku. Má nízkou spotřebu a snadnou komunikaci s I2C

<img width="800" height="600" alt="image" src="https://github.com/user-attachments/assets/f7b9bc40-25b1-4549-8f40-c33fb1505c3b" />


datasheet- https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bme280-ds002.pdf

---

-SGP41

SGP41 je pokročilý senzor kvality vzduchu. Je navržen speciálně pro detekci dvou hlavních typů znečištění v interiérech: těkavých organických látek (VOC) a oxidů dusíku (NOx).

<img width="800" height="600" alt="image" src="https://github.com/user-attachments/assets/3434e50e-890d-4abc-9461-612b514b67aa" />


datasheet- https://sensirion.com/media/documents/5FE8673C/61E96F50/Sensirion_Gas_Sensors_Datasheet_SGP41.pdf

---

-Logic Level Shifter

Logic Level Shifter je modul pro bezpečnou komunikaci mezi součástkami s různým napětím. Protože Arduino Uno pracuje s 5V logiku, zatímco některé senzory vyžadují 3,3 V, tento převodník slouží jako most. Zajišťuje, aby 5V signál z Arduina nezničil citlivější 3,3V součástky.

<img width="800" height="600" alt="image" src="https://github.com/user-attachments/assets/686055a1-fd39-4cec-8fec-7dcfb30c42e3" />

---

-SD karta

V našem zařízení funguje tento modul jako datalogger (zapisovač dat). Všechny hodnoty naměřené senzory se v pravidelných intervalech ukládají do textového souboru  přímo na kartu. 

<img width="800" height="600" alt="image" src="https://github.com/user-attachments/assets/172bfc9b-6083-4c04-88ee-1f411062a510" />

zdroj- https://www.pololu.com/product/2587

---

-Schéma zapojení

<img width="800" height="600" alt="SCHEMA snad konec" src="https://github.com/user-attachments/assets/b16237f4-0a3c-4c17-926f-e1eb805e7947" />

---
 
-Vývojový diagram

<img width="403" height="850" alt="image" src="https://github.com/user-attachments/assets/21adc598-9861-4a7a-bf14-a9e40a67a432" />


---

-Kde můžeme tyto zařízení využít

Chytré kanceláře nebo domácnosti můžeme sledovat kvalitu vzduchu v různých místnostech díky SPG41 můžeme řídit automatické spuštění čističky vzduchu nebo ventilace na základě hodnot VOC/NOx které model poskytuje. Dále je možná detekce úniku chemikálii nebo plynu. Monitoring vlhkosti nebo teploty pomocí BME280 může být použit jak v domácnostech, tak ve školách nebo továrnách pro poskytnutí co nejlepšího pracovního prostředí a skladových podmínek pro citlivé výrobky. Dalším využitím je detekce změn nadmořské výšky.

---

-Grafy naměřených hodnot jednotlivých veličin

<img width="1400" height="900" alt="untitled" src="https://github.com/user-attachments/assets/4f0b95c8-6e6a-44d6-b418-a4f44e7a331c" />

---

-📂Soubory📂

/..................................................Kořenový adresář projektu
├── .vscode/.......................................
├── include/.......................................
│   │   └── timer.h................................Prototypy časovače, systémová timebase
├── lib/...........................................Knihovny
│   ├── oled/......................................Ovladač OLED displeje SH1106
│   │   ├── oled.c.................................
│   │   ├── oled.h.................................
│   │   └── font.h.................................
│   ├── uart/......................................UART ovladač (Peter Fleury)
│   │   ├── uart.c.................................
│   │   └── uart.h.................................
│   ├── twi/.......................................I2C/TWI master ovladač pro AVR
│   │   ├── twi.c..................................
│   │   └── twi.h..................................
│   ├── rpeak/.....................................Detektor R-špiček v EKG signálu
│   │   ├── rpeak.c................................
│   │   └── rpeak.h................................
│   │── button/....................................Tlačítko Start/Stop (debounce, FSM)
│   │   ├── button.c...............................
│   │   └── button.h...............................
│   ├── ecg_loader/................................Modul pro načítání offline EKG datasetů
│   │   ├── ecg_loader.c...........................
│   │   ├── ecg_loader.h...........................
│   │   ├── wfdb_parser.c..........................
│   │   └── wfdb_parser.h..........................
├── src/........................................... 
│   └── main.c.....................................
├── ecg_datasets/..................................Testovací EKG signály (PTB-XL, low-res) 
│   ├──14030_lr.hea................................Hlavička signálu – parametry kanálu
│   ├──14030_lr.dat................................16bit ECG data (WFDB formát)
│   ├──14016_lr.hea................................
│   ├──14016_lr.dat................................
│   ├──14001_lr.hea................................
│   ├──14001_lr.dat................................
│   ├──14006_lr.hea................................
│   └──14006_lr.dat................................
├── platformio.ini.................................Konfigurace PlatformIO (board: Uno, AVR-GCC)
└── build..........................................
```



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


### 5️⃣ Spracovanie a zápis dát  
- Dáta sa spracujú, doplnia o timestamp a uložia do súboru na SD karte vo formáte:


