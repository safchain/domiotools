Domiotools
=========

A set of Domotic tools mainly focus on the Raspberry & Arduino platforms.

srts_sender
----------------
 Somfy RTS client for Raspberry & Arduino.

homeasy_sender
----------------------
Home Easy protocol client for Raspberry.

rf_eventd
-------------
Daemon that listens on GPIO pins in order to detect RF protocols and trigger MQTT message, and listens for MQTT message in order to trigger RF calls. Currently the aim of rf_eventd is to be used with OpenHAB, but could be used with other projects.

Dependencies
-------------------
 - wiringPi 
 - libconfig (>= 9.0) 
 - libmosquitto (>= 1.3.5)

Installation
----------------
./configure
make
sudo make install

rf_eventd configuration file
-------------------------------------

    version = "1.0";
    
    config:
    {
        publishers:
        (
            {
                type: "srts";
                address: 3333;
                output: "mqtt://localhost:1883/myhouse/lights/sdb_rdc";
                translations:
                {
                    UP: "ON";
                    DOWN: "OFF";
                }
            },
            {
                type: "homeasy";
                address: 3333;
                output: "mqtt://localhost:1111/myhouse/lights/sdb_rdc";
            }
         );
    
        subscribers:
        (
            {
                input: "mqtt://localhost:1111/myhouse/";
                type: "homeasy";
                address: 4242345234;
            }
        )
    };

License
-------

This program is free software; you can redistribute it and/or modify it under the terms of the
GNU General Public License as published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along with this program; if
not, write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
02110-1301, USA.


