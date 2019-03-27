#include <iostream>
#include <string>
#include "UDPPinger.h"
#include <unistd.h>


int main(int argc, char **argv)
{
    if(argc < 3){
        std::cout << "Usage: ./" << argv[0] << " <dest_IP> <dest_port>" << std::endl;
        return 0;
    }
    bool failfast = false;
    bool lastsuccess = false; 
    char c;
    while ((c = getopt(argc, argv, "fs")) != -1){
        switch(c){
            case 'f':
                failfast = true;
                break;
            case 's':
                lastsuccess = true;
                break;
            default:
                break;

        }
    }

    int rec_port = atoi(argv[argc-1]);      // second arg is port number
    udppinger::UDPPinger pinger = udppinger::UDPPinger(argv[argc-2], rec_port, failfast, lastsuccess);

    pinger.ping();


    return 0;
}