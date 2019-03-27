# UDP Pinger
UDP Pinger is a basic udp pinging program that sends 10 64 UDP packets to a specified IP Port. Response timeouts are set to 1 second by default.

**Requirements**
C++ 11
Unix timestamp support

## To compile

    make

## Usage

    ./udppinger <Destination IP> <Destination UDP port>
   

## Optional Features/Flags:

 `-f`  "fail fast"

 This option will stop sending packets at the first timeout response. All timeouts will automatically blacklist an IP address. With the `-f` flag set we will only attempt 1 ping of a blacklisted IP address. If successful then this IP will be removed from the blacklist automatically.
 *Note: IPs are automatically blacklisted/removed regardless of this flag*
 
`-s` "last **s**uccess"

This option will check a white-listed IP file to see if an IP previously had 10 successful pings in the last minute. If an IP exists on the white-list and this flag is set then we will try one ping and report the previous statistics if successful. A failed ping (regardless of this flag) will remove from the white-list.
