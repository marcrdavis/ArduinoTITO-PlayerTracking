# Arduino TITO and PlayerTracking
A homebrew slot machine player tracking and display project
By Marc Davis (11/7/2020)

  Project goals: To allow home slot machine owners the ability to use the player tracking
  display and card reader (with RFID modifications) which are normally non-functional
  outside of casinos. This replaces the Bally MasterCom device. The project can now
  communicate directly with SAS-enabled machines for remote control, TITO and metering.

  Hardware requirements: Arduino Mega 2560 R3; RFID RC 522; W5100 Ethernet Shield; Serial Port Shield;
  Compatible vacuum fluorescent display or LCD; if using an LCD then modifications will be required;
  Modifications will be required if using another type of ethernet shield; Wifi shields are NOT recommended

  Software requirements:
    You will need my modified version of IeeFlipNoFrills.h which fixes compatibility issues with
    newer Arduino IDE

  For TITO Setup please follow the included documentation

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

