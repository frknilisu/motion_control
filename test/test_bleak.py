"""
UART Service
-------------
An example showing how to write a simple program using the Nordic Semiconductor
(nRF) UART service.
"""

import asyncio
import json
import sys

from bleak import BleakScanner, BleakClient
from bleak.backends.scanner import AdvertisementData
from bleak.backends.device import BLEDevice

MY_DEVICE_NAME = "MyESP32 BLE"

UART_SERVICE_UUID = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
UART_RX_CHAR_UUID = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
UART_TX_CHAR_UUID = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

# All BLE devices have MTU of at least 23. Subtracting 3 bytes overhead, we can
# safely send 20 bytes at a time to any device supporting this service.
UART_SAFE_SIZE = 20

setActionCmdJson = {
    "cmd": "setActionData",
    "data": {
        "motor": {
            "direction": "a2b"
        },
        "capture": {
            "record_duration": 3600,
            "video_duration": 30,
            "fps": 24,
            "shutter_speed": "1_500"
        }
    }
}

messageSeq = [
    "startProgramming",
    "setA",
    "motorRun",
    "motorStop",
    "setB",
    setActionCmdJson,
    "finishProgramming"
]

async def uart_terminal():
    """This is a simple "terminal" program that uses the Nordic Semiconductor
    (nRF) UART service. It reads from stdin and sends each line of data to the
    remote device. Any data received from the device is printed to stdout.
    """

    device = None
    devices = await BleakScanner.discover()
    for d in devices:
        print(d)
        if d.name == MY_DEVICE_NAME:
            device = d
    
    print(device)

    async with BleakClient(device) as client:
        print(client)
        print("Connected, start typing and press ENTER...")

        loop = asyncio.get_running_loop()

        i = 0

        while True:
            input("Press Enter to continue...")
            currMsgObj = messageSeq[i%len(messageSeq)]
            currMsgStr = ""
            if type(currMsgObj) == dict:
                currMsgStr = json.dumps(currMsgObj)
            elif type(currMsgObj) == str:
                currMsgStr = json.dumps({"cmd": currMsgObj})
            await client.write_gatt_char(UART_RX_CHAR_UUID, bytearray(currMsgStr, "utf-8"))
            print("sent:", currMsgStr)
            i += 1

        #response = await client.disconnect()
        #print("DISCONNECT RESPONSE", response)


if __name__ == "__main__":
    try:
        asyncio.run(uart_terminal())
    except asyncio.CancelledError:
        # task is cancelled on disconnect, so we ignore this error
        pass


"""
while True:
    # This waits until you type a line and press ENTER.
    # A real terminal program might put stdin in raw mode so that things
    # like CTRL+C get passed to the remote device.
    data = await loop.run_in_executor(None, sys.stdin.buffer.readline)

    # data will be empty on EOF (e.g. CTRL+D on *nix)
    if not data:
        break

    # some devices, like devices running MicroPython, expect Windows
    # line endings (uncomment line below if needed)
    # data = data.replace(b"\n", b"\r\n")

    await client.write_gatt_char(UART_RX_CHAR_UUID, data)
    print("sent:", data)
"""