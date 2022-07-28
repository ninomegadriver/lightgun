/*
 * ESP32Lightgun
 * Nino MegaDriver
 * nino@nino.com.br
 * http://www.megadriver.com.br
 * License: GPL v3.0
 * 
 * Implementation of the classic and most common arcade
 * lightgun on an ESP32.
 * 
 * This sketch uses FabGL for video generation.
 * Please check https://github.com/fdivitto/FabGL
 * for license and VGA/Rgb wiring information.
 * 
 */

#pragma GCC optimize ("O3")

// Install FabGL from
// https://github.com/fdivitto/FabGL
#include <fabgl.h>

// IMPORTANT: The lighgun must be powered over 5v and that also means:
//             that: YOU MUST USE A LOGIC CONVERTER OVER THESE PINS!
uint8_t TRIGGER_PIN = 26; // Trigger pin from the lightgun
uint8_t OPTICAL_PIN = 25; // Optical pin from the lightgun


uint8_t triggerHalt = 0; // Debounce variable for the trigger pin

// Main video object
fabgl::VGADirectController DisplayController;

// Some predefined colors
uint8_t black, white, red, green;

// Screen tracking variables
int width, height, curLine = -1, hitLine=-1, hitX=-1, curTrackPos=0;

// Optical tracking variables
int track=0, track_pos=0, track_width=10, track_step=0, track_timeout = 0, fired=0;

// Interrupt for the gun's phototransistor
// When it detects a strong bright light, the line will
// pull down, otherwise it stays high
void IRAM_ATTR OpticalInterrupt() {
  // To avoid bouncing, remove the interrupt.
  detachInterrupt(digitalPinToInterrupt(OPTICAL_PIN));
  hitLine = curLine;  // Saves the detected "y" coordinate
  hitX = curTrackPos; // Saves the detected "x" coordinate
}

/*
 * Scanline callback - where magic is done!
 * 
 * CONCEPT:
 * When the shot trigger is presset, we start painting the
 * screen white. When the light reaches the Phototransistor
 * in the Lighgun it triggers the interrupt above.
 * 
 * So, to instantly grab the position on screen, we pass
 * internal values to global variables.
 * 
 * The I2S DMA buffer is filled line-by-line. So tracking the
 * "y" position on screen is easy.
 * 
 * However, for the "x" coordinate it gets a little trick as we can't
 * track the exact position using this method.
 * 
 * So, the solution is scanning the coordinate more than one pass.
 * Each pass splits the tracking space in half, and keep doing it
 * until we reach the appropriate gun position.
 * 
 */
void IRAM_ATTR drawScanline(void * arg, uint8_t * dest, int scanLine)
{

  curLine = scanLine; // Register globally in which line we're in

  // If the trigger has been fired, start the tracking routine
  if(scanLine == 0 && fired == 1) {
    fired = 2;
    track = 1;
    track_pos = 0;
    track_width = width/2;
    track_timeout=0;
    hitX = -1;
  }

  // Register globally in witch portion of the screen we're in
  curTrackPos = track_pos;

  // If we're tracking the shot, we start paiting the screen white
  if(track == 1){
    for(int x=0;x<width;x++){
      // Paints a vertical white strip on screen from left to right
      // On each pass, the strip will be cut in half..
      if(x <=track_pos+track_width && x>=track_pos) VGA_PIXELINROW(dest, x) = white;
      else VGA_PIXELINROW(dest, x) = black;
    }
  }else{ // We're not tracking this run, just draw a normal line...
    for(int x=0;x<width;x++){
      // Paint the boundaries of the screen green
      if(scanLine == 0 || scanLine == height -1 ) VGA_PIXELINROW(dest, x) = green; 
      else if(x==0 || x == width -1) VGA_PIXELINROW(dest, x) = green;
      // If we have a previous gun reading, show it in red
      else if((scanLine == hitLine || x == hitX) && hitLine > 0) VGA_PIXELINROW(dest, x) = red;
      // Otherwise just paint it black
      else VGA_PIXELINROW(dest, x) = black;
    }
  }

  // Ok, so we're tracking the screen and have reached the last scanline
  // on screen...
  if(scanLine == height-1 && track == 1) {
    if(hitLine >= 0){ // We've got a hit on this half of the screen, let's stop and start over!
      // We'll keep splitting the area in half until we're reach a mininum
      track_width /= 2;
      if(track_width >= 5){ // Minimun limit in pixels for a search area, denotates the "precision"
        // We're still tracking, so attach the interrupt again so we can detect the next optical hit
        attachInterrupt(digitalPinToInterrupt(OPTICAL_PIN), OpticalInterrupt, FALLING);
        track_pos = hitX;
        hitLine = -1;
      }else{
        // Ok, we're done!
        // Coordinates for the hit are stored on hitLine and hitX
        fired = 0;
        track_pos = 0;
        track = 0;
      }
    }else{
      // Ok so we don't have an optical hit just yet
      // let's keep searching for it!
      track_pos  += track_width;
      if(track_pos >= width) {
        fired = 0;
        track_pos = 0;
        track = 0;
        track_timeout=0;
      }
    }
  }

  // Moving the gun while the tracking is ongoing
  // can cause some odd behaviours, so implementing
  // a timeout rules it out.
  if(track == 1){
    track_timeout++;
    if(track_timeout > 4000){ // Abort & Reset, treat it as a miss...
      fired = 0;
      track_pos = 0;
      track = 0;
      hitLine = -1;
      hitX = -1;
    }
  }
  
}

// Interrupt for the lightun trigger
void IRAM_ATTR TriggerInterrupt() {
  if(triggerHalt == 0 && digitalRead(TRIGGER_PIN) == LOW){
      // Attach an interrupt for the Phototransistor
      attachInterrupt(digitalPinToInterrupt(OPTICAL_PIN), OpticalInterrupt, FALLING);
      fired=1;
      hitLine = -1;
      triggerHalt = millis();
  }
}


void setup(){

  // Video definitions
  DisplayController.setScanlinesPerCallBack(1);
  DisplayController.setDrawScanlineCallback(drawScanline);
  DisplayController.setResolution("\"320x240_60.00\" 6.00 320 336 360 400 240 243 247 252 -hsync -vsync");
  // Follow the wirings available at:
  // https://github.com/fdivitto/FabGL
  // This sketch is intended for arcade monitors, so just wire both HSync and VSync together and you'll be fine.
  
  width  = DisplayController.getScreenWidth();  // Register the screen width
  height = DisplayController.getScreenHeight(); // Register the screen height

  // Generate some pre-defined colors
  white = DisplayController.createRawPixel(RGB222(255, 255, 255));
  black = DisplayController.createRawPixel(RGB222(0, 0,0 ));
  red   = DisplayController.createRawPixel(RGB222(255, 0,0 ));
  green = DisplayController.createRawPixel(RGB222(0, 255,0 ));

  // Both lighgun input lines
  pinMode(TRIGGER_PIN, INPUT);
  pinMode(OPTICAL_PIN, INPUT);

  // Attach the interrupt for the lightgun trigger
  attachInterrupt(digitalPinToInterrupt(TRIGGER_PIN), TriggerInterrupt, FALLING);

}


void loop(){

  // Simple debounce for the lightgun trigger button
  if(triggerHalt > 0 && millis() - triggerHalt > 1000){
     triggerHalt = 0;
  }

}
