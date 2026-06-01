/**
 * @file MAX17048_Gauge.cpp
 * @brief Implementazione della libreria MAX17048_Gauge.
 * * Contiene l'implementazione dei metodi per la comunicazione I2C, la gestione 
 * degli stati e la decodifica dei registri hardware del MAX17048.
 */

#include "MAX17048_Gauge.h"

MAX17048_Gauge::MAX17048_Gauge() {}

MAX17048_Gauge::~MAX17048_Gauge() {}

// ==============================================================================
// I2C CORE OTTIMIZZATO (CON AUTO-RETRY)
// ==============================================================================

uint16_t MAX17048_Gauge::readRegister(uint8_t reg) {
    // Tenta la lettura fino a 3 volte per filtrare eventuali disturbi elettrici (EMI)
    for (uint8_t retries = 0; retries < 3; retries++) {
        _i2cPort->beginTransmission(_address);
        _i2cPort->write(reg);
        
        // Chiudiamo la trasmissione in modo netto per pulire il bus
        if (_i2cPort->endTransmission() == 0) { 
            if (_i2cPort->requestFrom(_address, (uint8_t)2) == 2) {
                uint16_t value = _i2cPort->read() << 8;
                value |= _i2cPort->read();
                return value; // Lettura andata a buon fine
            }
        }
        delay(5); // Pausa prima di riprovare
    }
    
    // In caso di guasto totale del bus, restituisce 0x0000.
    // Questo evita i NAN matematici, i finti allarmi 0x7F e i falsi Rate negativi.
    return 0x0000; 
}

bool MAX17048_Gauge::writeRegister(uint8_t reg, uint16_t value) {
    // Tenta la scrittura fino a 3 volte
    for (uint8_t retries = 0; retries < 3; retries++) {
        _i2cPort->beginTransmission(_address);
        _i2cPort->write(reg);
        _i2cPort->write(value >> 8);   // Invia MSB (Big-Endian)
        _i2cPort->write(value & 0xFF); // Invia LSB
        
        uint8_t error = _i2cPort->endTransmission();
        
        if (error == 0) return true;
        
        // Se inviamo il comando di Soft Reset, l'IC si riavvia istantaneamente
        // causando un NACK hardware intenzionale. Ignoriamo questo specifico errore.
        if (reg == MAX1704X_CMD_REG) return true; 
        
        delay(5);
    }
    return false;
}

// ==============================================================================
// INIT E CONTROLLO HARDWARE
// ==============================================================================

bool MAX17048_Gauge::begin(TwoWire &wirePort) {
    _i2cPort = &wirePort;
    _i2cPort->begin();

    if (!isDeviceReady()) return false; 
    if (!reset()) return false;

    enableSleep(false);
    sleep(false);
    
    return true;
}

uint16_t MAX17048_Gauge::getICversion() {
    return readRegister(MAX1704X_VERSION_REG);
}

uint8_t MAX17048_Gauge::getChipID() {
    return readRegister(MAX1704X_VRESET_REG) & 0xFF;
}

bool MAX17048_Gauge::isDeviceReady() {
    return (getICversion() & 0xFFF0) == 0x0010;
}

bool MAX17048_Gauge::reset() {
    writeRegister(MAX1704X_CMD_REG, 0x5400);
    delay(10); 

    // Pulisce l'alert di "Reset Indicator" generato dall'accensione
    for (uint8_t retries = 0; retries < 3; retries++) {
        if (clearAlertFlag(MAX1704X_ALERTFLAG_RESET_INDICATOR)) {
            return true;
        }
        delay(10);
    }
    return false;
}

void MAX17048_Gauge::quickStart() {
    uint16_t mode = readRegister(MAX1704X_MODE_REG);
    mode |= (1 << 14); 
    writeRegister(MAX1704X_MODE_REG, mode);
}

// ==============================================================================
// LETTURE PRINCIPALI
// ==============================================================================

float MAX17048_Gauge::getVoltage() {
    return readRegister(MAX1704X_VCELL_REG) * 0.000078125f; 
}

float MAX17048_Gauge::getSOC() {
    return readRegister(MAX1704X_SOC_REG) / 256.0f;
}

float MAX17048_Gauge::getChangeRate() {
    // Il cast a int16_t permette di gestire correttamente i valori negativi (scarica)
    return (int16_t)readRegister(MAX1704X_CRATE_REG) * 0.208f; 
}

bool MAX17048_Gauge::isCharging() {
    float rate = getChangeRate();

    // Macchina a stati con isteresi per filtrare il rumore analogico
    if (_isChargingState) {
        if (rate < 0.1f) _isChargingState = false; 
    } else {
        if (rate > 0.5f) _isChargingState = true;  
    }
    return _isChargingState;
}

// ==============================================================================
// GESTIONE ALERT
// ==============================================================================

void MAX17048_Gauge::setLowSOCThreshold(uint8_t percent) {
    percent = constrain(percent, 1, 32);
    uint16_t config = readRegister(MAX1704X_CONFIG_REG);
    config &= 0xFFE0; // Pulisce i 5 bit bassi
    config |= (32 - percent);
    writeRegister(MAX1704X_CONFIG_REG, config);
}

void MAX17048_Gauge::setAlertVoltages(float minv, float maxv) {
    uint8_t minVal = constrain((int)(minv / 0.020f), 0, 255);
    uint8_t maxVal = constrain((int)(maxv / 0.020f), 0, 255);
    // MSB = Soglia Minima, LSB = Soglia Massima
    writeRegister(MAX1704X_VALERT_REG, (minVal << 8) | maxVal);
}

void MAX17048_Gauge::getAlertVoltages(float &minv, float &maxv) {
    uint16_t valrt = readRegister(MAX1704X_VALERT_REG);
    minv = (valrt >> 8) * 0.020f;
    maxv = (valrt & 0xFF) * 0.020f;
}

bool MAX17048_Gauge::isActiveAlert() {
    return (readRegister(MAX1704X_CONFIG_REG) & (1 << 5)) > 0;
}

uint8_t MAX17048_Gauge::getAlertStatus() {
    return (readRegister(MAX1704X_STATUS_REG) >> 8) & 0x7F;
}

bool MAX17048_Gauge::clearAlertFlag(uint8_t flag) {
    uint16_t status = readRegister(MAX1704X_STATUS_REG);
    status &= ~((uint16_t)flag << 8); 
    writeRegister(MAX1704X_STATUS_REG, status);
    return (getAlertStatus() & flag) == 0;
}

void MAX17048_Gauge::clearAllAlerts() {
    uint16_t status = readRegister(MAX1704X_STATUS_REG);
    status &= 0x00FF; // Pulisce i flag nel MSB
    writeRegister(MAX1704X_STATUS_REG, status);
    
    uint16_t config = readRegister(MAX1704X_CONFIG_REG);
    config &= ~(1 << 5); // Disasserisce il pin fisico ALRT
    writeRegister(MAX1704X_CONFIG_REG, config);
}

// ==============================================================================
// GESTIONE VRESET
// ==============================================================================

void MAX17048_Gauge::setResetVoltage(float reset_v) {
    uint16_t reg = readRegister(MAX1704X_VRESET_REG);
    uint8_t val = constrain((int)(reset_v / 0.040f), 0, 127); 
    reg = (reg & 0x00FF) | (val << 8); 
    writeRegister(MAX1704X_VRESET_REG, reg);
}

float MAX17048_Gauge::getResetVoltage() {
    return (readRegister(MAX1704X_VRESET_REG) >> 8) * 0.040f;
}

// ==============================================================================
// POWER MANAGEMENT E HIBERNATION
// ==============================================================================

void MAX17048_Gauge::setActivityThreshold(float actthresh) {
    uint16_t hibrt = readRegister(MAX1704X_HIBRT_REG);
    uint8_t val = constrain((int)(actthresh / 0.00125f), 0, 255); 
    hibrt = (hibrt & 0xFF00) | val; 
    writeRegister(MAX1704X_HIBRT_REG, hibrt);
}

float MAX17048_Gauge::getActivityThreshold() {
    return (readRegister(MAX1704X_HIBRT_REG) & 0xFF) * 0.00125f;
}

void MAX17048_Gauge::setHibernationThreshold(float hibthresh) {
    uint16_t hibrt = readRegister(MAX1704X_HIBRT_REG);
    uint8_t val = constrain((int)(hibthresh / 0.208f), 0, 255); 
    hibrt = (hibrt & 0x00FF) | (val << 8); 
    writeRegister(MAX1704X_HIBRT_REG, hibrt);
}

float MAX17048_Gauge::getHibernationThreshold() {
    return (readRegister(MAX1704X_HIBRT_REG) >> 8) * 0.208f;
}

void MAX17048_Gauge::hibernate() {
    writeRegister(MAX1704X_HIBRT_REG, 0xFFFF);
}

void MAX17048_Gauge::wake() {
    writeRegister(MAX1704X_HIBRT_REG, 0x0000);
}

bool MAX17048_Gauge::isHibernating() {
    return (readRegister(MAX1704X_MODE_REG) & (1 << 12)) > 0;
}

void MAX17048_Gauge::enableSleep(bool en) {
    uint16_t mode = readRegister(MAX1704X_MODE_REG);
    if (en) mode |= (1 << 13);
    else mode &= ~(1 << 13);
    writeRegister(MAX1704X_MODE_REG, mode);
}

void MAX17048_Gauge::sleep(bool s) {
    uint16_t config = readRegister(MAX1704X_CONFIG_REG);
    if (s) config |= (1 << 7);
    else config &= ~(1 << 7);
    writeRegister(MAX1704X_CONFIG_REG, config);
}

// ==============================================================================
// COMPENSAZIONE
// ==============================================================================

void MAX17048_Gauge::setRCOMP(uint8_t rcomp) {
    uint16_t config = readRegister(MAX1704X_CONFIG_REG);
    config = (config & 0x00FF) | (rcomp << 8);     
    writeRegister(MAX1704X_CONFIG_REG, config);
}