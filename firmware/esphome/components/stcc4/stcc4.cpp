#include "stcc4.h"

#include "esphome/core/hal.h"
#include "esphome/core/log.h"

namespace esphome {
namespace stcc4 {

static const char *const TAG = "stcc4";

// Command IDs from the STCC4 datasheet
static const uint16_t STCC4_CMD_START_CONTINUOUS_MEASUREMENT = 0x218B;
static const uint16_t STCC4_CMD_READ_MEASUREMENT = 0xEC05;
static const uint16_t STCC4_CMD_STOP_CONTINUOUS_MEASUREMENT = 0x3F86;

// Standard Sensirion CRC-8: polynomial 0x31, init 0xFF
static uint8_t sensirion_crc8(const uint8_t *data, size_t len) {
  uint8_t crc = 0xFF;
  for (size_t i = 0; i < len; i++) {
    crc ^= data[i];
    for (uint8_t bit = 0; bit < 8; bit++) {
      crc = (crc & 0x80) ? (crc << 1) ^ 0x31 : crc << 1;
    }
  }
  return crc;
}

bool STCC4Component::write_command_(uint16_t command) {
  const uint8_t buffer[2] = {uint8_t(command >> 8), uint8_t(command & 0xFF)};
  return this->write(buffer, sizeof(buffer)) == i2c::ERROR_OK;
}

bool STCC4Component::read_words_(uint16_t *data, size_t count) {
  uint8_t buffer[12];  // large enough for the 4 words of a measurement read
  if (this->read(buffer, 3 * count) != i2c::ERROR_OK) {
    return false;
  }
  for (size_t i = 0; i < count; i++) {
    const uint8_t *word = buffer + 3 * i;
    if (sensirion_crc8(word, 2) != word[2]) {
      ESP_LOGW(TAG, "CRC mismatch on word %u", unsigned(i));
      return false;
    }
    data[i] = (uint16_t(word[0]) << 8) | word[1];
  }
  return true;
}

void STCC4Component::setup() {
  // in case the sensor was left measuring across a soft reboot
  this->write_command_(STCC4_CMD_STOP_CONTINUOUS_MEASUREMENT);
  delay(2);

  if (!this->write_command_(STCC4_CMD_START_CONTINUOUS_MEASUREMENT)) {
    ESP_LOGE(TAG, "Failed to start continuous measurement");
    this->mark_failed();
    return;
  }
}

void STCC4Component::update() {
  if (!this->write_command_(STCC4_CMD_READ_MEASUREMENT)) {
    this->status_set_warning();
    return;
  }

  // datasheet: max. 1 ms command execution time before data is ready
  this->set_timeout("read", 2, [this]() {
    uint16_t words[4];
    if (!this->read_words_(words, 4)) {
      ESP_LOGW(TAG, "Reading measurement failed");
      this->status_set_warning();
      return;
    }
    this->status_clear_warning();

    const int16_t co2 = int16_t(words[0]);
    const float temperature = -45.0f + 175.0f * words[1] / 65535.0f;
    const float humidity = -6.0f + 125.0f * words[2] / 65535.0f;

    ESP_LOGD(TAG, "CO2=%d ppm, T=%.1f°C, RH=%.1f%% (status=0x%04X)", co2, temperature, humidity, words[3]);

    if (this->co2_sensor_ != nullptr)
      this->co2_sensor_->publish_state(co2);
    if (this->temperature_sensor_ != nullptr)
      this->temperature_sensor_->publish_state(temperature);
    if (this->humidity_sensor_ != nullptr)
      this->humidity_sensor_->publish_state(humidity);
  });
}

void STCC4Component::dump_config() {
  ESP_LOGCONFIG(TAG, "STCC4:");
  LOG_I2C_DEVICE(this);
  LOG_UPDATE_INTERVAL(this);
  LOG_SENSOR("  ", "CO2", this->co2_sensor_);
  LOG_SENSOR("  ", "Temperature (SHT40 via STCC4)", this->temperature_sensor_);
  LOG_SENSOR("  ", "Humidity (SHT40 via STCC4)", this->humidity_sensor_);
}

}  // namespace stcc4
}  // namespace esphome
