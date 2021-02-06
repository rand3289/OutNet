OutNet - you are the internet!

As computers get faster the need for centralized services should decrease, yet major corporations push services into clouds to maintain control.  Enslavement by facebook, twitter, instagram, search engines and cloud providers can be avoided by distributing the equivalent of the server parts of these services among users.  Everyone can run a tiny search engine or a tiny cloud at home.  How will the other users find your tiny cloud, you ask?  The old way is to use domain names.  Domain names allow others to find your services on the internet.  The problem with using domain names is that they can be shut down by other parties. Governments or private coorporations can decide which domain names have the right to exist and which do not.  The alternative is the OutNet.

OutNet is a distributed directory for distributed internet services running over TCP/IPv4.  It is designed to find distributed services on the internet.  Services such as "Distributed Rating System", "Distributed Messaging System", "Distributed BBS" and others.  OutNet helps you as the service provider to find other nodes providing same type of service.  It is your responsibility as a service provider to make your service work with other nodes.

This document describes two services OutNetList and OutNetDir.  Both services run over HTTP on different port numbers.

OutNetList provides a list of IPv4 addresses, corresponding port numbers and ages of nodes participating in the OutNet.  Age is a number of minutes since the server has last been seen on line.  This 16 bit value allows for nodes to be up to 45.5 days old. In addition, OutNetList provides information about the local OutNetDir service.

OutNetDir and is a service that lists other types of distributed services running locally.  OutNetDir does not have to run on the same machine/IP where OutNetList is running.  One can choose not to run any local services other than OutNetList and disable this service.

Querying OutNetList port will return the following structure:

struct ResponsePocket {
    vector<HostPort> list;
};

struct HostPort { // all fields are in the network byte order
    uint32 host;
    uint16 port;
    uint16 age;
};

Fist HostPort in the OutNetList response contains the address of the OutNetDir service.

* Response data is encoded using application octet-stream mime type
* Design it to be transparent and undetectable by ISPs so it can not be blocked.
* OutNetDir (or should it be OutNetList?) should allow queries by service type
* change "running locally" in the text above to "run by the same individual or organization"
* how will the services provide IDs (public keys) of the service?
* OutNetList is capable of "opening a port in your router" via UPnP in order to be accessible from outside of your network.
* OutNetDir is capable of "opening ports in your router" via UPnP in order for your distributed services to accept connections.
* OutNetList becomes a peer list for your local distributed services.  They can find peers by querying OutNetList.
* Make a service capable of searching for a "public key <-> IP" mapping.  Or "public key hash" to IP mapping.
* To support other distributed services we need a library that does public key encryption/signatures or/and open a TLS connection using those keys?
* OutNet notification service (OutNetBootstrap) that offers to install a corresponding distributed service for P2P communication/file sharing/stream sharing(based on protocol/mime). It has to be able to display/manage other's public keys. (public key = URL)
* Although OutNetList and OutNetDir run over HTTP to bypass some firewalls and network restrictions, your distributed services do not have to run over HTTP.  However OutNetList and OutNetDir do not support HTTP proxies since being behind a proxy prohibits someone from connecting to you.  This makes you a ghost on OutNet since you will be using other people's resources without providing back.

