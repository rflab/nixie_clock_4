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
Sheet 13 13
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
L IC_8PIN IC12
U 1 1 53F95D31
P 5950 3850
F 0 "IC12" H 5950 4100 39  0000 C CNN
F 1 "LPS331AP" H 5950 3850 39  0000 C CNN
F 2 "" H 5950 3750 60  0000 C CNN
F 3 "" H 5950 3750 60  0000 C CNN
	1    5950 3850
	1    0    0    -1  
$EndComp
$Comp
L +5V #PWR055
U 1 1 53F95D40
P 5300 3400
F 0 "#PWR055" H 5300 3490 20  0001 C CNN
F 1 "+5V" H 5300 3490 30  0000 C CNN
F 2 "" H 5300 3400 60  0000 C CNN
F 3 "" H 5300 3400 60  0000 C CNN
	1    5300 3400
	1    0    0    -1  
$EndComp
Wire Wire Line
	5500 3700 5300 3700
Wire Wire Line
	5300 3400 5300 4000
Wire Wire Line
	6400 3700 6700 3700
Wire Wire Line
	6700 3700 6700 4400
$Comp
L GND #PWR056
U 1 1 53F95D58
P 6700 4400
F 0 "#PWR056" H 6700 4400 30  0001 C CNN
F 1 "GND" H 6700 4330 30  0001 C CNN
F 2 "" H 6700 4400 60  0000 C CNN
F 3 "" H 6700 4400 60  0000 C CNN
	1    6700 4400
	1    0    0    -1  
$EndComp
Wire Wire Line
	4850 3800 5500 3800
Wire Wire Line
	5500 3900 4850 3900
Text HLabel 4850 3800 0    60   Input ~ 0
SCL
Text HLabel 4850 3900 0    60   Input ~ 0
SDA
Wire Wire Line
	5300 4000 5500 4000
Connection ~ 5300 3700
Text Notes 5700 4200 0    39   ~ 0
4:SDO //SLA low bit\n5:CS // IIC/~SPI\n6:int1\n7:int2
NoConn ~ 6400 3900
NoConn ~ 6400 3800
Wire Wire Line
	6400 4000 6550 4000
Wire Wire Line
	6550 4000 6550 3400
$Comp
L +5V #PWR057
U 1 1 53F95D95
P 6550 3400
F 0 "#PWR057" H 6550 3490 20  0001 C CNN
F 1 "+5V" H 6550 3490 30  0000 C CNN
F 2 "" H 6550 3400 60  0000 C CNN
F 3 "" H 6550 3400 60  0000 C CNN
	1    6550 3400
	1    0    0    -1  
$EndComp
$EndSCHEMATC
