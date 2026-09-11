FROM ubuntu:24.04

RUN apt-get update && \
    apt-get install -y --no-install-recommends curl ca-certificates git python3 && \
    rm -rf /var/lib/apt/lists/*

RUN curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | \
    BINDIR=/usr/local/bin sh

# ESP32 core (Xtensa targets are required by the BMV080 precompiled libraries)
RUN arduino-cli config init --additional-urls \
      https://espressif.github.io/arduino-esp32/package_esp32_index.json && \
    arduino-cli core update-index && \
    arduino-cli core install esp32:esp32

# Sensor libraries
RUN arduino-cli lib update-index && \
    arduino-cli lib install "Sensirion Core" "Sensirion I2C SGP40" \
      "Sensirion I2C SGP41" "Sensirion I2C SFA4x" "Sensirion I2C STCC4" && \
    arduino-cli config set library.enable_unsafe_install true && \
    arduino-cli lib install --git-url https://github.com/DFRobot/DFRobot_BMV080.git

WORKDIR /src
CMD ["arduino-cli", "compile", "--fqbn", "esp32:esp32:esp32s3", "--output-dir", "build", "sensor-test"]
