# Korra

![Infra](https://img.shields.io/github/actions/workflow/status/mburumaxwell/korra/iac.yml?branch=main&label=Infra&style=flat-square)
[![license](https://img.shields.io/github/license/mburumaxwell/korra.svg?style=flat-square)](LICENSE.md)

<!-- TODO: add description of the project -->
> An explanation will soon be here for the project. Please bear with me.

## Infrastructure

The [`deploy` folder](./deploy/) contains the deployment templates for Azure resources used by/for Korra. This forms most of the infrastructure that Korra runs on.

### Azure resources

Ensure the Azure CLI tools are installed and that you are logged in. Then deploy using this command from your terminal while at the root of the repository:

```bash
az deployment group create --resource-group KORRA --template-file deploy/main.bicep --subscription KORRA --confirm-with-what-if
```
