cmake_minimum_required(VERSION 3.13)

# Importiert das SDK (PICO_SDK_PATH ist im Docker Image gesetzt)
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(pico_test_project C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialisiert das SDK
pico_sdk_init()

# Erstellt das Executable aus unserer Datei
add_executable(pico_firmware
    src/main.c
)

# Erzeugt .uf2 Dateien etc.
pico_add_extra_outputs(pico_firmware)

# Bindet Standard-Bibliotheken ein
target_link_libraries(pico_firmware pico_stdlib)

# Aktiviert USB Output (optional, gut für Tests)
pico_enable_stdio_usb(pico_firmware 1)
pico_enable_stdio_uart(pico_firmware 1)