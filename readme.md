**Vysoké učení technické v Brně, Fakulta elektrotechniky a komunikačních technologií, Ústav radioelektroniky, 2025/2026**

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

Všetky získané dáta budú pravidelne zapisované do **textového súboru na SD kartu**. Časová pečiatka bude zabezpečená z **RTC modulu DS3231**

---
## 🎞️ Video ukážka:
[![video ukazka](https://github.com/user-attachments/assets/af561fbd-a82c-43eb-bab1-a1720ffcca0a)](https://github.com/user-attachments/assets/6c9e4a1d-1f91-4246-b8f2-ae7182b49fb9)

---
## 🧩 Hardvérové moduly pre prototyp

| Modul | Účel |
|-------|------|
| Arduino UNO | Riadiaca jednotka |
| ZS-042 (DS3231) | RTC modul – presný čas |
| BME280 | Meranie teploty, tlaku, vlhkosti a nadmorskej výšky |
| SGP41 | Meranie kvality ovzdušia (VOx, NOx) |
| Logic Level Shifter | Prevádza úrovne 5V ↔ 3.3V |
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

## Schéma zapojení

<img width="800" height="600" alt="SCHEMA snad konec" src="https://github.com/user-attachments/assets/b16237f4-0a3c-4c17-926f-e1eb805e7947" />

---

## Zapojení na nepájivém poli 

<img width="800" height="600" alt="obrazek zapojení" src="https://github.com/user-attachments/assets/7be98dc0-b6b8-4e7b-96d2-a09c3544b182" />

---

## Vývojový diagram

<img width="403" height="850" alt="image" src="https://github.com/user-attachments/assets/21adc598-9861-4a7a-bf14-a9e40a67a432" />


---

## Grafy naměřených hodnot jednotlivých veličin

<img width="800" height="600" alt="Figure_2" src="https://github.com/user-attachments/assets/100f4ad4-459d-4017-b843-762d03479d37" />



---
-
## 🛠️ Funkčný zámer kódu

### 1️⃣ Inicializácia RDS modulu  


### 2️⃣ Kontrola prítomnosti zariadení na I2C zbernici  
- Vyhľadanie adries pripojených modulov (RTC, senzory)

### 2.1 Zistenie prítomnosti SD karty  
- Overenie inicializácie SD karty

### 2.2 Kontrola súboru pre zápis  
- Ak karta existuje, detekuje sa prítomnosť súboru (napr. `datalog.txt`)  
- Ak súbor neexistuje, vytvorí sa nový s hlavičkami dát

### 3️⃣ Periodické meranie dát  
- **Každých 5 sekúnd** sa načítajú údaje zo všetkých senzorov

### 4️⃣ Spracovanie a zápis dát  
- Dáta sa spracujú, doplnia o timestamp a uložia do súboru na SD karte vo formáte:

---
## 📂Soubory📂

<pre>
DE2-SD-CARD-TESTING/
├── .gitignore
├── platformio.ini                      # Konfigurace PlatformIO
├── include/                            # Hlavičkové soubory
│   ├── README
│   └── timer.h                         # Časovače, systémová timebase
│
├── lib/                                # Knihovny
│   ├── FAT32/                          # Knihovna pro práci s FAT32
│   │   ├── FAT32.c
│   │   └── FAT32.h
│   ├── SPI/                            # SPI rutiny pro AVR
│   │   ├── SPI_routines.c
│   │   └── SPI_routines.h
│   ├── bme280/                         # Driver senzoru BME280
│   │   ├── bme280.c
│   │   ├── bme280.h
│   │   └── bme280_defs.h
│   ├── gas_index_algorithm/            # Algoritmus indexu kvality vzduchu
│   │   ├── GasIndexAlgorithm.c
│   │   └── GasIndexAlgorithm.h
│   ├── sgp41/                          # Driver senzoru SGP41
│   │   ├── SGP41.c
│   │   ├── SGP41.h
│   │   ├── sensirion_arch_config.h
│   │   ├── sensirion_common.c
│   │   ├── sensirion_common.h
│   │   ├── sensirion_shdlc.c
│   │   ├── sensirion_shdlc.h
│   │   ├── sensirion_uart.c
│   │   ├── sensirion_uart.h
│   │   ├── SensirionI2CSgp41.c
│   │   └── SensirionI2CSgp41.h
│   ├── twi/                            # I2C/TWI driver pro AVR
│   │   ├── twi.c
│   │   └── twi.h
│   ├── uart/                           # UART driver
│   │   ├── uart.c
│   │   ├── uart.h
│   │   └── uart_compat.h
│   └── README
├── src/                                # Zdrojové soubory aplikace
│   ├── bme.c
│   ├── main.c                          # Hlavní program
│   ├── sgp41.c
│   └── lab2-gpio.code-workspace
└── test/                               # Testovací soubory
    └── README
</pre>
---

## Kde můžeme tyto zařízení využít

Chytré kanceláře nebo domácnosti můžeme sledovat kvalitu vzduchu v různých místnostech díky SPG41 můžeme řídit automatické spuštění čističky vzduchu nebo ventilace na základě hodnot VOC/NOx které model poskytuje. Dále je možná detekce úniku chemikálii nebo plynu. Monitoring vlhkosti nebo teploty pomocí BME280 může být použit jak v domácnostech, tak ve školách nebo továrnách pro poskytnutí co nejlepšího pracovního prostředí a skladových podmínek pro citlivé výrobky. Dalším využitím je detekce změn nadmořské výšky.

---
