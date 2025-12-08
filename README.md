# **Quickstart: Flashing a Raspberry Pi Pico 2**

## **1. Install Dependencies**

```bash
sudo apt install libusb-1.0-0-dev
```

## **2. Install the Pico SDK**

```bash
git clone https://github.com/raspberrypi/pico-sdk.git
cd pico-sdk
git submodule update --init --recursive
```

## **3. Build the Firmware**

```bash
mkdir -p build
cd build
cmake ..
make 
```

Your `.uf2` file will appear in the `build/` directory.

## **5. Enter Bootloader Mode**

1. Unplug the Pico
2. Hold **BOOTSEL**
3. Plug in USB
4. Release BOOTSEL
   A drive appears:

* **RP2350** → Pico 2

## **6. Flash the UF2**

Drag your `.uf2` into the Pico’s USB storage device.

If the file is valid:

* The drive **disappears**
* The Pico **reboots automatically**

If it **stays mounted**, your UF2 is invalid (wrong board, corrupted build, etc.).

