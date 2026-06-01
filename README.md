# Power Management Unit (PMU) per Sistemi Embedded

Questo repository contiene tutti i file di progettazione hardware, le simulazioni e le librerie software per una **Power Management Unit (PMU) integrata**, sviluppata come progetto di Tesi di Laurea Triennale in Ingegneria Elettronica presso l'Università degli Studi di Genova.

Il modulo (dimensioni 50x50 mm) è progettato per colmare il divario tecnologico tra le tensioni variabili delle batterie ai Polimeri di Litio (Li-Po) e le tensioni fisse richieste dai carichi digitali moderni (come microcontrollori e SBC), garantendo continuità di servizio e sicurezza operativa.

---

## Caratteristiche Principali (Features)
* **Power Path Management:** Commutazione automatica tra sorgente esterna (5V) e batteria senza interruzioni di servizio (funzione UPS DC).
* **Conversione DC-DC:** Uscita stabile a 5V tramite convertitore Buck-Boost (fino a 1.3A continui) operante in modo trasparente indipendentemente dallo stato di carica della cella.
* **Ricarica Li-Po 1S:** Algoritmo CC-CV integrato con completamento carica a 4.2V.
* **Protezione Hardware (BMS):** Modulo di protezione contro sovrascarica (Cut-off < 2.4V), sovraccarica (> 4.3V) e cortocircuiti.
* **Fuel Gauging I2C:** Stima accurata dello Stato di Carica (SOC) e alert hardware tramite algoritmo ModelGauge (MAX17048), senza uso di resistenze di shunt.

---

## Architettura Hardware
Il progetto è stato interamente sviluppato utilizzando **KiCad 9.0**. Il sistema si basa sui seguenti circuiti integrati:
* **TPS630701:** Convertitore DC-DC Buck-Boost (Texas Instruments).
* **MCP73831:** Controller lineare di ricarica Li-Po/Li-Ion (Microchip).
* **MAX17048:** Sensore Fuel Gauge I2C (Maxim Integrated).
* **DW01A + FS8205:** IC di protezione batteria e doppio MOSFET N-Channel.
* **DMP1045U-7:** MOSFET P-Channel per la commutazione del Power Path.

---

## Struttura della Repository

```text
├── Hardware/
|   ├── Kicad_files/        # File di progetto (Schematici, Layout PCB)
│   ├── Production/         # File pronti per la produzione industriale
│   ├── Assembly/           # File BOM (Bill of Materials) e CPL per assemblaggio SMT
│   └── 3D_Models/          # Rendering 3D della scheda top/bottom
├── Simulations/            # File di simulazione SPICE (Proteus 8 Professional)
├── Firmware/               # Libreria C++ custom per MAX17048 e sketch di test (Arduino)
├── Datasheets/             # PDF dei datasheet originali dei componenti utilizzati
├── Docs/                   # Immagini, grafici di test termici/ripple e documentazione extra
└── README.md
