/**
 * @file main.cpp
 * @brief Script principale di test e validazione per la libreria MAX17048_Gauge.
 * * Questo firmware inizializza il sensore sul bus I2C, imposta le soglie critiche 
 * per gli allarmi hardware e legge periodicamente i dati telemetrici della batteria
 * (Tensione, SOC, Tasso di Carica), stampandoli sul monitor seriale.
 * * @author Emanuele Negrino
 * @date 2026
 */

#include <Arduino.h>
#include <Wire.h>
#include "MAX17048_Gauge.h"

MAX17048_Gauge battery; ///< Istanza globale del Fuel Gauge

float estimateCurrent_mA(float batteryCapacity_mAh);

/**
 * @brief Routine di inizializzazione.
 * Esegue il setup della porta seriale, del bus I2C, verifica la presenza 
 * dell'hardware e configura i parametri operativi del MAX17048.
 */
void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10); // Attesa per schede native USB come Arduino R4 o ESP32
  
  Serial.println(F("\n--- MAX17048 Test ---"));

  // 1. Inizializzazione Hardware
  if (!battery.begin(Wire)) {
    Serial.println(F("ERRORE: Sensore non trovato!"));
  }

  // 2. Informazioni sul Chip
  Serial.print(F("Versione IC: 0x"));
  Serial.println(battery.getICversion(), HEX); // Lettura attesa: 0x0011 o 0x0010
  Serial.print(F("Chip ID: 0x"));
  Serial.println(battery.getChipID(), HEX);

  // 3. Configurazione Soglie Alert
  battery.setAlertVoltages(3.2, 4.25); // Limiti di Tensione: Min 3.2V, Max 4.25V
  battery.setLowSOCThreshold(15);      // Limite di Capacità: Alert sotto il 15%
  
  // 4. Pulizia Iniziale
  // Fondamentale per rimuovere il flag 'Reset Indicator' (RI) che scatta fisicamente 
  // ogni volta che il chip riceve alimentazione iniziale.
  battery.clearAllAlerts();
  
  Serial.println(F("Setup completato con successo.\n"));
}

/**
 * @brief Routine di loop principale.
 * Campiona i dati dal chip I2C e gestisce dinamicamente gli interrupt hardware 
 * (tramite polling in questo caso) qualora i valori superino le soglie definite.
 */
void loop() {
  // Lettura dati telemetrici principali
  float voltage = battery.getVoltage();
  float soc     = battery.getSOC();
  float rate    = battery.getChangeRate();
  bool charging = battery.isCharging();

  // Output Dati in formato Tabellare per analisi seriale
  Serial.print(F("V: "));     Serial.print(voltage, 3); Serial.print(F("V | "));
  Serial.print(F("SOC: "));   Serial.print(soc, 1);     Serial.print(F("% | "));
  Serial.print(F("Rate: "));  Serial.print(rate, 2);    Serial.print(F("%/h | "));
  Serial.print(F("Stato: ")); Serial.println(charging ? F("IN CARICA") : F("A BATTERIA"));

  // Gestione e decodifica degli Alert Hardware registrati dal chip
  if (battery.isActiveAlert()) {
    uint8_t status = battery.getAlertStatus();
    Serial.print(F(">>> ALERT ATTIVO (0x")); Serial.print(status, HEX); Serial.println(F(") <<<"));

    // Analisi Bit a Bit del registro di Status
    if (status & MAX1704X_ALERTFLAG_VOLTAGE_HIGH) Serial.println(F("    - Sovratensione (>4.25V)"));
    if (status & MAX1704X_ALERTFLAG_VOLTAGE_LOW)  Serial.println(F("    - Sottotensione (<3.2V)"));
    if (status & MAX1704X_ALERTFLAG_SOC_LOW)      Serial.println(F("    - Batteria Scarica (<15%)"));
    if (status & MAX1704X_ALERTFLAG_SOC_CHANGE)   Serial.println(F("    - Variazione SOC 1%"));
    
    // Auto-Reset Logico: pulisce l'allarme solo se i parametri sono tornati in sicurezza
    if (voltage > 3.25 && voltage < 4.20 && soc > 16) {
      Serial.println(F("    - Parametri rientrati: Resetto Alert Hardware."));
      battery.clearAllAlerts();
    }
  }

  Serial.println(F("---------------------------------------------------------"));
  delay(2000);
}