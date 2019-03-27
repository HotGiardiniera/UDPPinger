#ifndef UDP_PINGER_H
#define UDP_PINGER_H

#include <string>
#include <netdb.h>
#include <unordered_set>
#include <unordered_map>


namespace udppinger{

class UDPPinger
{
public:
    UDPPinger(const std::string& _addr, int _port, bool _failfast, bool _lastsuccess);
    ~UDPPinger();

    int getSocket() const;
    int getPort() const;
    std::string getAddr() const;

    void ping();

private:
    std::string         addr;
    int                 port;
    int                 sock;
    struct addrinfo*    addressinfo;

    bool                failfast;
    bool                lastsuccess;

    std::string         blacklistfilename;
    std::string         whitelistfilename;

    std::unordered_set<std::string>                  blacklist;
    std::unordered_map<std::string, std::string>     whitelist;
    void removeFromFile(std::string filename, bool wl);

    
};

}



#endif //UDP_PINGER_H