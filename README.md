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