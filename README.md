# NeuronWireless_raise2
nRF MCU family version for the Neuron Platform

## Cloning the Repository with Submodules
To get a full copy of the repository, including all submodules:

1. Clone the repository:

```bash
git clone git@github.com:Dygmalab/NRF-Neuron.git
```

2. Navigate to the cloned directory and update the submodules:

```bash
cd NRF-Neuron && git submodule update --init --recursive
```

## Target device
NeuronWireless_raise2 is built to be loaded into the Nordic Semiconductor nRF52833 microcontroller.

## FLASH Memory Architecture

|                       |                           |
|-----------------------|---------------------------|
| NeuronWireless_raise2   | 0x0002E000 - ...          |
|-----------------------|---------------------------|
| rf_host_device        | 0x00027000 - 0x0002DFFF   |
|-----------------------|---------------------------|
| Softdevice S140       | 0x00001000 - 0x00026FFF   |
| MBR                   | 0x00000000 - 0x00000FFF   |

## Requirements
* `make 4.3`
* `gcc-arm-none-eabi 10.3`
* `python 3.11`
* `IntelHex` Python library

## NRF-Neuron Project Setup Guide on Ubuntu

To cover these requirements in Ubuntu based distributions, install the `build-essential` package. For other Linux distros install them independently.

Install the toolchain for the arm chips

```sudo apt install gcc-arm-none-eabi```

For installing the `IntelHex` call

```sudo pip install intelhex```

## Get NRF SDK

* Download the Nordic Semiconductor SDK [nrf5_sdk_17.1.0_ddde560](https://nsscprodmedia.blob.core.windows.net/prod/software-and-other-downloads/sdks/nrf5/binaries/nrf5_sdk_17.1.0_ddde560.zip)
* Unpack the `nrf5_sdk_17.1.0_ddde560` into the `libraries/SDK/nRF5_SDK_17.1.0_ddde560` folder

## Build

`cd NeuronWireless_raise2/build`

`make release` or `make debug`

## Flash

In order to upload the firmware into the Neuron, download Dygma flasher tool

https://github.com/Dygmalab/firmware-flasher/releases/latest

Follow the instructions described in its repository.

## NRF-Neuron Project Setup Guide on Windows 10

## Setting up the Compilation Environment

1. Download the `gcc-arm-none-eabi-10.3-2021.10-win32.exe` package from the following link: [ARM Developer Downloads](https://developer.arm.com/downloads/-/gnu-rm).

2. Install the downloaded package in `C:\gcc-arm`.

3. Add the `C:\gcc-arm\bin` directory to your system's PATH.

4. Download and install MSYS2 from this link: [MSYS2](https://www.msys2.org/).

5. MSYS2 comes with five different terminals. Open the `MSYS2 MINGW64` terminal and run the following command to update the packages:

```bash
pacman -Syu
```

6. Next, execute the following command to install `make`:

```bash
pacman -S make
```

7. Add the `arm-none-eabi-gcc` compiler to the MSYS2 PATH. Go to `C:\msys64\home\<YourUserName>` and add the following line to the `.bashrc` file: 

```bash
export PATH=$PATH:/c/gcc-arm/bin
```

8. Close and reopen the terminal. Check that the compiler has been added successfully by running:

```bash
arm-none-eabi-gcc --version
arm-none-eabi-g++ --version
```

## Get NRF SDK

* Download the Nordic Semiconductor SDK [nrf5_sdk_17.1.0_ddde560](https://nsscprodmedia.blob.core.windows.net/prod/software-and-other-downloads/sdks/nrf5/binaries/nrf5_sdk_17.1.0_ddde560.zip)
* Unpack the `nrf5_sdk_17.1.0_ddde560` into the `libraries/SDK/nRF5_SDK_17.1.0_ddde560` folder

## Setting up the Flashing Tool

1. Download and install 'nRF Connect for Desktop' from this link: [nRF Connect for Desktop](https://www.nordicsemi.com/Products/Development-tools/nrf-connect-for-desktop).

2. Open 'nRF Connect for Desktop' and install the ‘Programmer’ app to flash the project onto the microcontroller.

With these steps, you should have successfully set up your development and flashing environment for the NRF-Neuron project.
