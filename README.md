# Korra

![Firmware](https://img.shields.io/github/actions/workflow/status/mburumaxwell/korra/firmware.yml?branch=main&label=Firmware&style=flat-square)
![Infra](https://img.shields.io/github/actions/workflow/status/mburumaxwell/korra/iac.yml?branch=main&label=Infra&style=flat-square)
[![license](https://img.shields.io/github/license/mburumaxwell/korra.svg?style=flat-square)](LICENSE.md)

<!-- TODO: add description of the project -->
> An explanation will soon be here for the project. Please bear with me.

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
