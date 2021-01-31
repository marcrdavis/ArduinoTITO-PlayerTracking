# Arduino TITO and Player Tracking
A homebrew slot machine TITO, player tracking and display project
By Marc Davis (1/29/2021)

  Project goals: To allow home slot machine owners the ability to use the player tracking
  display and card reader (with RFID modifications) which are normally non-functional
  outside of casinos. This replaces the Bally MasterCom device. The project can now
  communicate directly with SAS-enabled machines for remote control, TITO and metering.
  
  Build 20210129 Updates
  * THERE ARE CHANGES TO THE CONFIG.TXT FILE IN THIS BUILD! PLEASE UPDATE YOUR CONFIG.TXT FILE
    AS PER THE DOCUMENTATION
  
  - Adds support for Noritake GU-7000 or equivalent VFDs with modified library
  - Adds support for pixel displays and new config properties for setting row and colunm sizes
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
    Compatible vacuum fluorescent display or LCD; if using a display other than the default IEE VFD then
    modifications will be required - see inline comments; Modifications will be required if using another 
    type of ethernet shield; Wifi shields are NOT recommended

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

