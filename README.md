# Asynchronous Audio Relay with Raspberry Pi

## Project Overview

This project implements an asynchronous audio relay using a Raspberry Pi. A server is run on the Raspberry Pi which allows users to call and share audio messages. If users disconnect, they are able to replay audio messages they missed once they have reconnected.

## Installation and Usage

### Dependencies/Requirements

- Windows/Linux OS (No guarantees for Mac)
- GCC or Clang
- [Portaudio](https://portaudio.com/)
  - Use a terminal like MSYS2 MinGW64 to have access to both GCC and Portaudio
- rsync (for automatically syncing code to the Pi)
- Rasbperry Pi 5

### Setup

```bash
git clone https://github.com/AamindMandragora/async-audio-relay-pi.git
```

In `shell/run-server.sh` and `shell/deploy-server.sh` on the local machine, change

```bash
PI_HOST=pi@127.0.0.1
PI_DIR=/home/pi/async-audio-relay
```

to the IP of the Raspberry Pi and the desired directory.

From the base directory of the local machine, run

``` bash
bash shell/run-server.sh
```

to run the server.

To connect, run

``` bash
bash shell/run-server.sh <username>
```

to connect.
In the client terminal, instructions will appear. They have the following functions:

- list: list all connected clients
- voicemail \<target> : play unheard voicemails from \<target>. When unspecified, all unheard voicemails are played
- call \<user> : Attempts to call the specified users (callee can accept or reject call)
- broadcast : Sends a message to all users

## Group Members and Roles

Advayth Pashupati (advayth2):

- SPSC client threads
- Persistent audio storage
- Audio data protocol

Albert Han (ahan30):

- Portaudio recording/playback
- Thread safe queue
- UDP server
