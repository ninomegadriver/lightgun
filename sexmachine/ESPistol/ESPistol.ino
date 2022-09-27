#pragma GCC optimize ("O3")

/*
 *   SEXMACHINE's ESPistol
 *   
 *   This firmware is part of "SEXMACHINE", a classic arcade lightGun implementation.
 *   Released under the termos of GPL v3.0 License.
 *   
 *   For extended info, documentation and diagram, please visit:
 *   https://github.com/ninomegadriver/lightgun/sexmachine
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

// Timing Variagles
long vsyncStart=0, hsyncStart = 0, hit = 0, deBounce = 0, lineDuration, lastRefresh;

int line       =0;           // Holds the current scanline being drawn
int hitLine    = -1;         // Holds the scanline number when the optical hit ocurred
int hsyncPin   = 25;         // Hsync Pin
int vsyncPin   = 26;         // VSync Pin
int gunPin     = 27;         // Optical Sensor from lighGun
int refresh    = 1000;       // Refresh interval in miliseconds

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

// Trigerred when the gun annouces a optical hit
void IRAM_ATTR GUN(){
  hit=micros();                   // Save the hit moment in time
  if(hit - deBounce > 17000){     // Do some debouncing
    deBounce = hit;
    if(hit>vsyncStart){           // If we're still in the same frame, notify the software via Serial
      char msg[100];
      // Informs the duration of the line, the moment the hit occurred and the current scan line
      sprintf(msg, "=%010ld|%010ld|%003d!", lineDuration, hit-hsyncStart, line);
      Serial.print(msg);
    }
  }
}

void setup() {
  Serial.begin(115200);

  // All pins set to input
  pinMode(vsyncPin, INPUT);
  pinMode(hsyncPin, INPUT);
  pinMode(gunPin, INPUT);

  // configure the interrupts
  attachInterrupt(digitalPinToInterrupt(vsyncPin), VSYNC, FALLING);
  attachInterrupt(digitalPinToInterrupt(hsyncPin), HSYNC, FALLING);
  attachInterrupt(digitalPinToInterrupt(gunPin), GUN, FALLING);

  // Calculate the duration of a scanline
  lineDuration = pulseIn(hsyncPin, HIGH) + pulseIn(hsyncPin, LOW);
  
}

void loop() {
  // On a time basis, update the scanline duration in case
  // a resolution change happened  
  if(millis() - lastRefresh > refresh){
    lastRefresh  = millis();
    lineDuration = pulseIn(hsyncPin, HIGH) + pulseIn(hsyncPin, LOW);
  } 
}
