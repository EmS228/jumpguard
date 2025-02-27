import spidev
import RPi.GPIO as GPIO
import time

# Create SPI object
spi = spidev.SpiDev()

# Open SPI bus 0, device 0 (CE0)
spi.open(0, 0)

# Set SPI speed and mode
spi.max_speed_hz = 125000  # Max speed (can adjust)
spi.mode = 0b00  # SPI Mode 0

#RST GPIO setup pin 15
GPIO.setwarnings(False)
GPIO.setmode(GPIO.BCM)
GPIO.setup(22, GPIO.OUT)
GPIO.output(22, GPIO.LOW)
time.sleep(1.01)
GPIO.output(22, GPIO.HIGH)

# LoRa init
def LoRaInit():
    response = spi.xfer2([0x42, 0x00])
    if response[1] != 0x12:
        print("not the right chip") 
        while True:
            time.sleep(1)

    spi.xfer2([0x81, 0x80]) #into LoRa mode

    spi.xfer2([0x86, 0xE4]) #set freq 915 mHz
    spi.xfer2([0x87, 0xC0])
    spi.xfer2([0x88, 0x00])

    spi.xfer2([0x8E, 0x00]) #set tx and rx base address
    spi.xfer2([0x8F, 0x00])

    spi.xfer2([0x8C, 0x23]) #set gain

    spi.xfer2([0xA6, 0x04]) #turn on LNA gain

    spi.xfer2([0xCD, 0x84]) #PADac default

    spi.xfer2([0x8B, 0x2B]) #set RegOCP 

    spi.xfer2([0x89, 0x8F]) # set RegPa Config

    spi.xfer2([0x81, 0x81]) # set to LoRa/standby mode

def send(signal):
    print("starting to send packet:", hex(signal))
    response = spi.xfer2([0x12, 0x00])
    if response[1] != 0x00:
        print("flags not cleared when trying to send") 
        spi.xfer2([0x92, 0xFF])
        return
    spi.xfer2([0x81, 0x81]) #put into standby mode
    time.sleep(0.001)
    spi.xfer2([0x9D, 0x72]) #set modem config
    time.sleep(0.001)
    spi.xfer2([0x8D, 0x00]) #set fifo register pointer
    time.sleep(0.001)
    spi.xfer2([0xA2, 0x00]) #set payload to 0 
    time.sleep(0.001)
    spi.xfer2([0x80, signal]) #put signal in fifo reg
    time.sleep(0.001)
    spi.xfer2([0xA2, 0x01]) #set payload length
    time.sleep(0.001)
    spi.xfer2([0x81, 0x83]) #put into transmit mode
    time.sleep(0.001)
    while True:
        time.sleep(0.001)
        print("checkin txdone flag")
        response = spi.xfer2([0x12, 0x00])
        if (response[1] & 0x08):
            spi.xfer2([0x12, 0xFF])
            print("done sending")
            time.sleep(0.001)
            break


LoRaInit()
while True:
    LoRaInit()
    send(0x31)
    time.sleep(0.500)
    LoRaInit()
    send(0x30)
    time.sleep(0.500)


#response = spi.xfer2([0x42, 0x00])
#print("Response:", [hex(byte) for byte in response])

# Close SPI connection
spi.close()
GPIO.cleanup()
