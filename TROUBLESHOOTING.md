# Troubleshooting

A few things may be an issue but I have described them here for my own use and for any other person sho might need them.

## Enterprise WiFi

To test using the [`samples/net/wifi/shell` sample](https://github.com/zephyrproject-rtos/zephyr/tree/main/samples/net/wifi/shell), try these commands:

```bash
wifi connect -s "eduroam" -k 14 -I "<username>@ljmu.ac.uk" -P "<password>" -a "anonymous"
```

```bash
wifi connect -s "ASK4 Wireless (802.1x)" -k 14 -I "<username>" -P "<password>" -a "anonymous"
```
