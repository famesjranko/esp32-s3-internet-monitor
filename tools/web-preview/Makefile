# Host preview of the firmware web UI. See README.md.
FW_DIR        ?= ../../InternetMonitor
ARDUINOJSON   ?= $(HOME)/Arduino/libraries/ArduinoJson/src
CXX           ?= g++
CXXFLAGS      ?= -std=c++17 -O1 -Wall -Wextra -Wno-unused-variable -Wno-unused-function -Wno-unused-parameter
BUILD         := build
OUT           := out

.PHONY: all run portal shots clean
all: $(BUILD)/preview

$(BUILD)/preview: preview.cpp fake_state.h $(wildcard stubs/*.h stubs/*/*.h) $(wildcard $(FW_DIR)/*.h $(FW_DIR)/*/*.h)
	@mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) -DARDUINO=100 -DARDUINOJSON_ENABLE_ARDUINO_STREAM=0 -DARDUINOJSON_ENABLE_ARDUINO_PRINT=0 -DARDUINOJSON_ENABLE_PROGMEM=0 -Istubs -I$(FW_DIR) -isystem $(ARDUINOJSON) -o $@ preview.cpp

run: all
	$(BUILD)/preview

portal: all
	$(BUILD)/preview --portal

shots: all
	./shots.sh

clean:
	rm -rf $(BUILD) $(OUT)
