# Korra

![Firmware](https://img.shields.io/github/actions/workflow/status/mburumaxwell/korra/firmware.yml?branch=main&label=Firmware&style=flat-square)
![Firmware (PIO)](https://img.shields.io/github/actions/workflow/status/mburumaxwell/korra/firmware-pio.yml?branch=main&label=Firmware%20%28PIO%29&style=flat-square)
![Website](https://img.shields.io/github/actions/workflow/status/mburumaxwell/korra/website.yml?branch=main&label=Website&style=flat-square)
![Infra](https://img.shields.io/github/actions/workflow/status/mburumaxwell/korra/iac.yml?branch=main&label=Infra&style=flat-square)
[![license](https://img.shields.io/github/license/mburumaxwell/korra.svg?style=flat-square)](LICENSE.md)

<!-- TODO: add description of the project -->
> An explanation will soon be here for the project. Please bear with me.

## Adding Wi‑Fi Credentials

You can configure the device’s Wi‑Fi settings directly from the serial shell using the following commands:

```bash
wifi-cred-set-open <ssid>
wifi-cred-set-personal <ssid> <passphrase>
wifi-cred-set-ent <ssid> <identity> <username> <password>
```

### Examples of Adding Wi‑Fi Credentials

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

## Installation

<!-- TODO: add instructions for installing zephyr -->

After installing zephyr you may need to install relevant binaries for connectivity to work:

```bash
cd west-manifest && west init --local && cd ../ && \
west update && \
cd firmware && \
pnpm blobs:fetch
```

## Versioning

Versioning for zephyr applications is [detailed in their docs](https://docs.zephyrproject.org/latest/build/version/index.html). This repository uses [changesets](https://github.com/changesets/changesets) to manage versions and changelogs and mostly as private because the tool is fairly mature.

## Infrastructure

The [`deploy` folder](./deploy/) contains the deployment templates for Azure resources used by/for Korra. This forms most of the infrastructure that Korra runs on.

### Azure resources

Ensure the Azure CLI tools are installed and that you are logged in. Then deploy using this command from your terminal while at the root of the repository:

```bash
az deployment group create --resource-group KORRA --template-file deploy/main.bicep --subscription KORRA --confirm-with-what-if
```

## Troubleshooting

Should you get stuck you may consult the [troubleshooting guide](./TROUBLESHOOTING.md).
