# Project Mitreo 17/18
This software was developped to monitor the ancient archelogical site: "Mitreo of Circus Maximum" in Rome.
It's a client-server architecture where clients sent ambiental information to server.


## Client side SW
Clients read the sensors on boosterpack and sent the samples to a server (root of an RPL network). 

#### List of quantity sampled
* Acceleration
* Rotation
* Temperature ambiental and IR
* Pressure
* Humidity
* Intensity of Light

#### Sensors Scheduling 
Acceleration or Rotation are sampled for a second at 400 Hz (400 sample/sec) every 5 minutes
Others are sampled one shot every 30 minutes.

## Server side SW
Server receives messages from all clients in the network and print them in serial stream. 

#### Messages structure
* Id of sender node
* Information type
* Information

## Deploy
#### In order to deploy this project you have to:
1. Change NODE_ID define for each node you want add to network in sensors-networking-client.c. The define's value must be a char
2. Compile with sudo make TARGET=srf06-cc26xx BOARD=launchpad-bp/cc1350-bp
3. Flash each node client with sensors-networking-client.bin and one node server with sensors-networking-server.bin using SmartRF Flash Programmer 2 provided by Texas Instruments (or Contiki procedure)
4. Done 




