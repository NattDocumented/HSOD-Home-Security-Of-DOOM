# H.S.O.D. : Home Security Of DOOM

A simple home security alarm with personality and a little twist ***wink*** ***wink***.



## What is it?

It is just like your simple, ordinary home security/alarm system. If you input your PIN to arm or disarm the alarm, and if you proceed to enter your PIN just in time, the alarm won't go off. However, when H.S.O.D went off... It doesn't just play a normal alarm... I will scare the burglar away (hopefully)... it plays a pre-recorded audio which you can customize/add custom audio to your liking, of how you would like to torment your burglar.

My personal option is DOOM ETERNAL MUSIC INTRO with + "so you are the chosen one."


## How do you use it?

It is very simple, here's how to use it!

### 1. Arming the Device
* **Action:** On boot, the system is disarmed. Input the PIN.
* **Outcome:** The current state of the device will change from unarmed to armed, and the OLED display and an audio cue from the speaker will indicate that!

### 2. INTRUDER ALERT!!!
* **Action:** Human detected.
* **Outcome:** The device enters a warning state or a countdown state, to be exact. The OLED display and a countdown coming from the speaker tracking your remaining time will also indicate that!

### 3. Disarming the Device
* **Action:** Input the PIN.
* **Outcome:**
  * **Correct PIN:** Everything resets and goes back to normal (back to the unarmed/disarmed state), the OLED display and the sound cue will also indicate it as well!
  * **Incorrect PIN:** Everything continues, but you can still go ahead and try again until the countdown reaches zero (but there's also a max attempt)



## System Architecture

```mermaid

graph TD;
  %%Define styles
  classDef controller fill:
  classDef sensor fill:
  classDef output fill:
  classDef cloud fill:

  %% Nodes
  Radar[HLK-LD2410C]:::sensor
  Mic[ICS-43434]:::sensor
  Keypad[Keypad]:::sensor

  MCU[Arduino Nano ESP32]:::controller

  OLED[OLED Display]:::output
  DFPlayer[DFPlayer Mini + Speaker]:::output

  Wit[Wit.ai Speech API]:::cloud

  %% Connections
  Radar --> |UART2 / Pin D11 Trigger| MCU
  Mic --> | I2S Mono PCM via DMA | MCU
  Keypad --> |Digital I/O Pins 2-8| MCU

  MCU --> |Hardware I2C| OLED
  MCU --> |Hardware UART1| DFPlayer
  MCU -.-> |Chunked HTTPS POST| Wit

```



## State Machine

```mermaid

stateDiagram-v2
  [*] --> STATE_DISARMED : Boot

  STATE_DISARMED --> STATE_ARMED : Enter PIN + ENT

  STATE_ARMED --> STATE_COUNTDOWN : Human Detected

  STATE_COUNTDOWN --> STATE_DISARMED : Correct PIN / Voice Phrase <br> Entered 
  STATE_COUNTDOWN --> STATE_ALARM : Timer Hits 0

  STATE_ALARM --> STATE_DISARMED : Correct PIN / Voice Phrase <br> Entered 
  STATE_ALARM --> STATE_LOCKED_OUT : Too Many Failed Attempts

  STATE_LOCKED_OUT --> STATE_ARMED  

```



## Why did I make it?

Here's a little back story. I passed an entrance exam to one of the best high schools in Thailand (Triam Udom Suksa School, btw ***wink*** ***wink***), which means I have to move away and live alone in the big city, but, being a country boy, I came up with an idea. Wouldn't it be so fitting to add some security measures to my apartment because I'm living alone? Normal Securities are boring, corporate, no personality, there's nothing interesting, so I came up with something new of my own! I came up with this!



## Step-By-Step Build Guide.

### Tools Required
* **Soldering Iron & Solder Wire** 
* **Wire Flush Cutters**
* **Tweezers**


---

### [1] : Prep Your MicroSD Card

1. Insert your **MicroSD card** into your computer and format it to **FAT32**.
2. Drop your custom audio files inside (`0001.mp3`, `0002.mp3`, etc.)
   
   *  `0001.mp3` = Warning beep
   *  `0002.mp3` = Full alarm audio
   *  `0003.mp3` = Armed sound
   *  `0004.mp3` = Disarmed sound
   *  `0005.mp3` = Incorrect sound
   *  `0006.mp3` = Locked out sound
   *  `0007.mp3` = Startup sound

3. Eject the card and insert it right into the slot of the **DFPlayer Mini (U3)**.


### [2] : Soldering Components

This is fairly simple due to each component having its own designated spots on the board; simply find those and solder them in place.

**For Example/Demonstration:**
1. **Keypad:** Insert the 12 **Cherry MX Mechanical Switches (SW1 to SW12)** into the **big key grid squares** on the board. If you can't see it, I'm going to assume you're blind (I'm just playing with ya lol), flip it over, and solder, **simple!**
2. **Modules:** as stated, the OLED Display, Arduino Nano ESP32, HLK-LD2410C, and all the components and modules all have their own slot, just insert them into their own spot, **flip 'em over, and solder!** **simple!**

I think you get it now, simple! Isn't it?
All you have to do is do the same thing for every component of this device.

**Tips, but kind of necessarily:** After soldering everything, use your **Wire Flush Cutters** to cut the excess leads or pins off.


### [3] : Uploading Firmware (PlatformIO)

This project is built using **PlatformIO** which is inside of VS Code instead of the standard Arduino IDE. Here's how to flash the firmware:

#### 1. Clone the Project

```bash
git clone https://github.com/NattDocumented/HSOD-Home-Security-Of-DOOM.git
```

#### 2. Open in VS Code
1. Launch **Visual Studio Code**.
2. Make sure you have the **PlatformIO IDE** extension installed; if not, go ahead and install it.
3. Click `File` > `Open Folder...` from the top menu, navigate to the folder you just cloned, and click **Open**. (Make sure you open the root folder containing the `platformio.ini` file)

#### 3. Making Voice Unlock Works

1. Login to [Wit.ai](https://wit.ai) with your Meta (a.k.a. Facebook) account.
2. If done correctly, it should lead you to the dashboard/apps, if not [click this](https://wit.ai/apps).
3. Click **+ New App** to create a new app **(obviously)**. You can name it whatever you please, but I personally just named it `"hsod"`.
4. Go into your newly created app. You'll see a sidebar. Click on **Management**, and go to **Settings**.
5. Copy your **Client Access Token** and replace these line in the firmware's code.
   
    ```cpp
    const char* WIFI_SSID = "YOUR_WIFI_SSID"; // Your WiFi's SSID
    const char* WIFI_PASS = "YOUR_WIFI_PASSWORD"; // Your WiFi Password here
    const char* WIT_TOKEN = "YOUR_WIT_AI_TOKEN"; // Your Wit.ai's token here
    ```


#### 4. Build and Upload
1. Connect a USB-C data cable from your computer into your Arduino Nano ESP32 port.
2. Click the **PlatformIO Alien Icon** on the left menu bar.
3. Under the Project Tasks menu, click **Build** to compile the codebase.
4. Once the terminal displays a green `[SUCCESS]` message, click **Upload** right below it to flash the firmware onto your board.

#### 5. Done
As soon as the upload completes, the OLED screen will wake up, and the speaker will play the system startup sound.

### [4] : Casing / Enclosure

After finishing the other steps, I think this one is the easiest, so don't be tired yet, lmao.

1. **Have your 3D printed casing / Enclosure ready:** if you haven't done that, go ahead and print it, no rush, because this README.md can't run away from you lmao.
2. **Assemble:** This is easy. There should be 2 parts: the base and the lid. Since there are poles on the base and holes on the PCB for assembly, simply insert the PCB into the enclosure while making sure the USB of the Arduino Nano ESP32 is right where the hole for the USB on the casing is. After, just put the base and lids together while making sure all the components that should be visible are visible. Both the base and the lid have sockets for connecting to each other.
3. **Done!**

#### You can now enjoy this amazing device. Hope you have a great experience. Any feedback is appreciated.

## Gallery

### Schematics

![alt text](<resources/media/Screenshot 2026-06-10 192336.png>)
![alt text](<resources/media/Screenshot 2026-06-10 192353.png>)

### PCBs

![alt text](<resources/media/Screenshot 2026-06-10 191553.png>)
![alt text](resources/media/HSOD_PCB_KICAD.png)
![alt text](resources/media/HSOD_PCB_KICAD_NATT.png)

### Casing / Enclosure Design

![alt text](resources/media/84aaa23a-3642-4df2-9ff4-97746b07a7e6.PNG)
![alt text](resources/media/HSOD3DMODELCASING_2026-Jun-14_05-41-10AM-000_CustomizedView24308382313.png)
![alt text](resources/media/HSOD3DMODELCASING_2026-Jun-14_05-24-45AM-000_CustomizedView1489484771.png)
![alt text](<resources/media/test render stl.png>)