# DCC++-ArduinoThrottle
This is the code for a hardware throttle for the DCC++ system. It was initially based on the work by D. Bodnar (http://trainelectronics.com/DCC_Arduino/DCC++/Throttle/).

Changes from the original:
  1. The code has almost entirely been re-written into my prefered style.
  2. The screen is now a 20x4 LCD.
  3. The keypad is now 4x4 (0-9, A-D, *, #).
  4. The potentiometer for controlling the speed does not control the direction (letter - D).
  5. If using a motorised linear potentiometer (via an L298N) it will set the potentiometer to the currect value for the loco at switch.
  6. Fuctions 0-28 are all now implemented.
  7. A roster file can be used to store loco names with associated addresses for the display.
  8. Improved potentiometer debounce.

The functions are:
  * A - Toggle track power (off at startup).
  * B - Select a loco to active location. Type number, end with B (C cancels).
  * C - Cancel loco selection.
  * D - Toggle active loco direction.
  * \# - Switch between active locos.
  * 0-9 - Toggle function on active loco.
  * \* - Once for functions 10-19, twice for 20-28, thrice returns to 0-9. Then press number key for function.
  * Button 1 - Also selects a loco to active location.
  * Button 2 - Also switches between active locos.
