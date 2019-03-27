#include "UDPPinger.h"
#include <cmath>
#include <limits>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdexcept>
#include <cstring>
#include <fstream>
#include <unistd.h>
#include <iostream>
#include <sys/time.h>
#include <ctime>


namespace udppinger {

    UDPPinger::UDPPinger(const std::string& _addr, int _port, bool _failfast, bool _lastsuccess)
    :addr(_addr),port(_port), failfast(_failfast), lastsuccess(_lastsuccess)
    {
        // Set our hash of blacklisted ips coming from file 'blacklist.txt'
        std::fstream fs;
        std::string badIP;
        blacklistfilename = "blacklist.txt";
        fs.open (blacklistfilename, std::fstream::in);
        while(fs >> badIP){
            // badIP.erase(badIP.find_last_not_of("\n") + 1);
            blacklist.insert(badIP);
        }
        fs.close();

        std::string goodIp, fileavg, filemin, filemax, filetime;
        whitelistfilename = "whitelist.txt";
        fs.open (whitelistfilename, std::fstream::in);
        while(fs >> goodIp >> fileavg >> filemin >> filemax >> filetime){
            std::time_t now = std::time(nullptr);
            std::time_t fromfile(std::stol(filetime, nullptr));
            double seconds = difftime(now, fromfile);
            if(seconds < 60){ // Only whitelist ones we've hit in the last minute
                whitelist[goodIp] = fileavg + " " + filemin + " " + filemax;
            } else {
                this->removeFromFile(this->whitelistfilename, true);
            }
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

        bool onwhitelist = whitelist.count(this->getAddr());
        bool hasFailed = false;
        if(this->lastsuccess){
            if(onwhitelist){
                std::cout << "Using recent ping statistics and pinging only once" << std::endl;
                transmit = 1;
            } else {
                std::cout << "Recent ping statistics not found" << std::endl;
            }
        }

        float min = std::numeric_limits<float>::infinity();
        float max = -std::numeric_limits<float>::infinity();
        // float epsilon = std::numeric_limits<float>::epsilon();
        float total = 0, fduration;
        for(i = 1; i <= transmit; i++){
            std::string seq("Sequence number " + std::to_string(i));
            // Pad the sequence with null characters
            for(int s = seq.length(); s < 64; s++){
                seq += '\0';
            }
            start = clock();
            sent_bytes = sendto(sock, seq.c_str(), 64, 0, addressinfo->ai_addr, addressinfo->ai_addrlen);
            duration = clock() - start;
            if(sent_bytes != -1){
                std::cout << "bytes sent: " << sent_bytes << " " << seq << std::endl;
                char buf[64];
                int recvlen = recvfrom(sock, buf, 64, 0, addressinfo->ai_addr, &addressinfo->ai_addrlen);
                std::cout << "64 byte response from " << this->getAddr() << ": sequence " << i;
                if(recvlen >=  0){
                    fduration = ((float)duration)/CLOCKS_PER_SEC * 1000 ;
                    total += fduration;
                    max = fmax(max, fduration);
                    min = fmin(min, fduration);
                    std::cout << " time=" << fduration << " ms" << std::endl;
                    recieved++;
                } else {
                    // If we have a time out write it to a file
                    std::cout << " time=" << "TIMEOUT" << std::endl;
                    hasFailed = true;
                    if(!filewritten && !onBlackList){
                        std::fstream fs;
                        fs.open(this->blacklistfilename, std::fstream::out | std::fstream::app);
                        fs << this->getAddr() << "\n";
                        fs.close();
                        filewritten = true;
                        if(failfast){ i = i+1; break; }
                    }
                }
            } else {
                std::cout << "Unable to ping " << this->getAddr() << std::endl;
            }
        }
        if(lastsuccess && onwhitelist){
            if(!hasFailed ){
                std::cout << std::endl << "1 ping successful rereporting last minute statistics" << std::endl;
                std::cout << "IP -> Avg(ms) -> Min(ms) -> Max(ms)" << std::endl;
                std::cout  << this->getAddr() << " " << this->whitelist[this->getAddr()] << std::endl;
                return;
            }
        }
        i = i-1; // True number sent
        std::cout << std::endl << i << " packets transmitted, " << recieved << " recieved, ";
        if(i == 0) i = 1;
        float percentage = recieved == 0 ? 100.0 : (1-((float)recieved/(i)))*100;
        std::cout << percentage << "% loss " << std::endl;
        
        float avg;
        if(total == 0) avg = min = max = 0; // Total failure edge case
        else avg = total/recieved;
        std::cout << "RTT: avg: " << avg << "ms";
        std::cout << " min: " << min << "ms max: " << max << "ms" << std::endl;

        if(recieved == transmit){ //Successful ping, remove from our list and clear from file
            this->removeFromFile(this->blacklistfilename, false);
            // Add statistics to our whitelist for a given IP
            if(onwhitelist){
                this->removeFromFile(this->whitelistfilename, true);
            }
            std::fstream fs2;
            fs2.open(this->whitelistfilename, std::fstream::out | std::fstream::app);
            fs2 << this->getAddr() << " " << avg << " " << min << " " << max << " " << std::time(nullptr) << "\n";
            fs2.close();
        }
        if(onwhitelist && hasFailed){
          this->removeFromFile(this->whitelistfilename, true);
        }
    }

    void UDPPinger::removeFromFile(std::string filename, bool wl){
        std::string fileIp, ip = this->getAddr();
        std::ifstream filein(filename); 
        std::ofstream fileout("temp.txt");
        if(!filein || !fileout){
            std::cout << "Error opening files!" << std::endl;
            return;
        }
        ip = ip;
        if(wl){ // TODO this is very non DRY
            std::string goodIp, fileavg, filemin, filemax, filetime;
            while(filein >> goodIp >> fileavg >> filemin >> filemax >> filetime){
                if(goodIp != ip){
                    fileout << goodIp << " " << fileavg << " " << filemin << " " << filemax << " " << filetime;
                }
            }
        } else {
            while(filein >> fileIp){
                if(fileIp != ip){
                    fileout << fileIp;
                }
            }
        }
        if (std::rename("temp.txt", filename.c_str())) {
            std::perror("Error renaming tempblacklist.txt");
            return;
        }

    }


}