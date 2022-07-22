// https://www.codeproject.com/Articles/27237/Easy-Port-Forwarding-and-Managing-Router-with-UPnP
#ifdef _WIN32
    #define _CRT_SECURE_NO_WARNINGS           // sprintf: This function may be unsafe
    #define _WINSOCK_DEPRECATED_NO_WARNINGS 1 // gethostbyname(), inet_addr() are deprecated in windows
    #include <winsock2.h>
    #include <ws2tcpip.h> // socklen_t
#else
    #include <signal.h>
    #include <arpa/inet.h> // inet_addr()
    #include <netinet/in.h> // sockaddr_in
    #include <unistd.h> // close(), usleep()
    #include <sys/ioctl.h>
    #define ioctlsocket(s,o,v) ioctl(s,o,v)
    #define closesocket(s) close(s)
    #define Sleep(t) usleep((t)*1000)
    #define SOCKET_ERROR   (-1)
#endif

#include <cstring> // memset()
#include <iostream>
#include <string>

#include "upnpnat.h"
#include "xmlParser.h"
#include "../log.h"

#define MAX_BUFF_SIZE 102400

static bool parseUrl(const char* url, std::string& host, unsigned short* port, std::string& path)
{
	std::string str_url=url;
	
	std::string::size_type pos1,pos2,pos3;
	pos1=str_url.find("://");
	if(pos1==std::string::npos)
	{
		return false;
	}
	pos1=pos1+3;

	pos2=str_url.find(":",pos1);
	if(pos2==std::string::npos)
	{
		*port=80;
		pos3=str_url.find("/",pos1);
		if(pos3==std::string::npos)
		{
			return false;
		}
		
		host=str_url.substr(pos1,pos3-pos1);
	}
	else
	{
		host=str_url.substr(pos1,pos2-pos1);
		pos3=str_url.find("/",pos1);
		if(pos3==std::string::npos)
		{
			return false;
		}
		
		std::string str_port=str_url.substr(pos2+1,pos3-pos2-1);
		*port=(unsigned short)atoi(str_port.c_str());
	}
	
	if(pos3+1>=str_url.size())
	{
		path="/";
	}
	else
	{
		path=str_url.substr(pos3,str_url.size());
	}	

	return true;
}


// "ST: urn:schemas-upnp-org:device:InternetGatewayDevice:1, urn:schemas-upnp-org:device:InternetGatewayDevice:2\r\n"

/******************************************************************
** Discovery Defines                                              *
*******************************************************************/
#define HTTPMU_HOST_ADDRESS "239.255.255.250"
#define HTTPMU_HOST_PORT 1900
#define SEARCH_REQUEST_STRING "M-SEARCH * HTTP/1.1\r\n"            \
                              "ST:upnp:rootdevice\r\n"             \
                              "MX: 2\r\n"                          \
                              "MAN:\"ssdp:discover\"\r\n"          \
                              "HOST: 239.255.255.250:1900\r\n"     \
                              "USER-AGENT: upnpnat\r\n"            \
                              "\r\n"
#define HTTP_OK "200 OK"
#define DEFAULT_HTTP_PORT 80


/******************************************************************
** Device and Service  Defines                                    *
*******************************************************************/

#define DEVICE_TYPE_1	"urn:schemas-upnp-org:device:InternetGatewayDevice:1"
#define DEVICE_TYPE_2	"urn:schemas-upnp-org:device:WANDevice:1"
#define DEVICE_TYPE_3	"urn:schemas-upnp-org:device:WANConnectionDevice:1"

#define SERVICE_WANIP	"urn:schemas-upnp-org:service:WANIPConnection:1"
#define SERVICE_WANPPP	"urn:schemas-upnp-org:service:WANPPPConnection:1" 


/******************************************************************
** Action Defines                                                 *
*******************************************************************/
#define HTTP_HEADER_ACTION "POST %s HTTP/1.1\r\n"                         \
                           "HOST: %s:%u\r\n"                              \
                           "SOAPACTION:\"%s#%s\"\r\n"                     \
                           "CONTENT-TYPE: text/xml; charset=\"utf-8\"\r\n"\
                           "Content-Length: %lu \r\n\r\n"

#define SOAP_ACTION  "<?xml version=\"1.0\" encoding=\"utf-8\"?>\r\n"     \
                     "<s:Envelope xmlns:s="                               \
                     "\"http://schemas.xmlsoap.org/soap/envelope/\" "     \
                     "s:encodingStyle="                                   \
                     "\"http://schemas.xmlsoap.org/soap/encoding/\">\r\n" \
                     "<s:Body>\r\n"                                       \
                     "<u:%s xmlns:u=\"%s\">\r\n%s"                        \
                     "</u:%s>\r\n"                                        \
                     "</s:Body>\r\n"                                      \
                     "</s:Envelope>\r\n"

#define PORT_MAPPING_LEASE_TIME "0" // "63072000" // two years

#define ADD_PORT_MAPPING_PARAMS "<NewRemoteHost></NewRemoteHost>\r\n"            \
                                "<NewExternalPort>%u</NewExternalPort>\r\n"      \
                                "<NewProtocol>%s</NewProtocol>\r\n"              \
                                "<NewInternalPort>%u</NewInternalPort>\r\n"      \
                                "<NewInternalClient>%s</NewInternalClient>\r\n"  \
                                "<NewEnabled>1</NewEnabled>\r\n"                 \
                                "<NewPortMappingDescription>%s</NewPortMappingDescription>\r\n"  \
                                "<NewLeaseDuration>"                             \
                                PORT_MAPPING_LEASE_TIME                          \
                                "</NewLeaseDuration>\r\n"

#define ACTION_ADD	 "AddPortMapping"
//*********************************************************************************


bool UPNPNAT::initNetwork()
{
#ifdef _WIN32 // linux does not need initialization
    WORD wVersionRequested = MAKEWORD (2, 2);
    WSADATA wsaData;
    int err = WSAStartup (wVersionRequested, &wsaData);
	if(err != 0){
		logErr() << "ERROR: WSAStartup() failed" << std::endl;
		return false;
	}
#else
	signal(SIGPIPE, SIG_IGN); // ignore SIGPIPE signal which can occur in write() or send()
#endif
	return true;
}


bool UPNPNAT::tcp_connect(int& sock, const char * host, unsigned short int port)
{
	sock=socket(AF_INET,SOCK_STREAM,0);

	// set recv() timeout //1 TODO: get seconds from LocalData::timeoutUPNP
	#ifdef _WIN32
		DWORD tv = 5*1000; // milliseconds // fucking MSFT
	#else
		timeval tv = {5,0}; // 5 seconds // tv.tv_sec = 5; tv.tv_usec = 0;
	#endif
	if ( setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(tv) ) ) { // rcve() timeout
		closesocket(sock);
		last_error = "Error setting read timeout on socket";
		return false;
	}

	struct sockaddr_in r_address;
    r_address.sin_family = AF_INET;
	r_address.sin_port=htons(port);
    r_address.sin_addr.s_addr=inet_addr(host);

	int ret = connect(sock, (const struct sockaddr *)&r_address, sizeof(struct sockaddr_in) );
	if(ret!=0){
        closesocket(sock);
		char temp[100];
		sprintf(temp, "Failed to connect to %s:%i\n", host, port);
		last_error=temp;
		return false;
	}

	return true;
}


bool UPNPNAT::discovery(int retry)
{
    int udp_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    struct sockaddr_in r_address;
    r_address.sin_family=AF_INET;
    r_address.sin_port=htons(HTTPMU_HOST_PORT);
    r_address.sin_addr.s_addr=inet_addr(HTTPMU_HOST_ADDRESS);

    bool bOptVal = true;
    int bOptLen = sizeof(bool);
    int ret=setsockopt(udp_socket, SOL_SOCKET, SO_BROADCAST, (char*)&bOptVal, bOptLen); 

    u_long val = 1;
    ioctlsocket (udp_socket, FIONBIO, &val); // non block

    std::string send_buff=SEARCH_REQUEST_STRING;
    std::string recv_buff;
    char buff[MAX_BUFF_SIZE+1]; //buff should be enough big

	for(int i=1; i<=retry; ++i)
    {
        if(i>1){ Sleep(10*1000); } // starting as a daemon.  wait for the network to come up.
        ret=sendto(udp_socket, send_buff.c_str(), send_buff.size(), 0, (struct sockaddr*)&r_address, sizeof(struct sockaddr_in));
        Sleep(1000);

        ret=recvfrom(udp_socket,buff,MAX_BUFF_SIZE,0,NULL,NULL);
		if(ret==SOCKET_ERROR){
            continue;
		}
		buff[ret] = 0; // null terminate the buffer strings

		recv_buff=buff;
        size_t pos=recv_buff.find(HTTP_OK);
        if(pos==std::string::npos){
			continue;                       //invalid response
		}

        std::string::size_type begin=recv_buff.find("http://");
        if(begin==std::string::npos){
            continue;                       //invalid response
		}

        std::string::size_type end=recv_buff.find("\r",begin);
        if(end==std::string::npos){
			continue;                       //invalid response
		}

		describe_url.assign(recv_buff,begin,end-begin);

		if(!get_description()){
			continue;
		}
		if(!parse_description()){
			continue;
		}

        log() << "Got data from Describe URL: " << describe_url.c_str() << std::endl;
        log() << "Control URL: " << this->control_url.c_str() << std::endl;
        log() << "Service Type: " << this->service_type.c_str() << std::endl;
        // log() << "Base URL: " << this->base_url.c_str() << std::endl;
        // log() << "Service URL: " << this->service_describe_url.c_str() << std::endl;
        // log() << "Description Info: " << this->description_info.c_str() << std::endl;

        closesocket(udp_socket);
		return true ;
    }

	last_error="Fail to find an UPNP NAT.\n";
    closesocket(udp_socket);
    return false;                               //no router finded 
}


bool UPNPNAT::get_description()
{ // uses: describe_url, sets: description_info
	std::string host,path;
	unsigned short int port;
	int ret=parseUrl(describe_url.c_str(),host,&port,path);
	if(!ret)
	{
		last_error="Failed to parseURl: "+describe_url;
		return false;
	}

    int sock;
	ret = tcp_connect(sock, host.c_str(),port);
	if(!ret){
		return false; // last_error is set in tcp_connect()
	}

	char request[200];
	sprintf (request,"GET %s HTTP/1.1\r\nHost: %s:%d\r\n\r\n",path.c_str(),host.c_str(),port);
	ret=send(sock, request, strlen(request), 0);

	//get description xml file
	char buff[MAX_BUFF_SIZE+1];
	int maxSize = MAX_BUFF_SIZE;
	char* buffPtr = buff;
	while ( (ret=recv(sock, buffPtr, maxSize, 0)) > 0 )
	{
		buffPtr+=ret;
		maxSize-=ret;
	}
	*buffPtr = 0;
	closesocket(sock);

	char* xml = strstr(buff,"<root ");
	if(!xml){
		last_error = "Error finding root element in xml.";
		return false;
	}
	description_info = xml;
	return true;
}


bool UPNPNAT::parse_description()
{ // uses: description_info, describe_url  sets: base_url, control_url, service_describe_url
	XMLNode node=XMLNode::parseString(description_info.c_str(),"root");
	if(node.isEmpty())
	{
		last_error="The device descripe XML file is not a valid XML file. Cann't find root element.\n";
		return false;
	}
	
	XMLNode baseURL_node=node.getChildNode("URLBase",0);
	if(!baseURL_node.getText())
	{
		std::string::size_type index=describe_url.find("/",7);
		if(index==std::string::npos )
		{
			last_error="Fail to get base_URL from XMLNode \"URLBase\" or describe_url.\n";
			return false;
		}
		base_url=base_url.assign(describe_url,0,index);
	}
	else
		base_url=baseURL_node.getText();
	
	int num,i;
	XMLNode device_node,deviceList_node,deviceType_node;
	num=node.nChildNode("device");
	for(i=0;i<num;i++)
	{
		device_node=node.getChildNode("device",i);
		if(device_node.isEmpty())
			break;
		deviceType_node=device_node.getChildNode("deviceType",0);
		if(strcmp(deviceType_node.getText(),DEVICE_TYPE_1)==0)
			break;
	}

	if(device_node.isEmpty())
	{
		last_error="Fail to find device \"urn:schemas-upnp-org:device:InternetGatewayDevice:1 \"\n";
		return false;
	}	

	deviceList_node=device_node.getChildNode("deviceList",0);
	if(deviceList_node.isEmpty())
	{
		last_error=" Fail to find deviceList of device \"urn:schemas-upnp-org:device:InternetGatewayDevice:1 \"\n";
		return false;
	}

	// get urn:schemas-upnp-org:device:WANDevice:1 and it's devicelist 
	num=deviceList_node.nChildNode("device");
	for(i=0;i<num;i++)
	{
		device_node=deviceList_node.getChildNode("device",i);
		if(device_node.isEmpty())
			break;
		deviceType_node=device_node.getChildNode("deviceType",0);
		if(strcmp(deviceType_node.getText(),DEVICE_TYPE_2)==0)
			break;
	}

	if(device_node.isEmpty())
	{
		last_error="Fail to find device \"urn:schemas-upnp-org:device:WANDevice:1 \"\n";
		return false;
	}	

	deviceList_node=device_node.getChildNode("deviceList",0);
	if(deviceList_node.isEmpty())
	{
		last_error=" Fail to find deviceList of device \"urn:schemas-upnp-org:device:WANDevice:1 \"\n";
		return false;
	}

	// get urn:schemas-upnp-org:device:WANConnectionDevice:1 and it's servicelist 
	num=deviceList_node.nChildNode("device");
	for(i=0;i<num;i++)
	{
		device_node=deviceList_node.getChildNode("device",i);
		if(device_node.isEmpty())
			break;
		deviceType_node=device_node.getChildNode("deviceType",0);
		if(strcmp(deviceType_node.getText(),DEVICE_TYPE_3)==0)
			break;
	}
	if(device_node.isEmpty())
	{
		last_error="Fail to find device \"urn:schemas-upnp-org:device:WANConnectionDevice:1\"\n";
		return false;
	}	
	
	XMLNode serviceList_node,service_node,serviceType_node;
	serviceList_node=device_node.getChildNode("serviceList",0);
	if(serviceList_node.isEmpty())
	{
		last_error=" Fail to find serviceList of device \"urn:schemas-upnp-org:device:WANDevice:1 \"\n";
		return false;
	}	

	num=serviceList_node.nChildNode("service");
	const char * serviceType;
	bool is_found=false;
	for(i=0;i<num;i++)
	{
		service_node=serviceList_node.getChildNode("service",i);
		if(service_node.isEmpty())
			break;
		serviceType_node=service_node.getChildNode("serviceType");
		if(serviceType_node.isEmpty())
			continue;
		serviceType=serviceType_node.getText();
		if(strcmp(serviceType,SERVICE_WANIP)==0||strcmp(serviceType,SERVICE_WANPPP)==0)
		{
			is_found=true;
			break;
		}
	}

	if(!is_found)
	{
		last_error="can't find  \" SERVICE_WANIP \" or \" SERVICE_WANPPP \" service.\n";
		return false;
	}

	this->service_type=serviceType;
	
	XMLNode controlURL_node=service_node.getChildNode("controlURL");
	control_url=controlURL_node.getText();

	//make the complete control_url;
	if(control_url.find("http://")==std::string::npos&&control_url.find("HTTP://")==std::string::npos)
		control_url=base_url+control_url;
	if(service_describe_url.find("http://")==std::string::npos&&service_describe_url.find("HTTP://")==std::string::npos)
		service_describe_url=base_url+service_describe_url;

	return true;	
}


bool UPNPNAT::add_port_mapping(const char * _description, const char * _destination_ip, unsigned short int _port_ex, unsigned short int _port_in, const char * _protocol)
{
	int ret;

	std::string host,path;
	unsigned short int port;
	ret=parseUrl(control_url.c_str(),host,&port,path);
	if(!ret)
	{
		last_error="Fail to parseUrl: "+control_url+"\n";
		return false;
	}
	
	int sock;
	ret=tcp_connect(sock, host.c_str(),port);
	if(!ret){
		return false; // last_error is set in tcp_connect()
    }
	
	char buff[MAX_BUFF_SIZE+1];
	sprintf(buff,ADD_PORT_MAPPING_PARAMS,_port_ex,_protocol,_port_in,_destination_ip,_description);
	std::string action_params=buff;
	
	sprintf(buff,SOAP_ACTION,ACTION_ADD,service_type.c_str(),action_params.c_str(),ACTION_ADD);
	std::string soap_message=buff;

	long unsigned int ss = soap_message.size();
	sprintf(buff,HTTP_HEADER_ACTION,path.c_str(),host.c_str(),port,service_type.c_str(),ACTION_ADD, ss);
	std::string action_message=buff;
	
	std::string http_request=action_message+soap_message;

//printf("\nSENDING:\n");
//printf(http_request.c_str());
	
	//send request
	ret=send(sock, http_request.c_str(), http_request.size(),0);

	//wait for response 			
	std::string response;
	while ( (ret=recv(sock,buff,MAX_BUFF_SIZE,0)) > 0 )
	{
		buff[ret] = 0;
		response+=buff;
	}
	closesocket(sock);

//printf("\nRESPONSE:\n");
//printf(response.c_str());

	if(response.find(HTTP_OK)!=std::string::npos){
		return true;
	}

	char temp[100];
	sprintf(temp,"Failed to add port mapping (%s/%s)\n",_description,_protocol);
	last_error=temp;
	last_error += response;
	return false;
}


bool UPNPNAT::getExternalIP(std::string& IpOut, uint32_t& localIP){
#define EXTERNAL_IP_ACTION	"GetExternalIPAddress"
#define EXTERNAL_IP_PARAMS  " " // "<NewExternalIPAddress></NewExternalIPAddress>\r\n"

	int ret;

	std::string host,path;
	unsigned short int port;
	ret=parseUrl(control_url.c_str(),host,&port,path);
	if(!ret)
	{
		last_error="Fail to parseUrl: "+control_url;
		return false;
	}
	
	int sock;
	ret=tcp_connect(sock, host.c_str(),port);
	if(!ret){
		return false; // last_error is set in tcp_connect()
    }
	
	// get local IP
	localIP = 0;
	sockaddr_in addr;
	socklen_t addrSize = sizeof(addr);
	if( 0==getsockname(sock, (sockaddr*) &addr, &addrSize ) ){
		localIP = addr.sin_addr.s_addr; // in network byte order
	}

	char buff[MAX_BUFF_SIZE+1];
	sprintf(buff, SOAP_ACTION, EXTERNAL_IP_ACTION, service_type.c_str(), "", EXTERNAL_IP_ACTION);
	std::string soap_message=buff;

	long unsigned int ss = soap_message.size();
	sprintf(buff,HTTP_HEADER_ACTION,path.c_str(),host.c_str(),port,service_type.c_str(),EXTERNAL_IP_ACTION, ss);
	std::string action_message=buff;
	
	std::string http_request=action_message+soap_message;

//printf("\nSENDING:\n");
//printf(http_request.c_str());
	
	//send request
	ret=send(sock, http_request.c_str(), http_request.size(),0);

	//wait for response 			
	std::string response;
	while ( (ret=recv(sock,buff,MAX_BUFF_SIZE,0)) > 0 )
	{
		buff[ret] = 0;
		response+=buff;
	}
	closesocket(sock);

//printf("\nRESPONSE:\n");
//printf(response.c_str());

	if(response.find(HTTP_OK)!=std::string::npos){
		long unsigned int index = response.find("<?xml ");
		if( index == std::string::npos ){
			last_error = "Can't find XML in router's reponse.";
			return false;
		}
        XMLResults result;
		XMLNode envelope=XMLNode::parseString(&response[index], "s:Envelope", &result);
		XMLNode body = envelope.getChildNode(); // TODO: iterate to find NewExternalIPAddress node?
		XMLNode resp = body.getChildNode();
		XMLNode ipNode = resp.getChildNode("NewExternalIPAddress");
	    if(ipNode.isEmpty())
	    {
		    last_error="Can't find NewExternalIPAddress element in xml: ";
			last_error+= XMLNode::getError(result.error);
		    return false;
	    }
        IpOut = ipNode.getText();
		return true;
	}

	last_error="Failed to get external IP";
	return false;
}
