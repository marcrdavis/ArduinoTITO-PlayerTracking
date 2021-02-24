# Arduino TITO and Player Tracking
A homebrew slot machine TITO, player tracking and display project
By Marc Davis (2/22/2021)

  Project goals: To allow home slot machine owners the ability to use the player tracking
  display and card reader (with RFID modifications) which are normally non-functional
  outside of casinos. This replaces the Bally MasterCom device. The project can now
  communicate directly with SAS-enabled machines for remote control, TITO and metering.
 
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
    Arduino Mega 2560 R3; RFID RC 522; W5100 Ethernet Shield; Serial Port Shield;
    Compatible vacuum fluorescent display or LCD; if using a display other than the default LCD then
    modifications will be required - see inline comments; Compatible keypad; if using a keypad other than
    the default Bally 6x2/3x4 then modifications will be required - see inline comments; Modifications will 
    be required if using another type of ethernet shield; Wifi shields are NOT recommended

  Software requirements:
    If using an IEE or Noritake VFD You will need my modified version of the libraries included in the zip file

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

