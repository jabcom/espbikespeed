import socket
import time
import random

#IP data will be sent to
UDP_IP = "127.0.0.1"
#Port data will be sent to
UDP_PORT = 5005
#Frequency data will be sent, Hertz
SEND_FREQUENCY = 120
#Speed of acceleration, Linier
ACCELERATION_CURVE = 0.5
#Time in seconds to chagne speed target
SPEED_STEP_CHANGE = 10
#Max speed to aim for
MAX_SPEED = 10

def sendSpeed(speed):
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.sendto(bytes(str(speed), "utf-8"), (UDP_IP, UDP_PORT))

speed = 0.0
speedTarget = random.randint(1,MAX_SPEED)
timeDelta = 0.0
while True:
    time.sleep(1/SEND_FREQUENCY)
    sendSpeed(speed)
    print(chr(27)+'[2j' + '\033c' + '\x1bc')
    print("Sending data to: " + UDP_IP + ":" + str(UDP_PORT))
    print("SPEED: " + str(speed))
    print("TARGET: " + str(speedTarget))
    if speed <= speedTarget:
        speed = speed + (ACCELERATION_CURVE/SEND_FREQUENCY)
    else:
        speed = speed - (ACCELERATION_CURVE/SEND_FREQUENCY)
    if speed < 0.0:
        speed = 0.0
    timeDelta = timeDelta + 1/SEND_FREQUENCY
    if timeDelta >= SPEED_STEP_CHANGE:
        timeDelta = 0.0
        speedTarget = random.randint(0,MAX_SPEED)
