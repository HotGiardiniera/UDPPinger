#include "UDPPinger.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdexcept>
#include <cstring>
#include <fstream>
#include <unistd.h>
#include <iostream>
#include <sys/time.h>
#include <time.h> 

namespace udppinger {

    UDPPinger::UDPPinger(const std::string& _addr, int _port, bool _failfast, bool _lastsuccess)
    :addr(_addr),port(_port), failfast(_failfast), lastsuccess(_lastsuccess)
    {
        // Set our hash of blacklisted ips coming from file 'blacklist.txt'
        std::fstream fs;
        std::string badIP;
        fs.open ("blacklist.txt", std::fstream::in);
        while(fs >> badIP){
            // badIP.erase(badIP.find_last_not_of("\n") + 1);
            blacklist.insert(badIP);
        }
        fs.close();

        fs.open ("whitelist.txt", std::fstream::in);
        while(fs >> goodIp){
            blacklist.insert(goodIp);
        }
        fs.close();

        char port_string[16];
        snprintf(port_string, sizeof(port_string), "%d", port);
        port_string[sizeof(port_string) / sizeof(port_string[0]) - 1] = '\0';
        struct addrinfo hints;
        std::memset(&hints, 0, sizeof(struct addrinfo));
        hints.ai_family = AF_UNSPEC;    // Allows IPv4 or IPv6
        hints.ai_socktype = SOCK_DGRAM; // Datagram socket
        hints.ai_protocol = IPPROTO_UDP; // UDP protocol
        int s = getaddrinfo(addr.c_str(), port_string, &hints, &addressinfo);
        if(s != 0 || !addressinfo){
            std::cout << "Usage: ./<this_program>"<< " <dest_IP> <dest_port>" << std::endl;
            throw std::runtime_error("Invalid host/port");

        }
        sock = socket(addressinfo->ai_family, SOCK_DGRAM, addressinfo->ai_protocol);
        if(sock == -1){ // Failed socket connection
            freeaddrinfo(addressinfo);
            throw std::runtime_error("Failed to create socket");
        }
        // Lastly set a timeout for our pinger
        struct timeval tv;
        tv.tv_sec = 1;
        tv.tv_usec = 0;  // 100 ms
        if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,&tv, sizeof(tv)) < 0) {
            perror("Error could not set a timeout");
        }
    }

    UDPPinger::~UDPPinger(){
        freeaddrinfo(addressinfo);
        close(sock);
    }

    int UDPPinger::getSocket() const {
        return sock;
    }

    int UDPPinger::getPort() const {
        return port;
    }

    std::string UDPPinger::getAddr() const {
        return addr;
    }

    void UDPPinger::ping(){
        int transmit = 10;
        int recieved = 0 ;
        int sent_bytes;
        clock_t start, duration;
        int i;
        bool onBlackList = false, filewritten = false;

        if(blacklist.count(this->getAddr())){
            onBlackList = true;
            std::cout << "Warning! UDP ping to " << this->getAddr() << " previously failed";
            if(this->failfast){
              std::cout << " only attempting 1 ping";
              transmit = 1;
            } else {
                 std::cout << ". Consider running with the -f flag for one ping!";
            }
            std::cout << std::endl;
        }

        if(this->lastsuccess){
            if(whitelist.count(this->getAddr())){
                std::cout << "Using recent ping statistics and pinging only once" << std::endl;
                transmit = 1;
            } else {
                std::cout << "Recent ping statistics not dound" << std::endl;
            }
            
        }

        for(i = 0; i < transmit; i++){
            std::string seq("Sequence number " + std::to_string(i+1));
            // Pad the sequence with null characters
            for(int s = seq.length(); s < 64; s++){
                seq += '\0';
            }
            start = clock();
            sent_bytes = sendto(sock, seq.c_str(), 64, 0, addressinfo->ai_addr, addressinfo->ai_addrlen);
            duration = clock() - start;
            if(sent_bytes != -1){
                std::cout << "bytes sent: " << sent_bytes << std::endl;
                char buf[64];
                int recvlen = recvfrom(sock, buf, 64, 0, addressinfo->ai_addr, &addressinfo->ai_addrlen);
                std::cout << "64 byte response from " << this->getAddr() << ": sequence " << i;
                if(recvlen >=  0){ 
                    std::cout << " time=" << ((float)duration)/CLOCKS_PER_SEC * 1000 << " ms" << std::endl;
                    recieved++;
                } else {
                    // If we have a time out write it to a file
                    std::cout << " time=" << "TIMEOUT" << std::endl;
                    if(!filewritten && !onBlackList){
                        std::fstream fs;
                        fs.open("blacklist.txt", std::fstream::out | std::fstream::app);
                        fs << this->getAddr() << "\n";
                        fs.close();
                        filewritten = true;
                        if(failfast) break;
                    }

                }
            } else {
                std::cout << "Unable to ping " << this->getAddr() << std::endl;
            }
        }
        std::cout << i << " packets transmitted, " << recieved << " recieved, ";
        std::cout << ((1-((float)recieved/i))*100) << "% loss" << std::endl;

        if(recieved == transmit){ //Successful ping, remove from our list and clear from file
            this->removeFromBlackList();
        }
    }

    void UDPPinger::removeFromBlackList(){
        std::string fileIp, ip = this->getAddr();
        std::ifstream filein("blacklist.txt"); 
        std::ofstream fileout("tempblacklist.txt");
        if(!filein || !fileout){
            std::cout << "Error opening files!" << std::endl;
            return;
        }
        ip = ip;
        while(filein >> fileIp){
            if(fileIp != ip){
                std::cout << "rewriting " << fileIp << std::endl;
                fileout << fileIp;
            }
        }
        if (std::rename("tempblacklist.txt", "blacklist.txt")) {
            std::perror("Error renaming tempblacklist.txt");
            return;
        }

    }


}