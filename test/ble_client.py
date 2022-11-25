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

ONLINE = True

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
    "gotoOrigin",
    "startAction",
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
        self.geometry('720x600')
        self.title('Slider Demo')
        self.grid_columnconfigure((0, 1), weight=1)
        self.config(padx=20, pady=20, bg='gray')

        self.slider_value = tk.IntVar()
        self.next_msg = ""
        self.totalFrame = tk.IntVar()
        self.totalFrame.set(0)

        self.var_timelapseDuration = tk.StringVar()
        self.var_fps = tk.StringVar()
        self.var_videoDuration = tk.StringVar()
        self.var_shutterSpeed = tk.StringVar()
        self.var_direction = tk.StringVar(value="a2b")

        self.slider = ttk.Scale(
            master=self,
            orient=tk.HORIZONTAL,
            from_=-1000,
            to=1000,
            variable=self.slider_value,
            length=200,
            cursor="plus"
        )
        self.slider.set(0)
        self.slider.bind('<ButtonRelease-1>', self.setZero)

        btn_startProgramming = ttk.Button(self, text='Start Programming', command=lambda: self.assign_next_msg(0))
        btn_setA = ttk.Button(self, text='SetA', command=lambda: self.assign_next_msg(1))
        btn_setB = ttk.Button(self, text='SetB', command=lambda: self.assign_next_msg(2))
        btn_setActionData = ttk.Button(self, text='Set Action Data', command=self.setActionData)
        btn_finishProgramming = ttk.Button(self, text='Finish Programming', command=lambda: self.assign_next_msg(4))
        btn_gotoOrigin = ttk.Button(self, text='Goto Origin', command=lambda: self.assign_next_msg(5))
        btn_startAction = ttk.Button(self, text='Start Action', command=lambda: self.assign_next_msg(6))

        label_sliderValue = ttk.Label(self, textvariable=self.slider_value)
        label_timelapseDuration = ttk.Label(self, text="Timelapse Duration")
        label_fps = ttk.Label(self, text="FPS")
        label_videoDuration = ttk.Label(self, text="Video Duration")
        label_shutterSpeed = ttk.Label(self, text="Shutter Speed")
        label_totalFrames = ttk.Label(self, text="Total Frames")
        label_numberOfTotalFrame = ttk.Label(self, textvariable=self.totalFrame)

        self.entry_timelapseDuration = ttk.Entry(self, textvariable=self.var_timelapseDuration)
        self.entry_videoDuration = ttk.Entry(self, textvariable=self.var_videoDuration)
        self.entry_videoDuration.bind('<FocusOut>', self.getTimelapseDuration)
        self.entry_videoDuration.bind('<FocusOut>', self.getVideoDuration)

        self.combo_fps = ttk.Combobox(self, values=fpsList, textvariable=self.var_fps, state='readonly')
        self.combo_shutterSpeed = ttk.Combobox(self, values=shutterSpeedList, textvariable=self.var_shutterSpeed, state='readonly')
        self.combo_fps.set("24")
        self.combo_shutterSpeed.set("1/500")
        self.combo_fps.bind("<<ComboboxSelected>>", self.fpsSelected)
        self.combo_shutterSpeed.bind("<<ComboboxSelected>>", self.shutterSpeedSelected)

        rbtn_a2b = ttk.Radiobutton(self, text="A -> B", variable=self.var_direction,
                                    value="a2b", width=8)
        rbtn_b2a = ttk.Radiobutton(self, text="B -> A", variable=self.var_direction,
                                    value="b2a", width=8)

        label_sliderValue.grid(row=0, column=0)
        self.slider.grid(row=0, column=0, pady=5, columnspan=5)
        btn_startProgramming.grid(row=1, column=0, pady=5, columnspan=5)
        btn_setA.grid(row=2, column=0, pady=5, columnspan=5)
        btn_setB.grid(row=3, column=0, pady=5, columnspan=5)
        label_timelapseDuration.grid(row=4, column=0, padx=10, pady=5, sticky="e")
        self.entry_timelapseDuration.grid(row=4, column=1, sticky="w")
        label_fps.grid(row=5, column=0, padx=10, pady=5, sticky="e")
        self.combo_fps.grid(row=5, column=1, sticky="w")
        label_videoDuration.grid(row=6, column=0, padx=10, pady=5, sticky="e")
        self.entry_videoDuration.grid(row=6, column=1, sticky="w")
        label_shutterSpeed.grid(row=7, column=0, padx=10, pady=5, sticky="e")
        self.combo_shutterSpeed.grid(row=7, column=1, sticky="w")
        label_totalFrames.grid(row=8, column=0, padx=10, pady=5, sticky="e")
        label_numberOfTotalFrame.grid(row=8, column=1, sticky="w")
        rbtn_a2b.grid(row=9, column=0, pady=5)
        rbtn_b2a.grid(row=9, column=1, pady=5)
        btn_setActionData.grid(row=10, column=0, pady=5, columnspan=5)
        btn_finishProgramming.grid(row=11, column=0, pady=5, columnspan=5)
        btn_gotoOrigin.grid(row=12, column=0, pady=5, columnspan=5)
        btn_startAction.grid(row=13, column=0, pady=5, columnspan=5)

        self.running = True
        self.runners = [self.tk_loop()]

    async def tk_loop(self):
        while self.running:
            self.update()
            await asyncio.sleep(0.05)

    def setZero(self, event):
        self.slider.set(0)

    def assign_next_msg(self, idx):
        print("assign_next_msg:", idx)
        self.next_msg = messageSeq[idx]

    def setActionData(self):
        data = messageSeq[3]
        data["data"]["capture"]["record_duration"] = int(self.var_timelapseDuration.get())
        data["data"]["capture"]["video_duration"] = int(self.var_videoDuration.get())
        data["data"]["capture"]["fps"] = int(self.var_fps.get())
        data["data"]["capture"]["shutter_speed"] = str(self.var_shutterSpeed.get()).replace("/", "_")
        data["data"]["motor"]["direction"] = str(self.var_direction.get())
        print(data)
        self.next_msg = data
        #self.assign_next_msg(3)

    def getTimelapseDuration(self, event):
        td = int(self.var_timelapseDuration.get())
        print(td)

    def getVideoDuration(self, event):
        #vd = int(self.var_videoDuration.get())
        #print(vd)
        self.updateTotalFrame()

    def fpsSelected(self, event):
        #fps = int(event.widget.get())
        #print(fps)
        self.updateTotalFrame()

    def shutterSpeedSelected(self, event):
        shutterSpeed = event.widget.get()
        print(shutterSpeed)

    def updateTotalFrame(self):
        vd = int(self.var_videoDuration.get())
        fps = int(self.var_fps.get())
        self.totalFrame.set(vd*fps)

    def stop(self):
        self.running = False

    async def run(self):
        await asyncio.gather(*self.runners)


class App(AsyncTk):

    def __init__(self):
        super().__init__()
        #self.create_interface()
        if(ONLINE):
            self.runners.append(self.uart_terminal())

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