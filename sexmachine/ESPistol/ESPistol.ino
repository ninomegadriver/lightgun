#pragma GCC optimize ("O3")

/*
 *   SEXMACHINE's ESPistol
 *   
 *   This firmware is part of "SEXMACHINE", a classic arcade lightGun implementation.
 *   Released under the termos of GPL v3.0 License.
 *   
 *   For extended info, please visit:
 *   https://github.com/ninomegadriver/lightgun/sexmachine *   
 *   
 *   Copyright (C) 2022 Antonio Tornisielo (A.K.A Nino MegaDriver)
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *   
 *   
*/

// Timing Variables
long vsyncStart=0, hsyncStart = 0, deBounce = 0, lineDuration;

int  line          =  0;        // Holds the current scanline being drawn
int  hitLine       = -1;        // Holds the scanline number when the optical hit ocurred
int  vsyncPin      = 26;        // VSync Pin
int  hsyncPin      = 25;        // Hsync Pin
int  gun1Pin       = 27;        // Optical Sensor from lighGun1
int  gun2Pin       = 33;        // Optical Sensor from lighGun1
int  activeGun     = 0;         // Gun to look for Optical Hit
long debaunceLimit = 17000;     // Time to deBounce Interrupts

// Triggered when a VSync pulse occurs
void IRAM_ATTR VSYNC()
{
   vsyncStart = micros(); // Write down the moment we've started
   line=0;                // Reset the line number to zero
}

// Triggered when a HSync pulse occurs
void IRAM_ATTR HSYNC() {
   hsyncStart = micros(); // Write down the moment we've started
   line++;                // Increment our scanline number
}

// Trigerred when the gun1 annouces a optical hit
void IRAM_ATTR GUN1(){
  long hit=micros();                   // Save the hit moment in time
  if(hit - deBounce > debaunceLimit && activeGun == 1){     // Do some debouncing
    deBounce = hit;
    if(hit>vsyncStart){           // If we're still in the same frame, notify the software via Serial
      char msg[100];
      // Informs the duration of the line, the moment the hit occurred and the current scan line
      sprintf(msg, "=%010ld|%010ld|%003d|%003d!", lineDuration, hit-hsyncStart, line, activeGun);
      Serial.print(msg);
    }
  }
}

// Trigerred when the gun2 annouces a optical hit
void IRAM_ATTR GUN2(){
  long hit=micros();                   // Save the hit moment in time
  if(hit - deBounce > debaunceLimit && activeGun == 2){     // Do some debouncing
    deBounce = hit;
    if(hit>vsyncStart){           // If we're still in the same frame, notify the software via Serial
      char msg[100];
      // Informs the duration of the line, the moment the hit occurred and the current scan line
      sprintf(msg, "=%010ld|%010ld|%003d|%003d!", lineDuration, hit-hsyncStart, line, activeGun);
      Serial.print(msg);
    }
  }
}

void setup() {

  // If we're powering up the whole system at once
  // wait a little for the video to start on the
  // host PI/PC - Adjust this according to your
  // boot time  
  delay(3000);
  
  
  Serial.begin(115200);

  // All pins set to input
  pinMode(vsyncPin,    INPUT);
  pinMode(hsyncPin,    INPUT);
  pinMode(gun1Pin,     INPUT);
  pinMode(gun2Pin,     INPUT);

  // configure the interrupts
  attachInterrupt(digitalPinToInterrupt(vsyncPin),       VSYNC, FALLING);
  attachInterrupt(digitalPinToInterrupt(hsyncPin),       HSYNC, FALLING);
  attachInterrupt(digitalPinToInterrupt(gun1Pin),         GUN1, FALLING);
  attachInterrupt(digitalPinToInterrupt(gun2Pin),         GUN2, FALLING);

  // Calculate the duration of a scanline
  lineDuration = pulseIn(hsyncPin, HIGH) + pulseIn(hsyncPin, LOW);
  
}

void loop() {
  while(Serial.available()){
    uint8_t r = Serial.read();
    if(r == 0x01) activeGun = 1;
    else activeGun = 2;
  }
}
