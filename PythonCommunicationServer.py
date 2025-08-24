# created in Python 3.13.5 by Ashtorak
# Disclaimer: Big parts of this have been created with the help of Grok since I don't have
# much experience with Python. Please let me know, if there is anything that needs to be improved.
# ---
# This example Python script shows how to exchange data with StarbaseSim via TCP sockets.
# When you run this, it starts a loop in main() function trying to connect to the StarbaseSim data server.
# When connected, by default it will receive some data of the last spawned booster and ship, if any.
# You can adjust from which rocket and how often the game sends data and send back commands.
# See the other comments below for more explanations or just go to "sendCommands" function
# and put your own logic there. You might need to reduce the tick time though.

import socket
import json
import select
import sys
import signal
import time
import threading
from enum import IntEnum, auto
from typing import List, Dict, Optional, Tuple

# Define time in seconds that game thread waits before sending data again (0 = every frame).
# This will be sent to the game when connecting to its server.
GameSendDataTick = 1.0 

# Define time interval in seconds at which main loop receives and processes data.
# Should be lower than GameSendDataTick, but larger than 0. If the game sends data
# every frame, you should set it lower than expected frame time, e.g. 0.016 for 60 FPS.
PythonMainTick = 0.5 

# The times are set to relatively large values here due to printing out the data.
# This leads to a fairly high latency when sending commands.
# So reduce the time intervals as needed for your application.

# Set the following to true if you want some data logging to the console
printSomeData = True

# Defining a global client variable here since it's needed in the on_exit() function
# that is called when you stop the script with Ctrl+C for example.
client = None

# This is for some extra silly stuff that is off by default. Can be enabled in main()
sillyCount = 7

# The following is a list of all commands with parameters and some explanations.
# See example below in main function on how to use them.
# Each command gets an Integer (Int) automatically assigned like this. It's important 
# to not change the order of commands here or remove a line or so. Else the Ints
# won't match up with the game Ints anymore.
class GameCommand(IntEnum):
    NONE = 0
    
    SendDataTick = auto() # with that you can change the send data tick any time
    # "value": float (seconds)
    
    SetWhoSendsData = auto()
    # "parameters": rocketID (default=0=last spawned one, e.g. B0 or S0)
    
    SetRocketSetting = auto() 
    # "parameters": setting name like what is available in-game, e.g. 
    # "value": float
    
    SpawnAtLocation = auto() # this does not use the in-game spawn settings
    # "parameters": "Booster" or "Ship"
    # "location": [float, float, float] 
    # "rotation": [float, float, float] (in Euler)
    
    Engines = auto()
    # "target": rocketID (0=last spawned one, e.g. B0 or S0)
    # "state": True/False (= start/stop)
    
    Raptor = auto() # for toggling individual raptors
    # "target": rocketID (0=last spawned one, e.g. B0 or S0)
    # "value": Raptor number
    # "state": True/False (= request ON / request OFF)
    
    Throttle = auto()
    # "target": rocketID (0=last spawned one, e.g. B0 or S0)
    # "value": 0...100%
    
    RCS = auto()
    # "target": rocketID (0=last spawned one, e.g. B0 or S0)
    # "state": True/False (= ON/OFF)
    
    Flaps = auto()
    # "target": rocketID (0=last spawned one, e.g. B0 or S0)
    # "state": True/False (= ON/OFF)
    
    FoldFlaps = auto() # only works if flaps attitude control is OFF
    # "target": rocketID (0=last spawned one, e.g. B0 or S0)
    # "state": True/False (= fold/unfold)
    
    GridFins = auto()
    # "target": rocketID (0=last spawned one, e.g. B0 or S0)
    # "state": True/False (= ON/OFF)
    
    Gimbals = auto()
    # "target": rocketID (0=last spawned one, e.g. B0 or S0)
    # "state": True/False (= ON/OFF)
    
    SetRCSManual = auto() # set RCS output directly
    # "target": rocketID (0=last spawned one, e.g. B0 or S0)
    # "parameters": "x", "y" or "z"
    # "value": -1...1
    
    SetDragManual = auto() # set Grid Fin or Flap output directly
    # "target": rocketID (0=last spawned one, e.g. B0 or S0)
    # "parameters": "x", "y" or "z"
    # "value": -1...1
    
    SetGimbalManual = auto() # set Gimbal output directly
    # "target": rocketID (0=last spawned one, e.g. B0 or S0)
    # "parameters": "x", "y" or "z"
    # "value": -1...1
    
    Propellant = auto() # set propellant directly
    # "target": rocketID (0=last spawned one, e.g. B0 or S0)
    # "value": float (in tons)
    
    CryotankPressure = auto()
    # "target": rocketID (0=last spawned one, e.g. B0 or S0)
    # "value": float (in bar(g))
    
    HotStage = auto() # start HotStage sequence (or stop if it's running)
    # "target": rocketID (0=last spawned one, e.g. B0 or S0)

    DetachHSR = auto()
    # "target": rocketID (0=last spawned one, e.g. B0 or S0)
     
    FTS = auto()
    # "target": rocketID (0=last spawned one, e.g. B0 or S0)
     
    OuterGimbalEngines = auto() # only works for booster
    # "target": rocketID (0=last spawned one, e.g. B0 or S0)
    # "state": True/False (= move out / back to normal)
    
    BoosterClamps = auto()
    # "target": rocketID (0=last spawned one, e.g. B0 or S0)
    # "state": True/False (= ON/OFF)
    
    ControllerAltitude = auto()
    # "target": rocketID (0=last spawned one, e.g. B0 or S0)
    # "state": True/False (= ON/OFF)
    
    ControllerEastNorth = auto()
    # "target": rocketID (0=last spawned one, e.g. B0 or S0)
    # "state": True/False (= ON/OFF)
    
    ControllerAttitude = auto()
    # "target": rocketID (0=last spawned one, e.g. B0 or S0)
    # "parameters": "on", "off", "FlameyEndDown" and "retro"
    
    AttitudeTarget = auto() # set one component of the target vector
    # "target": rocketID (0=last spawned one, e.g. B0 or S0)
    # "parameters": "x", "y" or "z"
    # "value": float 
    
    ChillValve = auto()
    # "target": rocketID (0=last spawned one, e.g. B0 or S0)
    # "state": True/False (= ON/OFF)
    
    DumpFuel = auto()
    # "target": rocketID (0=last spawned one, e.g. B0 or S0)
    # "state": True/False (= ON/OFF)
    
    PopEngine = auto()
    # "target": rocketID (0=last spawned one, e.g. B0 or S0)
    # "value": engine number (must be running)
    
    BigFlame = auto()
    # "target": rocketID (0=last spawned one, e.g. B0 or S0)
    # "state": True/False (= ON/OFF)
    
    Chopsticks = auto()
    # "parameters": "open", "close", "stopOpenClose", "Height", "SpeedArms", "SpeedLift"
    # "value": float (height in m, speeds 0...1)
    
    PadADeluge = auto()
    # "state": True/False (= ON/OFF)
    
    PadASQDQuickRetract = auto() # just triggers quick retract
    
    PadAOLMQuickRetract = auto() # just triggers quick retract
    
    PadABQDQuickRetract = auto() # just triggers quick retract
    
    MasseyDeluge = auto()
    # "state": True/False (= ON/OFF)


# Rocket data is stored in this class (this is the only data that is sent by the game currently)
class RocketDataPacket:
    def __init__(self, objectname: str, location: List[float], rotation: List[float], velocity: List[float], fuelMass: float, oxidizerMass: float, enginesThatAreRunningBitmask: int):
        self.objectname = objectname # e.g. B1 or S2 corresponding to in-game booster or ship ID
        self.location = location # in m (+Y is East and +X is North)
        self.rotation = rotation # this is quaternion x, y, z, w
        self.velocity = velocity # in m/s
        self.fuelMass = fuelMass # liquid mass in tons
        self.oxidizerMass = oxidizerMass # liquid mass in tons
        self.enginesThatAreRunningBitmask = enginesThatAreRunningBitmask # every bit stands for an engine (see receive function for example)

    def __str__(self): # for printing out some data directly
        return f"RocketData(objectname={self.objectname}, location={self.location}, rotation={self.rotation})"


# connect_to_server() will be called in main loop when client is not connected.
# Don't need to change anything here normally.
def connect_to_server():
    global client
    client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        client.connect(("localhost", 12345))
        client.settimeout(0.1)  # Non-blocking with timeout for reconnect
        print("Connected to Unreal server")
        
        command = {
                "command": int(GameCommand.SendDataTick),
                "value": GameSendDataTick
            }
        client.send((json.dumps(command) + "\n").encode())
        if(printSomeData):
            print(f"Set GameSendDataTick to {GameSendDataTick} s")
        return client
    except Exception as e:
        print(f"Failed to connect: {e}")
        return None


# receive_data() is also called in main loop. This will only need adjustment in the future, 
# if more data gets sent by the game, or maybe if you want to store the received 
# data in another format for some reason.
def receive_data(client, buffer: str) -> tuple[str, List[RocketDataPacket]]:
    RocketData = []
    try:           
        data = client.recv(4096).decode('utf-8')  # Receive up to 4 KB

        if not data:
            raise ConnectionResetError("Server connection closed")
            return buffer, transforms
            
        buffer += data
        # Process complete messages (newline-delimited)
        while '\n' in buffer:
            message, buffer = buffer.split('\n', 1)
            if (message and message != "Client still there?"):
                try:
                    json_data = json.loads(message)
                    
                    # Validate and extract objectname, location and rotation
                    objectname = json_data.get('objectname', '')
                    location = json_data.get('location', [])
                    rotation = json_data.get('rotation', [])
                    velocity = json_data.get('velocity', [])
                    fuelMass = json_data.get('fuelMass', 0)
                    oxidizerMass = json_data.get('oxidizerMass', 0)
                    enginesThatAreRunningBitmask = json_data.get('enginesThatAreRunningBitmask', 0)
                    
                    if isinstance(objectname, str) and isinstance(location, list) and len(location) == 3 and isinstance(rotation, list) and len(rotation) == 4:
                        RocketData.append(RocketDataPacket(objectname, location, rotation, velocity, fuelMass, oxidizerMass, enginesThatAreRunningBitmask))
                        
                        if(printSomeData):
                            # Extract state of Engine No. 3 (bit 3: left shift 1 by two)
                            is_engine_no3_running = (enginesThatAreRunningBitmask & (1 << 2)) != 0
                            print(f"Received transform: objectname = {objectname}\n location = {location}\n velocity = {velocity}\n propMass = {fuelMass+oxidizerMass}\n is engine No. 3 running: {is_engine_no3_running}")
                    else:
                        print(f"Invalid transform format: {json_data}")
                except json.JSONDecodeError as e:
                    print(f"Failed to parse JSON: {e}, Message: {message}")
        return buffer, RocketData
    except socket.timeout:
        return buffer, RocketData  # Timeout, return partial buffer
    except (ConnectionResetError) as e:
        raise
    except Exception as e:
        print(f"Unexpected error in receive_data: {e}")
        return buffer, RocketData 


# spawn a bunch of ships with timer        
def sillyFunction(count):
    global client
    commands = []
    if(count > 0):
        count -= 1
        commands.append({
            "command": GameCommand.SpawnAtLocation,
            "parameters": "Ship",
            "location": [count*11500, 66666, 11100]
        })        
        for command in commands:
            if client:
                client.send((json.dumps(command) + "\n").encode())
        threading.Timer(0.5, sillyFunction, args=(count,)).start()
        shipID = abs(count - sillyCount)
        # send additional commands delayed to make sure everything has been fully initialized
        threading.Timer(1.0, sillyFunction2, args=(shipID,)).start()
        # cleanup
        threading.Timer(11.0, sillyFunction3, args=(shipID,)).start()
    #else:


def sillyFunction2(target):
    global client
    commands = []
    commands.append({
        "command": GameCommand.Propellant,
        "target": "S"+str(target),
        "value": 400
    })
    commands.append({
        "command": GameCommand.Engines,
        "target": "S"+str(target),
        "state": True
    })
    commands.append({
        "command": GameCommand.Throttle,
        "target": "S"+str(target),
        "value": 100
    })                            
    for command in commands:
        if client:
            client.send((json.dumps(command) + "\n").encode())
            

def sillyFunction3(target):
    global client
    commands = []
    commands.append({
        "command": GameCommand.FTS,
        "target": "S"+str(target)
    })    
    for command in commands:
        if client:
            client.send((json.dumps(command) + "\n").encode())


# The following function shows an example of how to send a command based on the data.
# Put your own logic here instead of this.
def sendCommands(RocketData):
    commands = [] # this array keeps the json strings that get send to the game
            
    # Example: if location of booster B1 is above 100 m, send engine stop command
    for rocket in RocketData:
        if rocket.objectname == "B1":
            if rocket.location[2] > 100:
                commands.append({
                    "command": GameCommand.Engines,
                    "target": "B1", # send to booster B1
                    "state": False # = stop
                })
                # It's crucial to follow the json formatting here.
                # The fields can be in arbitrary order. The commas not though! ;)
                # To avoid comma errors and such, best is to use something like VSCode.
                
    # send all commands to game (if you append more to the array)
    for command in commands:
        client.send((json.dumps(command) + "\n").encode())
        if(printSomeData):
            print(f"Sent command: {command}")


# main() function with main loop that does everything
def main():
    buffer = ""  # Persistent buffer for partial messages
    global client
    test = True 
    silly = False
    
    while True:
        try:
            if not client:
                client = connect_to_server()
                if not client:      # couldn't connect to game
                    time.sleep(1)   # try reconnecting in 1 second interval
                    continue
                buffer = ""  # Clear buffer on new connection
                
            # Receive and process data from Unreal
            # Note: by default the game sends the transforms from the last spawned booster and ship.
            # You can change that with SetWhoSendsData (see example below with "test" variable).
            # Currently it only sends data for one booster and one ship at a time.
            
            buffer, RocketData = receive_data(client, buffer)
            
            sendCommands(RocketData)
            
            # The following was used for testing, but left in as examples.
            # Comment and uncomment to try it out or just use as reference.
            if(test):
                test = False
                command = {
                    "command": GameCommand.SetWhoSendsData,
                    "target": "S1"
                    
                    #"command": GameCommand.Raptor,
                    #"target": "B1",
                    #"value": 2,
                    #"state": False # = off
                    
                    #"command": GameCommand.SetGimbalManual,
                    #"target": "B1",
                    #"parameters": "x",
                    #"value": -1
                    
                    #"command": GameCommand.Chopsticks,
                    #"parameters": "Height",
                    #"value": 99
                }
                client.send((json.dumps(command) + "\n").encode())
                 
            if(silly):
                silly = False
                sillyFunction(sillyCount)

        except Exception as e:
            print(f"Error: {e}")
            if client:
                client.close()
                client = None
           
        time.sleep(0.5)

# Define the function to call when stopping (e.g. via Ctrl+C)
def on_exit(signum, frame):
    global client
    print("\n Closing client...")
    if client:
        client.close()
    print("Client closed!")
    sys.exit(0)

# Register the signal handler for Ctrl+C (SIGINT)
signal.signal(signal.SIGINT, on_exit)
# Handle SIGTERM (e.g., from taskkill or Stop-Process)
signal.signal(signal.SIGTERM, on_exit)

if __name__ == "__main__":
    main()