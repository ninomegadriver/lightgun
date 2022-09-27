## Sexmachine's Advance Mame  

This is a modified version of Advance Mame v3.9 to implement the original classic Arcade lightGun using the "Sexmachine Setup". ***This version is intended for use only with the Raspberry PI OS bullseye or superior, it doesn't work on Windows platforms or any other Linuxes ***

You can find all the modifications done by doing a 'grep' in the main folder:  
``` grep SEXMACHINE * -R 2>/dev/null ```  

## Live Demo  
[![Watch the video](https://img.youtube.com/vi/usHYl3YvgNg/maxresdefault.jpg)](https://youtu.be/usHYl3YvgNg)  

## Instructions:  
Download and burn a PI OS bullseye ***LITE version*** image into a SDCard, boot and configure it on your Raspberry PI. For this technique to work, the emulator must have full control of the Framebuffer so ***it won't work inside X***. There's no reason to use a graphical version of PI OS as you'll need to call the emulator from a plain text shell console.  

Follow the [***Sexmachine's circuit diagram***](https://github.com/nino/lightgun/sexmachine) for connecting the Raspberry PI to the Arcade Monitor and the lightGun. Then, use the provided [***"config.txt"***](https://github.com/nino/lightgun/sexmachine/config.txt) to boot the Pi into the correct resolution.

Afterwards, you may install the emulator using the provided .deb package:  

```sudo dpkg -i sexmachine-rpi4-0.1a.deb```  

This package will install the "sexmachine" binary file into /usr/bin, along with the WiringPI Dev Library into /usr/local/lib.  

Before running it, you must first connect the ESP32 to your Raspberry PI using a traditional USB cable. You can also boot the PI with the ESP32 already connected as it will also work as expected. Then, you may use the emulator like the original advmame, i.e:  

``` sexmachine opwolf ```  

## Building from sources  
First, enter the WiringPi folder, build and install it:  
```
cd WiringPi;     
./build;   
sudo ldconfig;   
```  

Then, use the provided 'cfg.sh' to build the midified Advance Mame. This will configure and build the emulator with all the needed features and optimization flags:  
```
./cfg.sh
```  

If building succeeds, just run it as usual. However, I recommend you running it as root:  
```  
sudo ./advmame opwolf  
```  


*************************************************************************  
AdvanceMAME - Copyright (C) 1999-2018 by Andrea Mazzoleni  
MAME - Copyright (C) 1997-2003 by Nicola Salmoria and the MAME Team  
SEXMACHINE - Copyright (C) 2022 by Antonio Tornisiello  
