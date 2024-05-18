import sys
import os
import logging

from os.path import expanduser

sys.path.append(expanduser("~") + '/.platformio/packages/framework-arduinoespressif32/tools') 

from zeroconf import ServiceBrowser, ServiceListener, Zeroconf
from requests import request
import espota
import argparse
import random
import subprocess

FLASH = 0
SPIFFS = 100

PROGRESS = True
TIMEOUT = 10

revision = (
    subprocess.check_output(["git", "rev-parse", "HEAD"])
    .strip()
    .decode("utf-8")
)

class Listener(ServiceListener):
 
    def update_service(self, zc: Zeroconf, type_: str, name: str) -> None:
        print(f"Service {name} updated")

    def remove_service(self, zc: Zeroconf, type_: str, name: str) -> None:
        print(f"Service {name} removed")

    def add_service(self, zc: Zeroconf, type_: str, name: str) -> None:
        global PROGRESS
        PROGRESS = True
        espota.PROGRESS = True
        global TIMEOUT
        TIMEOUT = 10
        espota.TIMEOUT = 10
        info = zc.get_service_info(type_, name)
        if (name.startswith('btclock-')):
            print(f"Service {name} added")
            print("Address: " + str(info.parsed_addresses()))
            # python  ~/.platformio/packages/framework-arduinoespressif32/tools/espota.py -i 192.168.20.231 -f .pio/build/lolin_s3_mini_qr/firmware.bin -r
            #arguments = [f"-i {str()} -f  -r"]
            namespace = argparse.Namespace(
                esp_ip=info.parsed_addresses()[0], 
                image=f"{os.getcwd()}/.pio/build/lolin_s3_mini_213epd/firmware.bin",
                littlefs=f"{os.getcwd()}/.pio/build/lolin_s3_mini_213epd/littlefs.bin",
                progress=True
                )
            if (str(info.properties.get(b"version").decode())) != "3.0":
                print("Too old version")
                return

            if (str(info.properties.get(b"rev").decode())) == revision:
                print("Newest version, skipping but updating LittleFS")
                espota.serve(namespace.esp_ip, "0.0.0.0", 3232, random.randint(10000,60000), "", namespace.littlefs, SPIFFS)

            else:
                print("Different version, going to update")
                #espota.serve(namespace.esp_ip, "0.0.0.0", 3232, random.randint(10000,60000), "", namespace.littlefs, SPIFFS)

                espota.serve(namespace.esp_ip, "0.0.0.0", 3232, random.randint(10000,60000), "", namespace.image, FLASH)
            #print(arguments)
            
            #logging.basicConfig(level = logging.DEBUG, format = '%(asctime)-8s [%(levelname)s]: %(message)s', datefmt = '%H:%M:%S')
                    
            #espota.serve(namespace.esp_ip, "0.0.0.0", 3232, random.randint(10000,60000), "", namespace.image, FLASH)
            #address = "http://" + info.parsed_addresses()[0]+"/api/restart"
            #response = request("GET", address)
            #espota.main(namespace)

zconf = Zeroconf()
 
serviceListener = Listener()
 
browser = ServiceBrowser(zconf, "_http._tcp.local.", serviceListener)

try:
    input("Press enter to exit...\n\n")
finally:
    zconf.close()