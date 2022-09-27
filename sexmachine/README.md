# SEXMACHINE  
"SEXMACHNE" is a implementation of the Classic Arcade LightGun for playing games on a Raspberry PI connected to a legacy arcade monitor, using a modified Advance Mame Emulator.  

All documentation, files, software, firmware and hardware are provided under the terms of the GPL v3.0 license.

This setup consists basicaly in three modules:  
- A Raspberry PI outputting a VGA signal via VGA666 interface and monitoring the LightGun trigger connected to a GPIO;  
- An ESP32, with the help of a Hex Inverter IC, monitoring the HSync and VSync pulses along with the LightGun phototransistor's optical signal; and  
- A modified version of Advance Mame for running the game and putting it all together.  

## The Concept  
The concept behind this setup is the same used on most of the LightGun arcade games, such as Operation Wolf, Lethal Enforcers, Area 15 and others:  
- When the gun trigger is pressed, the game paints a whole white frame;
- The CRT monitor starts painting it from the top left of the corner, to right bottom corner;
- The game starts counting;
- Inside the LightGun there's a double convex lens converging the light focus to phototransistor; and
- When the light hits the phototransistor, the position is calculated according to the registered counter.  

However for a wider compatibility SEXMACHINE's setup is slightly different:
- The Sexmachine's Advance Mame monitors the gun trigger via GPIO;
- When the trigger is pressed, the emulator waits for a VSync, paints a white frame, then wait for VSync again.
- The ESP32 have Interrupts attached to the VSync and HSync video pulses, along with the optical sensor from the gun; and
- The ESP32 monitors every scanline and, in case of a hit, notifies the emulator via serial communication for it to translate the right gun shot position to the game.

## Circuit Diagram  
![Sexmachine Circuit's Diagram](https://raw.githubusercontent.com/ninomegadriver/lightgun/main/sexmachine/sexmachine_circuit_diagram.jpg)

## SEXMACHINE Jamma Board  
The circuit is quite simple and can be easily build on a breadboard. Furthermore, if you have a spare generic VGA666 hat, you can also use it. Just solder some extra wires to the terminals for VSync (GPIO2), HSync (GPIO3) and the gun trigger (GPIO27), and get the CSync for the arcade monitor on the Hex Inverter pins 4 and 8.  

However, I've desinged a board intended to be plug-and-play on any Jamma arcade cabinet. It was designed to ease the assembly process, by having all the components labeled in the silkscreen layer. Quite simple: order the pcb on PCBWay, order the part list and just solder everything in place until you have no holes left.

![Sexmachine Jamma Board](https://raw.githubusercontent.com/ninomegadriver/lightgun/main/Images/sexmachine_board.png)  


## Part List

|     Label       |                       Item                     |
|-----------------|:----------------------------------------------:|
| Arcade LightGun | Still manufactured and available by SUZOHAPP [here](https://na.suzohapp.com/products/optical_guns/96-2300-12)
| Raspberry PI    | Raspberry PI 4 (recommended) or 3B+                            |
| ESP32           | ESP32 Dev Kit V1 30 pins                       |
| Red2            | 16kΩ Resistor                                  |
| Red3            | 8.2kΩ Resistor                                 |
| Red4            | 4.3kΩ Resistor                                 |
| Red5            | 2kΩ Resistor                                   |
| Red6            | 1kΩ Resistor                                   |
| Red7            | 510Ω Resistor                                  |
| Green2          | 16kΩ Resistor                                  |
| Green3          | 8.2kΩ Resistor                                 |
| Green4          | 4.3kΩ Resistor                                 |
| Green5          | 2kΩ Resistor                                   |
| Green6          | 1kΩ Resistor                                   |
| Green7          | 510Ω Resistor                                  |
| Blue2           | 16kΩ Resistor                                  |
| Blue3           | 8.2kΩ Resistor                                 |
| Blue4           | 4.3kΩ Resistor                                 |
| Blue5           | 2kΩ Resistor                                   |
| Blue6           | 1kΩ Resistor                                   |
| Blue7           | 510Ω Resistor                                  |
| F04             | Hex Inverter, 74F04, 74LS04, 7404, CD4069      |
| Logic Converter | Generic 4 ports logic level converter module   |
| PAM8910         | Generic PAM8910 digital audio amplifier module |
| Headers         | Generic Male Headers                           |
| Audio Jack      | 3.5mm Audio Jack                               |
| Volume POT      | 5kΩ Potentiometer                              |
| Terminal Block  | 2 Pin Terminal Blocks for Speakers Out         |
| Power Out       | Female USB connector PCB mount "A" type        |

## Putting It All Together and Playing  

- Download a [PI OS Bullseye lite](https://www.raspberrypi.com/software/) image and burn it to a SDCard. There's no need to download a graphical version as, for this setup to work, you must run the emulator inside a pure text tty console. ***It wont't work inside X***;  
- Plug the SDCard into your Raspberry PI and finish the instalation and configuration. You can do this plugged into a LCD/LED monitor, there's no need to connect it just yet to the arcade monitor as the resolution will be really small and just make things harder;
- Download and install the [Sexmachine's Advance Mame](https://github.com/ninomegadriver/lightgun/tree/main/sexmachine/sexmachine_advancemame) from the [releases section](https://github.com/ninomegadriver/lightgun/releases)
- Copy the provided [config.txt](https://raw.githubusercontent.com/ninomegadriver/lightgun/main/sexmachine/config.txt) to your "/boot" partition so you may boot into the right configuration for the Arcade Monitor;
- Proceed to assemble the hardware setup. Either by doing it on a breadboard or, if you have the Jamma Board, mount it on top of the RPI's GPIO;
- Using the Arduino IDE upload the "ESPistol" sketch to your ESP32. If you're not familiar with this process, there's a [nice tutorial here](https://randomnerdtutorials.com/installing-the-esp32-board-in-arduino-ide-windows-instructions/);
- Connect all needed parts, the LightGun, the Arcade Monitor and the ESP32 to the RPI via a usual USB cable; and
- Use the emulator as you would with a normal Advance Mame:  
``` sexmachine opwolf ```  

## Live Footage  

Live Demo:  
[![Watch the video](https://img.youtube.com/vi/usHYl3YvgNg/maxresdefault.jpg)](https://youtu.be/usHYl3YvgNg)  

1cc (first loop) run:
[![Watch the video](https://img.youtube.com/vi/k8lXJYMKKos/maxresdefault.jpg)](https://youtu.be/k8lXJYMKKos)  

## Some Games Tested So Far...
| romset |  game | status | comments |
|--------|:-----:|:-------:|:--------:|
| opwolf | Operation Wold | perfect | |
| lethalen | Lethal Enforcers | perfect | This whole project started on a Lethal Enforcers original arcade PCB repair |
| lethalj | Lethal Justice | perfect | |
| le2 | Lethal Enforcers 2 | percect | |
| ghlpanic | Ghoul Panic | perfect | |
| area51 | Area 51 | playable| The lightGun works perfect, however the emulator doesn't perform well on this game. However with 0.75 frameskip it's quite playable |
| timecris | Time Crisis | bad | The game crashes shortly after it boots |
| duckhunt | Duck Hunt | playable | This game uses a different method of locating the gun. It works on this setup, but precision is a problem. However it should be quite simple to write a code just for it. Perhaps I'll do it in the future |

## Current Status And Limitations  
- Although the Jamma Board is already mapped to use the test/service, coins, start and buttons 1-3, I haven't coded the inputs just yet on the ESP32 and the Sexmachine's Advance Mame.
- The same goes for the player 2 LightGun. It's already mapped on the board, but I haven't coded it just yet.

I'll do so and plus test and fine tune more games in the near future...

*************************************************************************  
AdvanceMAME - Copyright (C) 1999-2018 by Andrea Mazzoleni  
MAME - Copyright (C) 1997-2003 by Nicola Salmoria and the MAME Team  
SEXMACHINE - Copyright (C) 2022 by Antonio Tornisiello  
