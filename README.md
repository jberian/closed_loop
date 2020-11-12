Closed-Loop System for Type 1 diabetes
====

    This repository contains all the projects needed to build a 
    wearable closed-loop system for type 1 diabetes that uses a new
    algorithm to estimate the basal insulin needs of the patient and
    perform the control using temporary basal rates.

## Disclaimer

    This software and information is designed for educational purposes only 
    and should NEVER be used on real patients.
    
    This information is provided 'as is' and in no event shall the 
    provider be liable for any damages, including, without 
    limitation, damages resulting from lost data or lost profits or 
    revenue, the costs of recovering such data, the costs of substitute 
    data, claims by third parties or for other similar costs, or any 
    special, incidental, or consequential damages arising out of use or
    misuse of this data. The accuracy or reliability of the data is not 
    guaranteed or warranted in any way and the provider disclaim 
    liability of any kind whatsoever, including, without limitation, 
    liability for quality, performance, merchantability and fitness 
    for a particular purpose arising out of the use, or inability 
    to use the data. Information obtained via this software MUST
    NEVER BE USED to take medical decisions.

## [License - GPL V2](gpl-v2)
[gpl-2]: https://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation version 2 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

## Contents

   This repository containts the following information in their corresponding
   folder:
   
1. **Demo** - Contains a time-lapse video of a demo system connected to Nightscout and controlling a patient's glycaemia.

2. **Firmware** - Contains the project files needed to compile the platform's CC1111 firmware using IAR Embedded Workbench (the trial version should be enough). The code for the CC2540 processor is not included since the project should be downloaded directly from TI's website.
   
3. **Platform** - Contains the information to build two different platforms: 
    1. *Prototype* - It's a PCB in which some commercial off-the-self boards can be soldered and provides the needed interconnections for the system.
    2. *Final* - Final version of the wearable system.
 
4. **Publications** - Publications related to this work.

5. **Simulation** - Contains a modified version of the [simglucose simulator](https://github.com/jxx123/simglucose) along with a controller that behaves like the "real" version of the controller.

6. **Uploader** - Contains the xCode project for an really simple iOS uploader for Nightscout. This software allows the platform to upload data to Nightscout using an iOS device.


## Thanks

None of this would be possible without all the hard work of my colleagues Ignacio Bravo, Alfredo Gardel and Jose Luis Lazaro.

Sergio Hernandez, thank you very much for your countless hours of hardware designing.

And thank you Jinyu Xie for developing such excellent software ([simglucose](https://github.com/jxx123/simglucose)) and allow us to build upon it.
