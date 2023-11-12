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

Hold the top button while resetting to reset WiFi settings. Hold the 2nd button from the top while resetting to activate QR-code WiFi configuration.

"Steal focus on new block" means that when a new block is mined, the display will switch to the block height screen if it's not on it already.

Most [information](https://github.com/btclock/btclock_v2/wiki) about BTClock v2 is still valid for this version.

**NOTE**: The software assumes that the hardware is run in a controlled private network. The Web UI and the OTA update mechanism are not password protected and accessible to anyone in the network. Also, since the device only fetches numbers through WebSockets it will skip server certificate verification to save resources.

## Known issues
- After starting it might take a while before the correct data is displayed
- Quite often the screens will hang, especially after updating