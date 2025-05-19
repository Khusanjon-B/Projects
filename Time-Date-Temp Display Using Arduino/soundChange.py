import serial
import time
from pycaw.pycaw import AudioUtilities, IAudioEndpointVolume
from ctypes import cast, POINTER
from comtypes import CLSCTX_ALL


ser = serial.Serial('COM8', 9600, timeout=1)
time.sleep(2)  # wait for the serial connection to initialize

# Get audio device
devices = AudioUtilities.GetSpeakers()
interface = devices.Activate(
    IAudioEndpointVolume._iid_, CLSCTX_ALL, None)
volume = cast(interface, POINTER(IAudioEndpointVolume))

trigger_code = "TIME?"  # This is the code you're scanning for
response_code = time.ctime()   # This is the code you'll send back

# Step 1: Wait for the specific trigger code
while True:
    if ser.in_waiting > 0:
        line = ser.readline().decode('utf-8').strip()
        print(f"Received: {line}")
        if line == trigger_code.strip():
            print("Trigger code detected!")
            ser.write(response_code.encode('utf-8'))  # Send response
            break

# Step 2: Start reading serial normally
print("Starting normal serial read...")

while True:
    
    if ser.in_waiting > 0:
        
        line = ser.readline().decode('utf-8').strip()
        
        if line:  # Check if line is not empty
            
            try:
                vol = float(line)/100.0
                
                current_volume = volume.GetMasterVolumeLevelScalar()
                
                if abs(vol - current_volume) > 0.001:
                    
                    volume.SetMasterVolumeLevelScalar(vol, None)
                
            except ValueError:
                
                print(f"Ignoring invalid input: {line}")
            
            if line == "TIME?":
                ser.write(response_code.encode('utf-8'))


