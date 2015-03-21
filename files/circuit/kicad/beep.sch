EESchema Schematic File Version 2
LIBS:power
LIBS:device
LIBS:transistors
LIBS:conn
LIBS:linear
LIBS:regul
LIBS:74xx
LIBS:cmos4000
LIBS:adc-dac
LIBS:memory
LIBS:xilinx
LIBS:special
LIBS:microcontrollers
LIBS:dsp
LIBS:microchip
LIBS:analog_switches
LIBS:motorola
LIBS:texas
LIBS:intel
LIBS:audio
LIBS:interface
LIBS:digital-audio
LIBS:philips
LIBS:display
LIBS:cypress
LIBS:siliconi
LIBS:opto
LIBS:atmel
LIBS:contrib
LIBS:valves
LIBS:mydevice
LIBS:kicad_project-cache
EELAYER 27 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 8 11
Title ""
Date "31 aug 2014"
Rev ""
Comp ""
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L LOAD U12
U 1 1 53F901A9
P 5750 3950
F 0 "U12" H 5830 3710 39  0000 C CNN
F 1 "LOAD" H 5630 3710 39  0000 C CNN
F 2 "" H 5750 3950 60  0000 C CNN
F 3 "" H 5750 3950 60  0000 C CNN
	1    5750 3950
	1    0    0    -1  
$EndComp
Text HLabel 5750 4550 3    60   Input ~ 0
beep
$Comp
L +5V #PWR040
U 1 1 53F901C9
P 5750 3400
F 0 "#PWR040" H 5750 3490 20  0001 C CNN
F 1 "+5V" H 5750 3490 30  0000 C CNN
F 2 "" H 5750 3400 60  0000 C CNN
F 3 "" H 5750 3400 60  0000 C CNN
	1    5750 3400
	1    0    0    -1  
$EndComp
Wire Wire Line
	5750 3450 5750 3400
Wire Wire Line
	5750 4450 5750 4550
$EndSCHEMATC
