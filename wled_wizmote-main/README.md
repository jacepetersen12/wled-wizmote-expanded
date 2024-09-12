# WizMote custom firmware (Supports up to 6 button presses for buttons 1-4)
This firmware expands the wizmotes capablities to 33 button outputs by enabling the user to single press, double press, etc up to 6 presses + long press on buttons 1-4. 

Note that the first press wakes the hardware, so the button needs to be pressed long enough to be read. (I usually just press the brightness up button to wake up the remote, and then proceed with my desired button presses while it's awake)

This firmware will NOT work with wiz remotes imported by REV Ritter GmbH which use a PCA6416A to read button presses.

Don't judge the coding too harshly. About 90% was written by Chat-GPT

The remote can be flashed by accessing the header pins. I used a sparkfun serial basic device to interface. The unmarked pin on the remote is 3.3v. Connect IO0 to ground on boot to enable programming the device.

Credit to zango-me, who published a custom firmware that I adapted to work with the wizmotes that use an HC165AG shift register to read button presses.
https://github.com/zango-me/wled_wizmote

Note: This will break the current button behavior from within WLED. You'll need to modify the firmware in the remote.cpp file. I'm working on a usermod that will allow presets to be mapped to each button press from within settings.


Button Mappings:

{single, double, triple, quadruple, quintuple, hextuple, long} //Button Name

{1, 0, 0, 0, 0, 0, 0},          Off Button (immediately returns)
{3, 0, 0, 0, 0, 0, 0},          On Button (immediately returns)
{5, 6, 7, 8, 9, 10, 11},        Button 2 on the Wiz Remote
{12, 13, 14, 15, 16, 17, 18},   Button 1 on the Wiz Remote
{19, 20, 21, 22, 23, 24, 25},   Button 4 on the Wiz Remote
{26, 27, 28, 29, 30, 31, 32},   Button 3 on the Wiz Remote
{33, 0, 0, 0, 0, 0, 0},         Brightness Up (immediately returns)
{35, 0, 0, 0, 0, 0, 0},         Brightness Down (immediately returns)
{37, 0, 0, 0, 0, 0, 0}          Sleep Button (immediately returns)