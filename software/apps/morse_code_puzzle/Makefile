PROJECT_NAME = $(shell basename "$(realpath ./)")

# Configurations
NRF_IC = nrf52833
SDK_VERSION = 16
SOFTDEVICE_MODEL = blank

# Source and header files
APP_HEADER_PATHS += .
APP_HEADER_PATHS += ../sx1509
APP_HEADER_PATHS += ../lilybuzzer
APP_HEADER_PATHS += ../keypad
APP_SOURCE_PATHS += .
APP_SOURCE_PATHS += ../sx1509
APP_SOURCE_PATHS += ../lilybuzzer
APP_SOURCE_PATHS += ../keypad
APP_SOURCES += $(foreach path,$(APP_SOURCE_PATHS),$(notdir $(wildcard $(path)/*.c)))

# Path to base of nRF52x-base repo
NRF_BASE_DIR = ../../nrf52x-base/

# Include board Makefile (if any)
include ../../boards/microbit_v2/Board.mk

# Include main Makefile
include $(NRF_BASE_DIR)/make/AppMakefile.mk
