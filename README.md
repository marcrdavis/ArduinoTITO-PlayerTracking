# Arduino TITO and Player Tracking
A homebrew slot machine TITO, player tracking and display project
By Marc Davis (10/13/2024) 

  Project goals: To allow home slot machine owners the ability to add Ticket In/Ticket Out (TITO), 
  Remote Control, Monitoring and Player Tracking (Display/Keypad/Reader) to their SAS-Compatible 
  games using an Arduino Mega 2560 and reusing existing player tracking hardware
  
  The project now includes sketches for TITO-only hardware based on the Arduino UNO and compatible 
  with the BETTORSlots TITO and TITO Deluxe hardware. Please see the included documentation for
  more details.

  NEW! NOVEMBER 22, 2025
  - New Web Management Console for the Deluxe sketches. This will eventually be ported to the Mega sketches

  Version 3.0 Build 20241013DR4
  - Adds support for the Arduino UNO R4 WiFi Board - see Arduino TITO Deluxe - UNO R4 WiFi Project - Documentation.pdf
    for additional details and instructions
  - This implements the TITO Deluxe functionality on the UNO R4 WiFi
  - Thanks to j.DeLutis and vashadow for their invaluable assistance in working out the 9-bit serial on the R4
  
  Version 3.0 Build 20240123
  - Fixes an issue where the getMacAddress function was returning identical MAC addresses; you
    will need to manually set the MAC address in code before uploading sketch

  Version 3.0 Build 20240117
  - Consolidated the RFID and MAG sketches into one sketch to reduce what I need to maintain
  - Adds 'Show Config' option to web UI and Game Manager to display the Arduino config.txt file
  - Removes Tournament Mode; this was not very stable and not freqently used based on feedback. If
    you want to continue using Tournament mode then stay on the version 2.0 sketch
  - Updates to the Game Manager code; removal of Tournament Mode controls and code cleanup
  - Updates to the Player Tracking Server
  - The V2 sketches will remain for anyone needing that code; will eventually be retired. No additional
    updates will be occurring to the V2 branch.
  - Updated documentation

  Version 2.0 Build 20240107

  - Adds Jackpot Reset to the Deluxe sketch and webUI

  Build 20230802

  - Fixes an issue with the MEGA sketches where the SAS general poll was failing due to the wrong serial UART being updated
  - Fixes an issue where under certain conditions the AutoAddCredits feature would generate a SAS read error
  
  Build 20230717

  - Sorry! small bug in the RFID and MAG projects was keeping the TITO from working

    
  Arduino TITO Deluxe – Build 20230706

  The Deluxe board is based on the Arduino Uno; which only has 2K of usable RAM for variables. The previous versions were not entirely stable due to the memory requirements of the Ethernet and SD libraries. The lack of 
  garbage collection meant that repeated use of some variables would eventually deplete the available RAM and make the app crash. To fix this I have made the following changes to the project:

  This version removes the use of the SD card; the few required configuration variables can be set in the sketch prior to loading it onto the Uno for the first time. These variables would rarely need to be changed and 
  some can be altered during game-play by using the webUI

  With the removal of the SD library we were able to remove the SPI and iniFile libraries – freeing up considerable space

  IMPORTANT: You must remove the SD card from the device before use; with a card inserted the network will not start

  Since the SD card is no longer present the web interface has been moved from the device to a hosted site. This was done for several reasons:

      o	Performance
      o	Memory limitations of the Uno hardware 
      o	The need to free up code space for additional features

  Even though the web interface is hosted on the Internet it does not have access to your network. You can only manage machines on the same local network as the device you are connecting from. If you are still not 
  comfortable with that for any reason you can host the webpage yourself on a web server on your network – then simply change the ‘webUI’ property in the code to point to your server. The only file needed is the 
  index.html file, which is included in the package.

  To access the web interface you can either browse to the IP Address of your game or go to http://arduinotito.infinityfreeapp.com and enter the IP Address of the game in the space provided.

  -	The code has been completely refactored and valuable memory-saving space has been recovered
  -	You can now use the latest version of the Ethernet library; this has dramatically improved the reliability of the web server. (if you were using version 1.0.4 previously please upgrade your library)
  - This version fixes an issue where the ticket info text was not being url-decoded before being sent to the game
  -	This version adds the option to remotely reboot the Arduino from the webUI

  Build 20230706 Updates

  - Updates to the MEGA Sketches
  - Updated SAS protocol implementation based on work done to optimize it for the Arduino Uno
  - Fixes a bug which under certain conditions could cause the player comps could go negative

  Build 20230328 Updates
  
  - Updates to the TITO and TITO Deluxe Sketches only
  - Fixes a bug in the ChangeToCredits option in the WebUI - sorry!
  
  Build 20230325 Updates
  
  - Updates to the TITO and TITO Deluxe Sketches only
  - Refactored code to remove usage of String variables and to free up additional memory on the Uno hardware
  - Restores the config.txt file to the TITO Deluxe sketch for configuration of basic parameters
  - Adds additional remote control options to the Deluxe sketch; including Bill Validator controls and Change to Credits
  - The code optimizations in the Deluxe version will eventually be ported back into the main project
  
  Build 20221023 Updates
  
  THERE ARE CHANGES TO THE CONFIG.TXT FILE IN THIS BUILD! PLEASE UPDATE YOUR CONFIG.TXT FILE AS PER THE DOCUMENTATION
  
  - Adds support for ESP8266 WiFi via the Songhe Mega2560 + WiFi R3 board; other ESP8266 implementations may work as well
  - Adds support for the Futaba NA202SD08FA display using the included FutabaVFD library
  - Adds support for the XS Technologies PI70-120-TLA-DFR magnetic card reader using the included MagStripeSerial library
  - Updates to the MagStripe library; if you are using a MagStripe reader you must update your library with the included newer version or the sketch will not compile
  - Updated documentation with wiring for new supported hardware
  - Github package cleaned up and reorganized
  
  Build 20220726 Updates
  
  - Updates to MEGA Sketches only
  - Fixes an issue where comp credits could go negative
    
  Build 20220617 Updates
  
  - Updates to MEGA Sketches only
  - Adds 'autoAddCredits' feature which will automatically add the number of credits specified in 'changeCredits'
    to the game when the credit meter is less than or equal to the 'creditFloor' value in the config file. To prevent
    excessive polling the check only runs after the attract message resets; so it could take up to 30 seconds for the
    credits to be added to the game

  Build 20220607 Updates
  
  - Updates to TITO and TITO Deluxe sketches only; due to memory issues in the Deluxe sketch I have had to remove
    the config.txt file support; the settings need to be applied to the sketch directly before downloading to 
    the UNO. This will increase stability of the Deluxe sketch.
  - Minor updates to the SAS portion of the code

  Build 20211112 Updates
  
  - Fixes an issue with DHCP renewal
 
  Build 20211031 Updates
  
  - Fixes an issue with the Update Ticket Text function where the 2nd address line would not be saved correctly
  - Adds new sketches: Arduino TITO and TITO Deluxe; a TITO only Arduino Uno solution compatible with BETTORSlots hardware

  Build 20210818 Updates
  
  - Added option on WebUI to set/change the comp credit value on the inserted player card
  - Restored the Handpay Reset option to the WebUI
  - Fixed the issue which was preventing the index.htm file from being updated by Game Manager
  - Fixed an issue where under certain conditions comp credits would be removed from the card before being successfully applied to game
  
  Build 20210814 Updates
  
  - WebUI updated to include Reboot Arduino option; replaces Jackpot Reset button on UI
  - Adds IP Address and Version information by pressing 0 on the keypad when in Attract mode
  - Minor code changes
  
  EXPERIMENTAL BUILD 20210522C
  
  Included in this package is an experimental version of the MAGSTRIPE sketch that includes support for the ArduCam OV2640_MINI_2MP_PLUS 
  Camera Module for the purpose of implementing a 'Jackpot Winner' Photo. Admins can set a minimum jackpot amount at which a photo will
  be taken of the player. The photo can be seen or downloaded using the Web UI. This is an experimental feature and is NOT RECOMMENDED 
  for novice users as it requires modification of the cabinet or player tracking bracket to accommodate the camera. This feature is also 
  only compatible with the MAGSTRIPE version due to limits in the SPI bus. Please see the Addendum PDF in the download for more information. 
  I make no promises that this code will make it into the main branch.
  
   Build 20210425 Updates
  
  - Adds support for 8-bit LCDs
  - Adds support for the Noritake CU20025ECPB-U1J display using the LiquidCrystal library in 8 bit mode
  - Updates to documentation including the make/model # of the supported magnetic reader
 
   Build 20210325 Updates
  
  - This build corrects many (if not all) of the compiler warnings generated by the code during compile; no other functional changes in this version
 
   Build 20210324 Updates
  
  - Fixes a potential issue in the setupPlayerMessage function where some pseudo-code was left in a conditional statement 
  - Fixes an issue in the MAG version where some debug code was left in a conditional statement when testing if SAS was available
  - Cleans up the waitForResponse function by removing the deprecated byte return value 

   Build 20210316 Updates
  
  - Fixes a bug in 20210314 where the web UI would not load properly - sorry about that!

   Build 20210314 Updates
  
  - Adds support for Magnetic Card Readers with a modified version of the MagStripe library to support insert-type readers
  - Improved documentation

   Build 20210312 Updates
  
  - Fixes a bug where the Update Player Name function on the Web UI was not working properly
  - Game Manager: This version removes the ability to remotely update the index.htm file due to 
    a problem with the file being saved incorrectly by the Arduino - probably due to a buffer size
    issue; Will revisit this later. The config.txt file can still be updated from Game Manager.
   
   Build 20210306 Updates
  
  - Fixes a bug where the Player Total Winnings would not be calculated correctly
  - Fixes an error in the documentation regarding the Serial Tx/Rx pinouts

   Build 20210222 Updates
  
  - Adds support for updating SD card files (config.txt and index.htm) remotely using Game Manager; this feature is experimental
    and may be removed in a future release if issues arise; see documentation for details
  - Adds support for rebooting of Arduino board from Game Manager
    
  Build 20210219 Updates
  
  - Fixes a bug in 20210216 where the board would not set the sasOnline flag to true which
    prevented the RFID reader from working
  - Added a feature to enable periodic saving of player stats in case of loss of power to game
  - You will still need to apply the config.txt changes from the prior build to use this release
    
  Build 20210216 Updates
  * THERE ARE CHANGES TO THE CONFIG.TXT FILE IN THIS BUILD! PLEASE UPDATE YOUR CONFIG.TXT FILE
    AS PER THE DOCUMENTATION
    
  - Added support for Keypads, including Bally 6x2, 3x4 and ACT 8x2
  - Added support for the Admin Card; cardType 2
  - Added keypad-accessible Admin Controls
  - Added keypad-accessible Player Controls
  - Added support for Player Comps
  - Added support for Game Manager Tournament Mode
  - Added support for Time Sync with Game Manager host
  - Added additional messaging to display on game state changes
  - Fixed issue where player data would not be loaded if player card was left in machine at power-on
  - Fixed a few bugs in the SAS protocol implementation; also adds ACK/NACK responses to applicable commands
  - Package now includes Game Manager - a Windows App to manage your machines
  - Minor fixes to Web UI; you will need to replace the index.html file on your SD card(s) with the new one in the package
  - Updates and fixes to APTS card server; if using the card server you will need to update it and all of your machines at the same time because of
    message-format changes in this version
  - Significant code changes, refactoring and improvements
  - Improved documentation; now with pinouts for supported displays and keypads; Source code in-line comments also extensively updated
    
  Build 20210129 Updates
  * THERE ARE CHANGES TO THE CONFIG.TXT FILE IN THIS BUILD! PLEASE UPDATE YOUR CONFIG.TXT FILE
    AS PER THE DOCUMENTATION
  
  - Adds support for Noritake GU-7000 or equivalent VFDs with modified library
  - Adds support for pixel displays and new config properties for setting row and column sizes
  - Fixes a bug where the DisplayWidth property was not being read from the config.txt file
  - Improved documentation; includes pinouts for tested displays
  
  Build 20210125 Updates
  
  - Adds support for DataVision DV-16236 or equivalent LCDs
  - Fixes some messages so they will display properly on 16x2 displays
  - Fixed a typo in the config.txt sample file
  
  Build 20210111 Updates
  
  - Adds 2 retries to TITO operations to improve responsiveness and reliability
  
  Build 20210107 Updates
  
  - Fixes compatibility issues with Bally, WMS and Konami machines
  - Fixes an issue where the Handpay queue would not be flushed - causing repeating exception 51s
  - Fixes an issue with the Handpay Reset
  - Fixes a bug in the waitForResponse function where the first two bytes were set to zero
  - Fixes an issue where the onlyTITO option would not be set properly from the config file
  - Adds code to clear the comm buffer on start
  - Adds verbose logging for many common events, including TITO progress, validation and some system events
  - Code cleanup and other minor bugs fixed
  
  Build 20201222 Updates
  - Fixes issue where playerMessage was not being updated properly when a second different card was inserted
  - Adds HandpayReset to WebUI
  - Adds Update Player Name function to WebUI
  - Minor html markup fixes to WebUI
  
  Hardware requirements: 
    Arduino Mega 2560 R3/Songhe Mega2560 + WiFi R3/Arduino Uno; RFID RC 522 or compatible Magnetic Card Reader; 
    W5100 Ethernet Shield; Serial Port board; Compatible vacuum fluorescent display or LCD; if using a display
    other than the default LCD then modifications will be required - see inline comments; Compatible keypad; if
    using a keypad other than the default Bally 6x2/3x4 then modifications will be required - see inline comments; 
    Modifications will be required if using another type of ethernet shield

  Software requirements:
    If using an IEE, Futaba or Noritake VFD You will need my modified version of the libraries included in the zip file

  Upgrading from earlier versions:
    Be sure to check the sample config.txt file in the zip file for new or changed parameters that may be required
    for the new version

  For TITO Setup please follow the included documentation. This has been tested on IGT, Bally, WMS and Konami.

  Note: Remote access has been made compatible with BETTORSlots TITO apps for IOS/Android; BETTORSlots is
        not affiliated with this project and does not support or endorse using their apps for this purpose;
        This project does not use BETTORSlots code.

  This software is licensed to you under The Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 
  International license (https://creativecommons.org/licenses/by-nc-nd/4.0/legalcode).

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS” AND ANY EXPRESS
  OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
  AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
  IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
  OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

