# **Quickstart: Flashing a Raspberry Pi Pico 2**

# Getting the Firmware

You can get the firmware for your Pico in one of two ways:
1. **[GitLab Pipeline](#option-1-download-from-gitlab-recommended)**: Let GitLab build it for you and download the result.
2. **[Build Natively](#option-2-build-natively)**: Build it on your own machine (best for development).

---

## **Option 1: Download from GitLab (Recommended)**

You do not need to install any software to get the latest firmware. You can simply fork this repository and let the GitLab CI/CD pipeline build it for you.

1. **Fork the Project**
   * Click the **Fork** button in the top right of the GitLab repository page.
   * Select your personal namespace.

2. **Run the Pipeline**
   * Go to **Build > Pipelines** in the left sidebar of your forked repository.
   * If a pipeline isn't running already, click **Run pipeline** in the top right.
   * Select the `main` branch and click **Run pipeline**.

3. **Download the Artifacts**
   * Once the pipeline finishes (Green Checkmark ✅), click the **Download** icon (⬇️) on the right side of the pipeline row.
   * Select **Build Artifacts** (or the specific job name, e.g., `build_firmware`).
   * Unzip the download and locate the `.uf2` file.

---

## **Option 2: Build Natively**

If you want to modify the code or develop locally, follow these steps.

### **1. Install Build Dependencies**
You need CMake, the ARM GCC compiler, and standard build tools.

**Ubuntu / Debian / Raspberry Pi OS:**
```
sudo apt update
sudo apt install cmake gcc-arm-none-eabi libnewlib-arm-none-eabi build-essential
```

**Fedora:**
```
sudo dnf install cmake arm-none-eabi-gcc-cs arm-none-eabi-newlib make
```

***macOS:***
```
brew install cmake arm-none-eabi-gcc
```

2. Clone the Repository
```
git clone [https://gitlab.com/your-username/your-project.git](https://gitlab.com/your-username/your-project.git)
cd your-project
```

3. Build the Firmware
We use PICO_SDK_FETCH_FROM_GIT=ON, so the build system will automatically download the Pico SDK for you during the first configuration.
```
mkdir -p build
cd build

# Configure the project (downloads SDK automatically)
cmake -DPICO_SDK_FETCH_FROM_GIT=ON ..

# Compile
make
```

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

