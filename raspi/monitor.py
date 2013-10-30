#!/usr/bin/python
import serial
import datetime
import os
from time import sleep
import RPi.GPIO as GPIO

ser  = serial.Serial('/dev/ttyACM0', 9600)

GPIO.setmode(GPIO.BCM)
GPIO.setup(18, GPIO.OUT)
GPIO.output(18, True)

while True:
	data = ser.readline()
	if data.count("ted"):
		print data
		print datetime.datetime.now()
		os.system('/home/pi/Code/tavern/tavern-master/textme.py  -f /home/pi/Code/tavern/tavern-master/example_tavernrc  -c "wget http://192.168.1.15:8080/?action=snapshot -O /home/pi/camsnap.png" -m "Sensor tripped." -a /home/pi/camsnap.png')
		print "Sleeping for a bit..."
		sleep(10)
		print "Woke up."
	else:
		print "No motion."
	
