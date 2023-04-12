/*
* EACHEN Smart WiFi Curtain driver
*/

#ifdef USE_EWELINK_GATEWAY

#define XDRV_100           100

#include <TasmotaSerial.h>

#define WATCHDOG_PIN 14
#define WATCHDOG_TIME 100 // (100 * 100ms = 10s) -> low for 100ms every 10 window

#define STATUS_PIN 13
#define STATUS_TIME 20 // (100 * 100ms = 2s)

#define STATUS_NO_WIFI 0
#define STATUS_NO_SERVER 1
#define STATUS_NORMAL 2

#define SERIAL_BUFFER_SIZE 128
#define EOL 0x1B

/**
* WATCHDOG
* --------
* Pulls the software watchdog pin (GPIO14) low for 100ms in a 10 second window
*/

uint8_t watchdogCount = 0;
uint8_t watchdogState = HIGH;

void ewelink_bridge_setup_watchdog() {
  pinMode(WATCHDOG_PIN, OUTPUT);
  digitalWrite(WATCHDOG_PIN, HIGH);
}

void ewelink_bridge_poll_watchdog() {
  if ((watchdogCount == 0) && (watchdogState != LOW)) {
    digitalWrite(WATCHDOG_PIN, LOW);
    watchdogState = LOW;
  }
  else if (watchdogState != HIGH) {
    digitalWrite(WATCHDOG_PIN, HIGH);
    watchdogState = HIGH;
  }
  if (++watchdogCount >= WATCHDOG_TIME) {
    watchdogCount = 0;
  }
}

/**
* STATUS
* ------
* Emulate the status pin, which indicates the connection status of the connection, using different patterns
* over a 2 second window. Simulate connection: STATUS_NO_WIFI -> STATUS_NO_SERVER -> STATUS_NORMAL 
*/

uint8_t statusPinStates[][STATUS_TIME] = {
  {LOW, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH},
  {LOW, HIGH, LOW,  HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH},
  {LOW, LOW,  LOW,  LOW,  LOW,  LOW,  LOW,  LOW,  LOW,  LOW,  LOW,  LOW,  LOW,  LOW,  LOW,  LOW,  LOW,  LOW,  LOW,  LOW}
};
uint8_t statusWindowCount = 0;
uint8_t statusPinState = HIGH;
uint8_t connectionState = STATUS_NO_WIFI;

uint8_t awaitConnection = 8;
uint8_t stabilizationCount = 10;
bool connectionStable = false;

void ewelink_bridge_setup_status() {
  pinMode(STATUS_PIN, OUTPUT);
  digitalWrite(STATUS_PIN, HIGH);
}

uint8_t ewelink_bridge_status_next_state() {
  if (awaitConnection > 0) {
    awaitConnection--;
    return STATUS_NO_WIFI;
  }
  if (connectionState == STATUS_NO_WIFI) {
    return STATUS_NO_SERVER;
  }
  return STATUS_NORMAL;
}

void ewelink_bridge_poll_status() {
  uint8_t nextState = statusPinStates[connectionState][statusWindowCount];
  if (statusPinState != nextState) {
    digitalWrite(STATUS_PIN, nextState);
    statusPinState = nextState;
  }
  if (++statusWindowCount >= STATUS_TIME) {
    connectionState = ewelink_bridge_status_next_state();
    statusWindowCount = 0;
  }
  // Wait for some time once we are "connected" before signalling that we can start serial comms
  if (!connectionStable && (connectionState == STATUS_NORMAL)) {
    if (--stabilizationCount == 0) {
      connectionStable = true;
      ewelink_bridge_serial_tx(PSTR("AT+START"));
      AddLog(LOG_LEVEL_INFO, PSTR("EWE: Connection Stable"));
    }
  }
}

/*
* SERIAL
* ------
*/

TasmotaSerial *serial = nullptr;
char *rxBuffer = nullptr;
uint16_t rxBufferIndex = 0;

uint8_t dimmer = 0;
uint8_t power = 0;

void ewelink_bridge_setup_serial() {
  rxBuffer = (char*)(malloc(SERIAL_BUFFER_SIZE));
  if (rxBuffer != nullptr) {
    serial = new TasmotaSerial(Pin(GPIO_RXD), Pin(GPIO_TXD), 2);
    if (serial->begin(19200)) {
      if (serial->hardwareSerial()) {
         ClaimSerial();
      }
    }
  }
}

void ewelink_bridge_serial_tx(const char *buffer) {
  if (serial != nullptr) {
    serial->print(buffer);
    serial->write(EOL);
    serial->flush();
  }
}

/**
 * Process the recieved serial data and try to build AT commands from it.
 * Offload the recognised AT commands for processing.
 */
void ewelink_bridge_serial_rx() {
  while (serial->available()) {
    yield();
    uint8_t receivedByte = serial->read();
    if (receivedByte != EOL) {
      // If we're at the end of the buffer without having recognized a command, then just clear everything out
      if (rxBufferIndex >= SERIAL_BUFFER_SIZE - 1) {
        memset(rxBuffer, 0, SERIAL_BUFFER_SIZE);
        rxBufferIndex = 0;
      }
      // Wait for the first 'A', and then capture the rest that follows
      if (((rxBufferIndex == 0) && ('A' == receivedByte)) || (rxBufferIndex > 0)) {
        rxBuffer[rxBufferIndex++] = receivedByte;
      }
    }
    else {
      if (strncmp(rxBuffer, "AT+RESULT", 9) == 0) {
        ewelink_bridge_process_result(rxBuffer + 10);
      }
      else if (strncmp(rxBuffer, "AT+UPDATE", 9) == 0) {
        ewelink_bridge_process_update(rxBuffer + 10);
      }
      memset(rxBuffer, 0, SERIAL_BUFFER_SIZE);
      rxBufferIndex = 0;
    }
  }
}

/**
 * Process AT+RESULT commands that will be sent from the UM8005 in response to AT+UPDATE commands
 * sent to it by us. The response will only contain the sequence number sent to it.
 */
void ewelink_bridge_process_result(char* string) {
  char value[64];
  if (ewelink_bridge_find_parameter(string, "\"sequence\"", value, 64)) {
    AddLog(LOG_LEVEL_INFO, PSTR("EWE: Result Sequence %s"), value);
  }
  // Ack the result
  ewelink_bridge_serial_tx(PSTR("AT+SEND=ok"));
}

/**
 * Process AT+UPDATE commands that will be sent from the UM8005 in response to changes made to the
 * curtain position and will contain the current position. This happens whenever the curtain stops,
 * whether that is because of manually pulling the curtain, using the remote control, or after issuing
 * a position instruction to the curtain from Tasmota.
 */
void ewelink_bridge_process_update(char* string) {
  char value[64]; value[0] = 0;
  if (ewelink_bridge_find_parameter(string, "\"sequence\"", value, 64)) {
    AddLog(LOG_LEVEL_INFO, PSTR("EWE: Update Sequence: %s"), value);
  }
  if (ewelink_bridge_find_parameter(string, "\"setclose\"", value, 64)) {
    AddLog(LOG_LEVEL_INFO, PSTR("EWE: Update Position: %s"), value);
    // Work out the new dimmer and power values based off the device update
    uint8_t updatedDimmer = atoi(value);
    bool updatedPower = updatedDimmer > 0;
    // And calculate whether the values have changed
    bool dimmerChanged = updatedDimmer != dimmer;
    bool powerChanged = updatedPower != power;
    // Update our cached values so updates don't affect us when we trigger then
    dimmer = updatedDimmer;
    power = (dimmer > 0);
    // Trigger a command to update the dimmer if the value changed (manually pulling or remote control)
    if (dimmerChanged) {
      char scmnd[20];
      AddLog(LOG_LEVEL_INFO, PSTR("EWE: Set Dimmer: %d"), dimmer);
      snprintf_P(scmnd, sizeof(scmnd), PSTR(D_CMND_DIMMER " %d"), dimmer);
      ExecuteCommand(scmnd, SRC_SWITCH);
    }
    // Trigger a power update command if the reflected power value should change based on the dimmer value
    if (powerChanged) {
      uint8_t command = power ? POWER_ON : POWER_OFF;
      AddLog(LOG_LEVEL_INFO, PSTR("EWE: Set power: %d"), command);
      ExecuteCommandPower(Light.device, command, SRC_SWITCH);
    }
  }
  // Ack the update
  ewelink_bridge_serial_tx(PSTR("AT+SEND=ok"));
}

/**
 * Search through the AT command looking for a specific key and populating returnValue with the value string
 */
bool ewelink_bridge_find_parameter(char* buffer, const char* parameter, char* returnValue, int maxLength) {
  char newBuffer[SERIAL_BUFFER_SIZE];
  memcpy(newBuffer, buffer, SERIAL_BUFFER_SIZE);
  char *restOfPairs, *keyValuePair, *string = &newBuffer[0];
  while(keyValuePair = strtok_r(string, ",", &restOfPairs)) {
    char *endOfToken;
    char* key = strtok_r(keyValuePair, ":", &endOfToken);
    char* value = endOfToken;
    uint8_t parameterLength = strlen(parameter);
    uint8_t keyLength = strlen(key);
    if ((keyLength >= parameterLength) && (strncmp(key, parameter, parameterLength) == 0)) {
      uint8_t valueLength = strlen(key);
      valueLength = (valueLength < maxLength) ? valueLength : maxLength;
      memcpy(returnValue, value, valueLength);
      returnValue[valueLength] = 0;
      return true;
    }
    string = restOfPairs;
  }
  return false;
}

void ewelink_bridge_set_close(uint8_t closePercentage) {
    char txBuffer[80];
    snprintf_P(txBuffer, sizeof(txBuffer), PSTR("AT+UPDATE=\"sequence\":\"%d%03d\",\"setclose\":%d"), LocalTime(), millis()%1000, closePercentage);
    ewelink_bridge_serial_tx(txBuffer);
}

/*
* SETUP
* -----
*/

boolean ewelink_bridge_module_init() {
  UpdateDevicesPresent(1);
  TasmotaGlobal.light_type = LT_SERIAL1;
  return true;
}

void ewelink_bridge_setup() {
  dimmer = ((Settings->light_dimmer >= 0) && (Settings->light_dimmer <= 100)) ? Settings->light_dimmer : 0;
  power = (TasmotaGlobal.power > 0);
  ewelink_bridge_setup_watchdog();
  ewelink_bridge_setup_status();
  ewelink_bridge_setup_serial();
}

// Timer callback - 100ms
void ewelink_bridge_timer() {
  ewelink_bridge_poll_watchdog();
  ewelink_bridge_poll_status();
}

bool ewelink_bridge_tasmota_updates(const char *eventType) {
  bool switchChanged = TasmotaGlobal.power != power;
  bool dimmerChanged = (light_state.getDimmer() != dimmer);
  if (switchChanged || dimmerChanged) {
    power = TasmotaGlobal.power;
    dimmer = light_state.getDimmer();
    uint8_t percentage = power ? dimmer : 0;
    AddLog(LOG_LEVEL_INFO, PSTR("EWE: Send position: %d"), percentage);
    ewelink_bridge_set_close(percentage);
  }
  return true;
}

bool Xdrv100(uint32_t function) {
  bool result = false;
  switch (function) {
    case FUNC_LOOP:
      if (serial != nullptr) {
        ewelink_bridge_serial_rx();
      }
      break;
    case FUNC_INIT:
      ewelink_bridge_setup();
      break;
    case FUNC_EVERY_100_MSECOND:
      ewelink_bridge_timer();
      break;
    case FUNC_MODULE_INIT:
      result = ewelink_bridge_module_init();
      break;
    case FUNC_SET_DEVICE_POWER:
      result = ewelink_bridge_tasmota_updates("POWER");
      break;
    case FUNC_SET_CHANNELS:
      result = ewelink_bridge_tasmota_updates("CHANNELS");
      break;
  }
  return result;
}

#endif  // USE_EWELINK_GATEWAY
