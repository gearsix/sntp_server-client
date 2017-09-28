# SNTP Server & Client

## Contents
1. [Overview](##Overview)
2. [Usage](##Usage)
3. [Compiling](##Compiling)
4. [Authors](##Authors)

## Overview
One of my 2nd yr projects.
It's pretty much what it says, an RFC4330 compliant SNTP server & client with both unicast and multicast addressing modes.
I've put the relevant RFC in docs/ along with design docs and screenshots of the working binaries (console logs & wireshark).

## Requirements
**For information on the game's system requirements, see docs/requirements.txt**

## Usage
1. Either compile src/ or just use the pre-built binaries in bin/
2. run `./server-*`, * = unicast/multicast
	* Usage for server-unicast `./server-unicast`
	* Usage for server-multicast `./server-multicast broadcast_interface <port>` (<> = optional)
3. run `./client-*`, * = unicast/multicast
	* Usage for client-unicast `./client-unicast <host address> <port>` (<> = optional)
	* Usage for client-mcasts `./client-mcasts

## Compiling
Compiling is really simple, so I didn't bother with a Makefile or anything.
1. ```gcc src/* -o bin/*``` (* = file, needs to be specified)
2. Done!

## Authors
* [Alexander Collins](https://www.github.com/GeaRSiX)