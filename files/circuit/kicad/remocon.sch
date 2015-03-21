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
Sheet 9 11
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
L SENSOR U13
U 1 1 53F8FE49
P 5800 4200
F 0 "U13" H 5900 3950 39  0000 C CNN
F 1 "remocon" H 5650 3950 39  0000 C CNN
F 2 "" H 5500 4050 60  0000 C CNN
F 3 "" H 5500 4050 60  0000 C CNN
	1    5800 4200
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR041
U 1 1 53F8FE58
P 5800 4800
F 0 "#PWR041" H 5800 4800 30  0001 C CNN
F 1 "GND" H 5800 4730 30  0001 C CNN
F 2 "" H 5800 4800 60  0000 C CNN
F 3 "" H 5800 4800 60  0000 C CNN
	1    5800 4800
	1    0    0    -1  
$EndComp
$Comp
L +5V #PWR042
U 1 1 53F8FE67
P 5800 3550
F 0 "#PWR042" H 5800 3640 20  0001 C CNN
F 1 "+5V" H 5800 3640 30  0000 C CNN
F 2 "" H 5800 3550 60  0000 C CNN
F 3 "" H 5800 3550 60  0000 C CNN
	1    5800 3550
	1    0    0    -1  
$EndComp
Text HLabel 6300 4200 2    60   Input ~ 0
out
Wire Wire Line
	6250 4200 6300 4200
Wire Wire Line
	5800 3700 5800 3550
Wire Wire Line
	5800 4700 5800 4800
$Comp
L C C16
U 1 1 53FF6073
P 5350 4150
F 0 "C16" H 5350 4250 40  0000 L CNN
F 1 "0.1u" H 5356 4065 40  0000 L CNN
F 2 "~" H 5388 4000 30  0000 C CNN
F 3 "~" H 5350 4150 60  0000 C CNN
	1    5350 4150
	1    0    0    -1  
$EndComp
Wire Wire Line
	5800 3700 5350 3700
Wire Wire Line
	5350 3700 5350 3950
Wire Wire Line
	5350 4350 5350 4700
Wire Wire Line
	5350 4700 5800 4700
$EndSCHEMATC
