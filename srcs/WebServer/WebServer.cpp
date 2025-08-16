#include "WebServer.hpp"

WebServer::WebServer() {

}

WebServer::~WebServer() {

}

int WebServer::configVServers(std::string configFilePath) {
    this->configs.setConfigFilePath(configFilePath);
	this->configs.initConfig();
	_vServers = this->configs.getVServers();
    return 0;
}

int WebServer::initConnection() {
	connection.setVServers(_vServers);
	connection.startServer();
	return (0);
}