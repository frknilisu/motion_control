#! /usr/bin/python3

import asyncio
import json
import sys
import logging
from time import sleep
import tkinter as tk
from tkinter import Tk, Scale, Button, ttk

logging.basicConfig(
    format='%(asctime)s %(levelname)-8s %(message)s',
    level=logging.INFO,
    datefmt='%Y-%m-%d %H:%M:%S')

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

manualDriveCmdJson = {
    "cmd": "manualDrive",
    "speed": 200
}

setActionDataCmdJson = {
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
    "setB",
    setActionDataCmdJson,
    "finishProgramming",
    "motorRun",
    "motorStop"
]

fpsList = [
    "12", "24", "25", "30", "48", "50", "60", "96", "100", "120"
]

shutterSpeedList = [
    "60", "30", "15", "8", "4", "2", "1", 
    "1/2", "1/4", "1/8", "1/15", "1/30", "1/60", "1/80", 
    "1/125", "1/250", "1/500", "1/1000", "1/2000", "1/4000", "1/8000"
]

class AsyncTk(Tk):

    def __init__(self):
        super().__init__()
        self.geometry('400x300')
        self.title('Slider Demo')
        self.grid_columnconfigure((0, 1), weight=1)

        self.slider_value = tk.IntVar()
        self.next_msg = ""

        self.slider = ttk.Scale(
            master=self,
            from_=-1000,
            to=1000,
            orient='horizontal',
            variable=self.slider_value
        )

        self.slider.set(0)
        self.slider.bind('<ButtonRelease-1>', self.setZero)
        self.slider.grid(row=0, column=0, columnspan=5)

        btn1 = ttk.Button(self, text='Start Programming', command=lambda: self.assign_next_msg(0))
        btn2 = ttk.Button(self, text='SetA', command=lambda: self.assign_next_msg(1))
        btn3 = ttk.Button(self, text='SetB', command=lambda: self.assign_next_msg(2))
        btn4 = ttk.Button(self, text='Set Action Data', command=lambda: self.assign_next_msg(3))
        btn5 = ttk.Button(self, text='Finish Programming', command=lambda: self.assign_next_msg(4))

        label1 = ttk.Label(self, text="Timelapse Duration")
        label2 = ttk.Label(self, text="FPS")
        label3 = ttk.Label(self, text="Video Duration")
        label4 = ttk.Label(self, text="Shutter Speed")
        label5 = ttk.Label(self, text="Total Frames")

        entry1 = ttk.Entry(self)
        combo1 = ttk.Combobox(self, values = fpsList)
        entry2 = ttk.Entry(self)
        combo2 = ttk.Combobox(self, values = shutterSpeedList)
        combo1.set("12")
        combo2.set("1/500")

        btn1.grid(row=1, column=0, columnspan=5)
        btn2.grid(row=2, column=0, columnspan=5)
        btn3.grid(row=3, column=0, columnspan=5)
        label1.grid(row=4, column=0, sticky="e")
        entry1.grid(row=4, column=1, sticky="w")
        label2.grid(row=5, column=0, sticky="e")
        combo1.grid(row=5, column=1, sticky="w")
        label3.grid(row=6, column=0, sticky="e")
        entry2.grid(row=6, column=1, sticky="w")
        label4.grid(row=7, column=0, sticky="e")
        combo2.grid(row=7, column=1, sticky="w")
        label5.grid(row=8, column=0, sticky="e")
        btn4.grid(row=9, column=0, columnspan=5)
        btn5.grid(row=10, column=0, columnspan=5)

        self.running = True
        self.runners = [self.tk_loop()]

    async def tk_loop(self):
        while self.running:
            self.update()
            await asyncio.sleep(0.05)

    def select(self, option):
        print(option)

    def setZero(self, event):
        self.slider.set(0)

    def assign_next_msg(self, idx):
        print("assign_next_msg:", idx)
        self.next_msg = messageSeq[idx]

    def stop(self):
        self.running = False

    async def run(self):
        await asyncio.gather(*self.runners)


class App(AsyncTk):

    def __init__(self):
        super().__init__()
        #self.create_interface()
        #self.runners.append(self.uart_terminal())

    async def uart_terminal(self):
        print("uart_terminal")
        zeroSpeedCounter = 0
        device = None
        devices = await BleakScanner.discover()
        for d in devices:
            print(d)
            if d.name == MY_DEVICE_NAME:
                device = d
        
        print(device)

        async with BleakClient(device) as client:
            print(client)

            while self.running:
                #next_msg = self.next_msg.get()
                if self.next_msg:
                    currMsgObj = self.next_msg
                    self.next_msg = ""
                    #self.next_msg.set("")
                else:
                    speed = int(self.slider_value.get())
                    if speed == 0:
                        zeroSpeedCounter += 1
                    else:
                        zeroSpeedCounter = 0
                    if zeroSpeedCounter > 2:
                        await asyncio.sleep(1.0)
                        continue
                    manualDriveCmdJson["speed"] = speed
                    currMsgObj = manualDriveCmdJson
                currMsgStr = ""

                if type(currMsgObj) == dict:
                    currMsgStr = json.dumps(currMsgObj)
                elif type(currMsgObj) == str:
                    currMsgStr = json.dumps({"cmd": currMsgObj})
                
                await client.write_gatt_char(UART_RX_CHAR_UUID, bytearray(currMsgStr, "utf-8"))
                logging.info("sent: {}".format(currMsgStr))
                await asyncio.sleep(1.0)

async def main():
    app = App()
    await app.run()

if __name__ == "__main__":
    asyncio.run(main())