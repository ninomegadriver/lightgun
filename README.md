# Classic Arcade Lightgun PCB Breakout
Breakout and reproduction of the classic arcade lightgun board, manufactured by many different suppliers.  
  
## Actual Lightgun Picture
![Actual Lightgun](https://raw.githubusercontent.com/ninomegadriver/lightgun/main/Images/Lightgun.jpg)  
  
## Original PCB Pictures
![Original PCB Top](https://raw.githubusercontent.com/ninomegadriver/lightgun/main/Images/PCB-Top.jpg)  
![Original PCB Bottom](https://raw.githubusercontent.com/ninomegadriver/lightgun/main/Images/PCB-Bottom.jpg)
  
## Reproduced PCB Footprints  
![Reproduced Board Top](https://raw.githubusercontent.com/ninomegadriver/lightgun/main/Images/PCB-Footprint-Top.jpg)  
![Reproduced Board Bottom](https://raw.githubusercontent.com/ninomegadriver/lightgun/main/Images/PCB-Footprint-Bottom.jpg)  
  
## Breakout  
  
- Custom components reading availabe on the [Readings](https://github.com/ninomegadriver/lightgun/tree/main/Readings) folder.  
- Gerbers for the reproduced board [Here](https://github.com/ninomegadriver/lightgun/tree/main/Gerbers).  
- [Extra pictures](https://github.com/ninomegadriver/lightgun/tree/main/Gerbers) also provided.  
  
## Bill Of Materials  

| Part | Type               | Value        |
|------|--------------------|--------------|
|  C1  | Capacitor          | 100nf        |
|  C2  | Capacitor          | 50nf 503     |
|  C3  | Capacitor          | 10uf 16v     |
|  C4  | Capacitor          | 10uf 16v     |
|  C5  | Capacitor          | 10uf 16v     |
|  C6  | Capacitor          | 100nf        |
|  IC1 | Voltage Comparator | LM311N       |
|  S1  | Phototransistor    | bpw24r       |
|  L1  | Inductor           | 52R 5.8mH    |
|  R1  | Resistor           | 68k 5%       |
|  R2  | Resistor           | 50k 5%       |
|  R3  | Resistor           | 1k 5%        |
|  R4  | Resistor           | 1k 5%        |
|  R5  | Resistor           | 47R 5%       |
|  R6  | Resistor           | 4.6K 5%      |
|  R7  | Resistor           | 4.6K 5%      |
|  J1  | KK Connector       | 6 vias 2.54mm|
|  Q1  | Transistor         | 2N3994       |
| CR1  | Diode Rectifier    | 1N4148       |
  
## Hardware implementation on the ESP32
A proof-of-concept sketch for the ESP32 is also [available here](https://github.com/ninomegadriver/lightgun/blob/main/ESP32Lightgun/ESP32Lightgun.ino).  
  
[![ESP32 Lightgun Video](https://img.youtube.com/vi/4eyUXAVPKRU/0.jpg)](https://www.youtube.com/watch?v=4eyUXAVPKRU)  
  
