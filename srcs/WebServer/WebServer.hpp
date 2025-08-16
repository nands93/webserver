#ifndef WEBSERVER_HPP
# define WEBSERVER_HPP

# include "../ConfigParser/ConfigParser.hpp"
# include "../VirtualServer/VirtualServer.hpp"
# include "../Socket/Socket.hpp"
# include "../../includes/Defines.hpp"
# include "../../includes/Includes.hpp"

class VirtualServer;
class WebServer {
    public:
        WebServer();
        ~WebServer();

        int configVServers(std::string configFilePath);
		int initConnection();
    private:
        ConfigParser configs;
		std::vector<VirtualServer> _vServers;
		Socket connection;

};

#endif
