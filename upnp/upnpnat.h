// https://www.codeproject.com/Articles/27237/Easy-Port-Forwarding-and-Managing-Router-with-UPnP
#ifndef UPNPNAT_H
#define UPNPNAT_H
#include <string>

#ifdef _WIN32
    #pragma   warning(disable:   4251)
    class __declspec (dllexport) UPNPNAT
#else
    class UPNPNAT
#endif
{
public:
	bool init();
	bool discovery(int retry = 3); // find router
	std::string& get_last_error(){ return last_error; }

	// description:      port mapping name
	// destination_ip:   internal ip address
	// port_ex:          external listening port
	// destination_port: internal port
	// protocol:         TCP or UDP
	bool add_port_mapping(const char * description, const char * destination_ip, unsigned short int port_ex, unsigned short int destination_port, const char * protocol);
    bool getExternalIP(std::string& IpOut);
private:
	bool get_description();
	bool parse_description();
	bool parse_mapping_info();
	bool tcp_connect(int& sock, const char * addr,unsigned short int port);
	std::string service_type;
	std::string describe_url;
	std::string control_url;
	std::string base_url;
	std::string service_describe_url;
	std::string description_info;
	std::string last_error;
};

#endif
