#!/usr/bin/python3
import random
from socket import socket, AF_INET, SOCK_DGRAM

# Simple local python UDP server that will drop a packet ~40% of the time
# This server is only used to test UDP clients

def main():
    server = socket(AF_INET, SOCK_DGRAM)
    server.bind(("127.0.0.1", 12000))
    print("UDP server listening on", server.getsockname())

    while 1:
        rand = random.randint(0,10)
        message, address = server.recvfrom(1024)
        if message:
            print("Message recieved from", address, "rand", rand)
        if rand < 4:
            # Drop the packet
            continue
        server.sendto(message.upper(), address)

if __name__ == '__main__':
    main()