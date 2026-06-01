/**
 * @file MAX17048_Gauge.h
 * @brief Libreria per la gestione del Fuel Gauge MAX17048 tramite bus I2C.
 * * Questa libreria fornisce un'interfaccia a oggetti per interagire con l'IC 
 * Maxim MAX17048, un misuratore di stato di carica (SOC) per batterie Li-ion.
 * Include funzionalità per la lettura di tensione, SOC, tasso di variazione, 
 * oltre alla gestione avanzata degli allarmi hardware e del power management.
 * * @author Emanuele Negrino
 * @date 2026
 * @note Sviluppato per progetto di tesi.
 */

#ifndef MAX17048_GAUGE_H
#define MAX17048_GAUGE_H

#include <Arduino.h>
#include <Wire.h>

/** @defgroup MAX17048_Registers Registri Ufficiali MAX17048
 * Mappatura degli indirizzi di memoria del chip MAX17048.
 * @{
 */
#define MAX1704X_VCELL_REG    0x02 ///< Registro della tensione della cella (ADC)
#define MAX1704X_SOC_REG      0x04 ///< Registro dello stato di carica (SOC)
#define MAX1704X_MODE_REG     0x06 ///< Registro di configurazione della modalità (Sleep, QuickStart)
#define MAX1704X_VERSION_REG  0x08 ///< Registro della versione dell'IC
#define MAX1704X_HIBRT_REG    0x0A ///< Registro delle soglie di ibernazione
#define MAX1704X_CONFIG_REG   0x0C ///< Registro di configurazione generale e allarmi
#define MAX1704X_VALERT_REG   0x14 ///< Registro delle soglie di allarme tensione (Min/Max)
#define MAX1704X_CRATE_REG    0x16 ///< Registro del tasso di variazione di carica (%/hr)
#define MAX1704X_VRESET_REG   0x18 ///< MSB = Soglia VRESET, LSB = CHIP ID
#define MAX1704X_STATUS_REG   0x1A ///< MSB = Flag di stato e allarmi
#define MAX1704X_CMD_REG      0xFE ///< Registro per l'invio di comandi speciali (es. Reset)
/** @} */

/** @defgroup MAX17048_Alerts Flag di Alert
 * Maschere di bit per il registro STATUS (0x1A) per l'identificazione degli allarmi.
 * @{
 */
#define MAX1704X_ALERTFLAG_SOC_CHANGE      0x20 ///< (SC) Variazione SOC dell'1%
#define MAX1704X_ALERTFLAG_SOC_LOW         0x10 ///< (HD) Stato di carica sotto la soglia impostata
#define MAX1704X_ALERTFLAG_VOLTAGE_RESET   0x08 ///< (VR) Avvenuto reset per caduta di tensione
#define MAX1704X_ALERTFLAG_VOLTAGE_LOW     0x04 ///< (VL) Tensione sotto la soglia minima
#define MAX1704X_ALERTFLAG_VOLTAGE_HIGH    0x02 ///< (VH) Tensione sopra la soglia massima
#define MAX1704X_ALERTFLAG_RESET_INDICATOR 0x01 ///< (RI) Indicatore di accensione/riavvio
/** @} */

/**
 * @class MAX17048_Gauge
 * @brief Classe principale per il controllo del sensore MAX17048.
 * * Permette l'inizializzazione del sensore sul bus I2C, la lettura dei dati 
 * telemetrici della batteria e la configurazione degli interrupt hardware.
 */
class MAX17048_Gauge {
public:
    /**
     * @brief Costruttore di default.
     */
    MAX17048_Gauge();

    /**
     * @brief Distruttore.
     */
    ~MAX17048_Gauge();

    // --- Inizializzazione e Stato Hardware ---
    
    /**
     * @brief Inizializza il sensore e il bus I2C.
     * @param wirePort Riferimento all'oggetto TwoWire (di default usa Wire).
     * @return true se il sensore risponde correttamente ed è inizializzato, false altrimenti.
     */
    bool begin(TwoWire &wirePort = Wire);
    
    /**
     * @brief Verifica se il sensore è connesso e pronto.
     * @return true se l'IC risponde col codice di versione corretto.
     */
    bool isDeviceReady();
    
    /**
     * @brief Legge la versione hardware del chip.
     * @return Valore a 16-bit del registro VERSION.
     */
    uint16_t getICversion();
    
    /**
     * @brief Legge l'identificativo univoco (ID) del chip.
     * @return Valore a 8-bit rappresentante il Chip ID.
     */
    uint8_t getChipID();
    
    /**
     * @brief Esegue un riavvio software (Soft Reset) del MAX17048.
     * @return true se il reset è andato a buon fine.
     */
    bool reset();
    
    /**
     * @brief Forza il ricalcolo istantaneo del SOC (QuickStart).
     * @warning Da usare con cautela, solo se la batteria è in stato di riposo (no carichi attivi).
     */
    void quickStart();

    // --- Letture Principali ---

    /**
     * @brief Legge la tensione attuale della batteria.
     * @return Tensione in Volt (V). Ritorna NAN se il dispositivo non è pronto.
     */
    float getVoltage();       
    
    /**
     * @brief Legge lo stato di carica (SOC) della batteria.
     * @return Percentuale di carica (0.0% - 100.0%).
     */
    float getSOC();           
    
    /**
     * @brief Legge il tasso di carica o scarica (Charge Rate).
     * @return Variazione percentuale all'ora (%/hr). Valori positivi indicano carica, negativi scarica.
     */
    float getChangeRate();    
    
    /**
     * @brief Determina se la batteria è in ricarica tramite isteresi sul Charge Rate.
     * @return true se il sistema sta attivamente caricando la batteria, false altrimenti.
     */
    bool isCharging();        

    // --- Gestione Alert ---

    /**
     * @brief Imposta la soglia percentuale per l'allarme di batteria scarica (HD).
     * @param percent Valore percentuale (tra 1 e 32).
     */
    void setLowSOCThreshold(uint8_t percent);
    
    /**
     * @brief Imposta le soglie di allarme per la tensione.
     * @param minv Tensione minima in Volt (sotto la quale scatta l'allarme VL).
     * @param maxv Tensione massima in Volt (sopra la quale scatta l'allarme VH).
     */
    void setAlertVoltages(float minv, float maxv);
    
    /**
     * @brief Legge le soglie di allarme per la tensione attualmente impostate.
     * @param minv Variabile per memorizzare la tensione minima letta.
     * @param maxv Variabile per memorizzare la tensione massima letta.
     */
    void getAlertVoltages(float &minv, float &maxv);
    
    /**
     * @brief Controlla lo stato del pin ALRT (Alert) a livello hardware.
     * @return true se c'è almeno un allarme attivo e il pin ALRT è asserito.
     */
    bool isActiveAlert();
    
    /**
     * @brief Legge il blocco dei flag di stato per identificare l'allarme.
     * @return Byte contenente i flag attivi (comparabili con le costanti MAX1704X_ALERTFLAG_*).
     */
    uint8_t getAlertStatus();
    
    /**
     * @brief Cancella un singolo flag di allarme dal registro di stato.
     * @param flag Il flag da cancellare (es. MAX1704X_ALERTFLAG_VOLTAGE_HIGH).
     * @return true se la cancellazione è andata a buon fine.
     */
    bool clearAlertFlag(uint8_t flag);
    
    /**
     * @brief Resetta tutti gli allarmi attivi e spegne il segnale sul pin ALRT.
     */
    void clearAllAlerts();

    // --- VRESET (Rilevamento Rimozione Batteria) ---

    /**
     * @brief Imposta la tensione di Reset (VRESET). 
     * Se la tensione scende sotto questa soglia, l'IC si resetta.
     * @param reset_v Tensione di reset in Volt.
     */
    void setResetVoltage(float reset_v);
    
    /**
     * @brief Legge l'attuale tensione di Reset (VRESET) impostata.
     * @return Tensione di reset in Volt.
     */
    float getResetVoltage();

    // --- Power Management (Ibernazione e Sleep) ---

    /**
     * @brief Imposta la soglia di attività per uscire dall'ibernazione.
     * @param actthresh Variazione di tensione in Volt.
     */
    void setActivityThreshold(float actthresh);
    float getActivityThreshold();
    
    /**
     * @brief Imposta la soglia del Charge Rate per entrare in ibernazione.
     * @param hibthresh Tasso in %/hr. Se il rate è minore per 6 minuti, l'IC va in ibernazione.
     */
    void setHibernationThreshold(float hibthresh);
    float getHibernationThreshold();

    /** @brief Forza l'ingresso in modalità Ibernazione. */
    void hibernate();
    
    /** @brief Forza l'uscita dalla modalità Ibernazione. */
    void wake();
    
    /** @brief Verifica se l'IC è attualmente in modalità Ibernazione. */
    bool isHibernating();
    
    /** @brief Abilita o disabilita la possibilità di entrare in Sleep Mode profondo. */
    void enableSleep(bool en);
    
    /** @brief Manda l'IC in Sleep Mode profondo (consumo < 1µA) o lo risveglia. */
    void sleep(bool s);

    // --- Compensazione Temperatura ---

    /**
     * @brief Imposta il parametro RCOMP per la compensazione termica del modello della batteria.
     * @param rcomp Valore a 8-bit (default 0x97).
     */
    void setRCOMP(uint8_t rcomp);

private:
    TwoWire* _i2cPort;                ///< Puntatore all'oggetto I2C
    const uint8_t _address = 0x36;    ///< Indirizzo I2C predefinito del MAX17048

    bool _isChargingState = false;    ///< Memoria di stato per l'isteresi della ricarica

    /**
     * @brief Scrive una word a 16-bit in un registro I2C.
     * @param reg Indirizzo del registro.
     * @param value Valore a 16-bit da scrivere.
     * @return 
     */
    bool writeRegister(uint8_t reg, uint16_t value);
    
    /**
     * @brief Legge una word a 16-bit da un registro I2C.
     * @param reg Indirizzo del registro.
     * @return Valore a 16-bit letto (0xFFFF in caso di errore).
     */
    uint16_t readRegister(uint8_t reg);
};

#endif