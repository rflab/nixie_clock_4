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
Sheet 5 8
Title ""
Date "20 mar 2015"
Rev ""
Comp ""
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
Text HLabel 4100 4200 0    60   Input ~ 0
D+
Text HLabel 4100 4050 0    60   Input ~ 0
D-
$Comp
L R R20
U 1 1 53F90F3C
P 4500 4050
F 0 "R20" V 4580 4050 40  0000 C CNN
F 1 "68" V 4507 4051 40  0000 C CNN
F 2 "~" V 4430 4050 30  0000 C CNN
F 3 "~" H 4500 4050 30  0000 C CNN
	1    4500 4050
	0    -1   -1   0   
$EndComp
$Comp
L R R21
U 1 1 53F90F4B
P 4500 4200
F 0 "R21" V 4580 4200 40  0000 C CNN
F 1 "68" V 4507 4201 40  0000 C CNN
F 2 "~" V 4430 4200 30  0000 C CNN
F 3 "~" H 4500 4200 30  0000 C CNN
	1    4500 4200
	0    -1   -1   0   
$EndComp
$Comp
L R R23
U 1 1 53F90FB6
P 5650 4750
F 0 "R23" V 5730 4750 40  0000 C CNN
F 1 "NC" V 5657 4751 40  0000 C CNN
F 2 "~" V 5580 4750 30  0000 C CNN
F 3 "~" H 5650 4750 30  0000 C CNN
	1    5650 4750
	-1   0    0    1   
$EndComp
$Comp
L R R22
U 1 1 53F91017
P 4850 3600
F 0 "R22" V 4930 3600 40  0000 C CNN
F 1 "2K" V 4857 3601 40  0000 C CNN
F 2 "~" V 4780 3600 30  0000 C CNN
F 3 "~" H 4850 3600 30  0000 C CNN
	1    4850 3600
	1    0    0    -1  
$EndComp
$Comp
L +5V #PWR021
U 1 1 53F91040
P 4850 3250
F 0 "#PWR021" H 4850 3340 20  0001 C CNN
F 1 "+5V" H 4850 3340 30  0000 C CNN
F 2 "" H 4850 3250 60  0000 C CNN
F 3 "" H 4850 3250 60  0000 C CNN
	1    4850 3250
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR022
U 1 1 53F910A2
P 5650 5050
F 0 "#PWR022" H 5650 5050 30  0001 C CNN
F 1 "GND" H 5650 4980 30  0001 C CNN
F 2 "" H 5650 5050 60  0000 C CNN
F 3 "" H 5650 5050 60  0000 C CNN
	1    5650 5050
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR023
U 1 1 53F910B1
P 5850 5050
F 0 "#PWR023" H 5850 5050 30  0001 C CNN
F 1 "GND" H 5850 4980 30  0001 C CNN
F 2 "" H 5850 5050 60  0000 C CNN
F 3 "" H 5850 5050 60  0000 C CNN
	1    5850 5050
	1    0    0    -1  
$EndComp
$Comp
L DIODE D4
U 1 1 53F910F0
P 4850 4750
F 0 "D4" H 4850 4850 40  0000 C CNN
F 1 "DIODE" H 4850 4650 40  0000 C CNN
F 2 "~" H 4850 4750 60  0000 C CNN
F 3 "~" H 4850 4750 60  0000 C CNN
	1    4850 4750
	0    -1   -1   0   
$EndComp
$Comp
L DIODE D3
U 1 1 53F91104
P 5150 4750
F 0 "D3" H 5150 4850 40  0000 C CNN
F 1 "DIODE" H 5150 4650 40  0000 C CNN
F 2 "~" H 5150 4750 60  0000 C CNN
F 3 "~" H 5150 4750 60  0000 C CNN
	1    5150 4750
	0    -1   -1   0   
$EndComp
$Comp
L PWR_FLAG #FLG024
U 1 1 53FCAA4C
P 5900 3800
F 0 "#FLG024" H 5900 3895 30  0001 C CNN
F 1 "PWR_FLAG" H 5900 3980 30  0000 C CNN
F 2 "" H 5900 3800 60  0000 C CNN
F 3 "" H 5900 3800 60  0000 C CNN
	1    5900 3800
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR025
U 1 1 53FCBB40
P 7300 4600
F 0 "#PWR025" H 7300 4600 30  0001 C CNN
F 1 "GND" H 7300 4530 30  0001 C CNN
F 2 "" H 7300 4600 60  0000 C CNN
F 3 "" H 7300 4600 60  0000 C CNN
	1    7300 4600
	1    0    0    -1  
$EndComp
Wire Wire Line
	4750 4050 4850 4050
Wire Wire Line
	4850 4050 5950 4050
Wire Wire Line
	4750 4200 5150 4200
Wire Wire Line
	5150 4200 5400 4200
Wire Wire Line
	5400 4200 5950 4200
Wire Wire Line
	4100 4050 4250 4050
Wire Wire Line
	4250 4200 4100 4200
Wire Wire Line
	4850 3850 4850 4050
Wire Wire Line
	4850 4050 4850 4550
Wire Wire Line
	4850 3350 4850 3250
Wire Wire Line
	5950 4500 5850 4500
Wire Wire Line
	5850 4500 5850 5050
Wire Wire Line
	5950 4350 5650 4350
Wire Wire Line
	5650 4350 5650 4500
Wire Wire Line
	5650 5000 5650 5050
Wire Wire Line
	5150 4550 5150 4200
Wire Wire Line
	5500 3900 5900 3900
Wire Wire Line
	5900 3900 5950 3900
Wire Wire Line
	5900 3800 5900 3900
Connection ~ 5900 3900
Wire Wire Line
	7050 3900 7300 3900
Wire Wire Line
	7300 3900 7300 4050
Wire Wire Line
	7300 4050 7300 4350
Wire Wire Line
	7300 4350 7300 4500
Wire Wire Line
	7300 4500 7300 4600
Wire Wire Line
	7050 4050 7300 4050
Connection ~ 7300 4050
Wire Wire Line
	7050 4350 7300 4350
Connection ~ 7300 4350
Wire Wire Line
	7050 4500 7300 4500
Connection ~ 7300 4500
$Comp
L USB_MINI_B CON1
U 1 1 53FCC671
P 6500 4200
F 0 "CON1" H 6250 4650 60  0000 C CNN
F 1 "USB_MINI_B" H 6450 3700 60  0000 C CNN
F 2 "~" H 6500 4200 60  0000 C CNN
F 3 "~" H 6500 4200 60  0000 C CNN
	1    6500 4200
	1    0    0    -1  
$EndComp
$Comp
L R R32
U 1 1 5404B94E
P 5400 4750
F 0 "R32" V 5480 4750 40  0000 C CNN
F 1 "1M" V 5407 4751 40  0000 C CNN
F 2 "~" V 5330 4750 30  0000 C CNN
F 3 "~" H 5400 4750 30  0000 C CNN
	1    5400 4750
	1    0    0    -1  
$EndComp
Wire Wire Line
	5400 4500 5400 4200
Connection ~ 5400 4200
$Comp
L GND #PWR026
U 1 1 5404B98B
P 5400 5050
F 0 "#PWR026" H 5400 5050 30  0001 C CNN
F 1 "GND" H 5400 4980 30  0001 C CNN
F 2 "" H 5400 5050 60  0000 C CNN
F 3 "" H 5400 5050 60  0000 C CNN
	1    5400 5050
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR027
U 1 1 5404B998
P 5150 5050
F 0 "#PWR027" H 5150 5050 30  0001 C CNN
F 1 "GND" H 5150 4980 30  0001 C CNN
F 2 "" H 5150 5050 60  0000 C CNN
F 3 "" H 5150 5050 60  0000 C CNN
	1    5150 5050
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR028
U 1 1 5404B99E
P 4850 5050
F 0 "#PWR028" H 4850 5050 30  0001 C CNN
F 1 "GND" H 4850 4980 30  0001 C CNN
F 2 "" H 4850 5050 60  0000 C CNN
F 3 "" H 4850 5050 60  0000 C CNN
	1    4850 5050
	1    0    0    -1  
$EndComp
Text GLabel 5500 3900 0    60   Input ~ 0
USB5V
Wire Wire Line
	5400 5000 5400 5050
Wire Wire Line
	5150 4950 5150 5050
Wire Wire Line
	4850 4950 4850 5050
Connection ~ 5150 4200
Connection ~ 4850 4050
$EndSCHEMATC