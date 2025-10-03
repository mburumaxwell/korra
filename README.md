# Korra

> [!WARNING]
> As of 03 October 2025, this project and its repository has served its purpose and I no longer do active development here.
> The code remains available for reference and educational purposes.
 
<!-- Project logo could go here in the future -->

![Firmware](https://img.shields.io/github/actions/workflow/status/mburumaxwell/korra/firmware.yml?branch=main&label=Firmware&style=flat-square)
![Firmware (PIO)](https://img.shields.io/github/actions/workflow/status/mburumaxwell/korra/firmware-pio.yml?branch=main&label=Firmware%20%28PIO%29&style=flat-square)
![Website](https://img.shields.io/github/actions/workflow/status/mburumaxwell/korra/website.yml?branch=main&label=Website&style=flat-square)
![Processor](https://img.shields.io/github/actions/workflow/status/mburumaxwell/korra/processor.yml?branch=main&label=Processor&style=flat-square)
![Infra](https://img.shields.io/github/actions/workflow/status/mburumaxwell/korra/iac.yml?branch=main&label=Infra&style=flat-square)
[![license](https://img.shields.io/github/license/mburumaxwell/korra.svg?style=flat-square)](LICENSE.md)

## Overview

**Korra** is a modular IoT platform designed for rapid prototyping and deployment of connected devices. It provides:

- Firmware for embedded devices (Zephyr RTOS and PlatformIO/Arduino)
- Cloud provisioning and connectivity
- Infrastructure-as-Code (Azure)
- A web interface for management and monitoring

> **Note:** This project is under active development. Documentation and features are evolving.

## Features

- Wi-Fi connectivity (Ethernet, and Cellular to come)
- Cloud provisioning and secure credential management
- Modular sensor and actuator support
- Infrastructure templates for Azure
- Web dashboard (Next.js)
- Serial shell for device configuration

## Getting Started

### Prerequisites

- [Zephyr SDK](https://docs.zephyrproject.org/latest/develop/getting_started/index.html) (for `firmware`)
- [PlatformIO](https://platformio.org/) (for `firmware-pio`)
- [Node.js](https://nodejs.org/) and [pnpm](https://pnpm.io/) (for web and scripts)
- Azure CLI (for infrastructure deployment)

### Quick Start

#### 1. Clone the repository

```bash
git clone https://github.com/mburumaxwell/korra.git
cd korra
```

#### 2. Set up Zephyr (for `firmware`)

```bash
west init --local firmware && \
west update && \
west config zephyr.base external/zephyr && \
cd firmware && \
pnpm blobs:fetch
```

When setting up in the IDE, you may need to run `west config zephyr.base external/zephyr` (part of script above), for it to recognize the version in use.

The default board for zephyr is `frdm_rw612` for the `keeper` app. For the IDE to automatically offer navigation, you would need to build this target. Otherwise, you can change the value of `"C_Cpp.default.compileCommands"` in `.vscode/settings.json` to a more befitting one.

#### 3. Set up PlatformIO (for `firmware-pio`)

```bash
# Open with VSCode + PlatformIO extension or use `pio run`
```

#### 4. Set up the Website

```bash
cd website
pnpm install
pnpm dev
```

#### 5. Deploy Infrastructure (Azure)

```bash
az deployment group create --resource-group KORRA --template-file deploy/main.bicep --subscription KORRA --confirm-with-what-if
```

## Adding Wi‑Fi Credentials

You can configure the device's Wi‑Fi settings directly from the serial shell using the following commands:

```bash
wifi-cred-set-open <ssid>
wifi-cred-set-personal <ssid> <passphrase>
wifi-cred-set-ent <ssid> <identity> <username> <password>
```

**Examples:**

```bash
# Open (no encryption) network
wifi-cred-set-open "Home Network"

# Personal (WPA2) network
wifi-cred-set-personal OfficeNetwork mySecretPass

# Enterprise network (EAP)
wifi-cred-set-ent "EnterpriseNet" "user@example.com" alice s3cr3t

# Clear credentials
internet-cred-clear
```

## Directory Structure

- `firmware/` — Zephyr-based firmware for supported boards
- `firmware-pio/` — PlatformIO/Arduino-based firmware
- `processor/` — C#/.NET backend for processing events from IoTHub
- `website/` — Next.js web dashboard
- `deploy/` — Azure infrastructure templates (Bicep)

## Versioning

This repository uses [changesets](https://github.com/changesets/changesets) to manage versions and changelogs. Zephyr application versioning follows [Zephyr docs](https://docs.zephyrproject.org/latest/build/version/index.html).

## Troubleshooting

If you encounter issues, consult the [troubleshooting guide](./TROUBLESHOOTING.md).

## License

This project is licensed under the terms of the [GNU Affero General Public License v3.0 (AGPL-3.0)](LICENSE.md).
