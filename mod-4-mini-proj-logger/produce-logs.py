#!/usr/bin/env python3

from time import sleep

with open('/dev/cu.usbmodem3368367631321', mode='a') as serial:
    for i in range(2000):
        print("log Log number: ", i, file=serial)
        sleep(0.01)
