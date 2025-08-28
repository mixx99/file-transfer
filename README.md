# File-Transfer

Transfer files from client to server. This is very similar to my another project with P2P chat. [link](https://github.com/mixx99/ybica-telegram)

## Build
```bash
# Windows:
cmake -B build
cmake --build build
```

## Usage
```bash
cd build/Debug
# client.exe <ip> <tcp-port> <udp-port> <filename> <delay>
client.exe 127.0.0.1 5555 6000 test.txt 500
# server.exe <ip> <tcp-port> <directory>
server.exe 127.0.0.1 5555 temp
```

## Features

- Reliable file transfer over UDP with TCP-based control channel
- CRC checksum verification for data integrity
- Automatic packet ordering and duplicate handling
- Configurable packet delay for testing network conditions
- Safe console output (thread-safe logging)
- Simple CMake building.
- Multiple message types for flexible project expansion

## Tests

I've tested in with a few `.txt` and `.png` files and everything seems to work fine.
