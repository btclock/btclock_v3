# BTClock v3

Software for the BTClock project. Highly experimental version.

Biggest differences are:
- Uses WebSockets for all data
- Able to configure WiFi using the Improv protocol 
- Built on the ESP-IDF with Arduino as a library 
- Makes better use of native timers and interrupts
- Able to be flashed over-the-air (using ESP OTA)
- Added market capitalization screen
- LED flash on new block (and focus to block height screen on new block)

Most [information](https://github.com/btclock/btclock_v2/wiki) about BTClock v2 is still valid for this version.

**NOTE**: The software assumes that the hardware is run in a controlled private network. The Web UI and the OTA update mechanism are not password protected and accessible to anyone in the network.

## Known issues
- After starting it might take a while before the correct data is displayed
- Quite often the screens will hang, especially after updating