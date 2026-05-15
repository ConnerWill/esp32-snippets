/*
 * Water Level Sensor with LED Bar Graph and Empty Warning LED
 *
 * Sensor:
 *    POWER_PIN  -> sensor VCC
 *    SIGNAL_PIN -> sensor signal output
 *
 * LEDs:
 *    D2 -> full / highest level LED
 *    D3 -> next level down
 *    D4 -> next level down
 *    D5 -> lowest non-empty level LED
 *    D6 -> red empty LED
 *
 * Example wiring:
 *
 *    Arduino Pin   Purpose
 *    D7            Sensor power
 *    A5            Sensor signal
 *    D2            Full / highest level LED
 *    D3            High level LED
 *    D4            Medium level LED
 *    D5            Low level LED
 *    D6            Red empty LED
 */

#include <Arduino.h>

constexpr uint8_t POWER_PIN = 7;
constexpr uint8_t SIGNAL_PIN = A5;

constexpr uint8_t EMPTY_LED_PIN = 6;

/*
 * Calibrate these values for your actual sensor.
 *
 * SENSOR_MIN should be the raw analog value when the sensor is dry.
 * SENSOR_MAX should be the raw analog value at your desired "full" water level.
 */
constexpr uint16_t SENSOR_MIN = 0;
constexpr uint16_t SENSOR_MAX = 521;

constexpr uint8_t LEVEL_MIN = 0;
constexpr uint8_t LEVEL_MAX = 4;

/*
 * LED pins used as a bar graph.
 *
 * Important:
 *   This array is ordered from LOWEST non-empty level to HIGHEST level.
 *
 * Physical layout:
 *   D2 = full / highest
 *   D3 = high
 *   D4 = medium
 *   D5 = low
 *
 * Code order:
 *   D5 turns on first at level 1.
 *   D2 turns on last at level 4.
 */
const uint8_t LED_PINS[] = {5, 4, 3, 2};
constexpr uint8_t LED_COUNT = sizeof(LED_PINS) / sizeof(LED_PINS[0]);

/*
 * Reading settings.
 */
constexpr unsigned long READ_INTERVAL_MS = 1000;
constexpr uint8_t SENSOR_SETTLE_MS = 10;
constexpr uint8_t SAMPLE_COUNT = 10;
constexpr uint8_t SAMPLE_DELAY_MS = 2;

unsigned long lastReadMs = 0;

/**
 * Reads the water sensor using multiple samples and returns the averaged raw value.
 */
uint16_t readWaterSensor() {
  uint32_t total = 0;

  digitalWrite(POWER_PIN, HIGH);
  delay(SENSOR_SETTLE_MS);

  for (uint8_t i = 0; i < SAMPLE_COUNT; i++) {
    total += analogRead(SIGNAL_PIN);
    delay(SAMPLE_DELAY_MS);
  }

  digitalWrite(POWER_PIN, LOW);

  return static_cast<uint16_t>(total / SAMPLE_COUNT);
}

/**
 * Converts the raw analog sensor value into a water level from 0 to 4.
 */
uint8_t valueToLevel(uint16_t rawValue) {
  const long clampedValue = constrain(
    static_cast<long>(rawValue),
    static_cast<long>(SENSOR_MIN),
    static_cast<long>(SENSOR_MAX)
  );

  const long mappedLevel = map(
    clampedValue,
    SENSOR_MIN,
    SENSOR_MAX,
    LEVEL_MIN,
    LEVEL_MAX
  );

  return static_cast<uint8_t>(
    constrain(
      mappedLevel,
      static_cast<long>(LEVEL_MIN),
      static_cast<long>(LEVEL_MAX)
    )
  );
}

/**
 * Updates the LEDs based on the current water level.
 *
 * If level is 0:
 *   - red empty LED turns on
 *   - all level LEDs turn off
 *
 * If level is 1-4:
 *   - red empty LED turns off
 *   - level LEDs act like a bar graph
 *
 * Level behavior:
 *   Level 0 = D6
 *   Level 1 = D5
 *   Level 2 = D5, D4
 *   Level 3 = D5, D4, D3
 *   Level 4 = D5, D4, D3, D2
 */
void updateWaterLevelLeds(uint8_t level) {
  digitalWrite(EMPTY_LED_PIN, level == 0 ? HIGH : LOW);

  for (uint8_t i = 0; i < LED_COUNT; i++) {
    digitalWrite(LED_PINS[i], i < level ? HIGH : LOW);
  }
}

/**
 * Returns a human-readable status string for the current water level.
 */
const char *levelToStatus(uint8_t level) {
  switch (level) {
    case 0:
      return "EMPTY";
    case 1:
      return "LOW";
    case 2:
      return "MEDIUM";
    case 3:
      return "HIGH";
    case 4:
      return "FULL";
    default:
      return "UNKNOWN";
  }
}

/**
 * Prints the current sensor status to the serial monitor.
 */
void printWaterStatus(uint16_t rawValue, uint8_t level) {
  Serial.print(F("Raw: "));
  Serial.print(rawValue);

  Serial.print(F(" | Level: "));
  Serial.print(level);

  Serial.print(F(" | Status: "));
  Serial.println(levelToStatus(level));
}

void setup() {
  Serial.begin(9600);

  pinMode(POWER_PIN, OUTPUT);
  digitalWrite(POWER_PIN, LOW);

  pinMode(EMPTY_LED_PIN, OUTPUT);
  digitalWrite(EMPTY_LED_PIN, LOW);

  for (uint8_t i = 0; i < LED_COUNT; i++) {
    pinMode(LED_PINS[i], OUTPUT);
    digitalWrite(LED_PINS[i], LOW);
  }

  Serial.println(F("Water level monitor started"));
}

void loop() {
  const unsigned long now = millis();

  if (now - lastReadMs >= READ_INTERVAL_MS) {
    lastReadMs = now;

    const uint16_t rawValue = readWaterSensor();
    const uint8_t level = valueToLevel(rawValue);

    updateWaterLevelLeds(level);
    printWaterStatus(rawValue, level);
  }
}