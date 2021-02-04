OutNet - you are the internet!

As computers get faster the need for centralized services should decrease, yet major corporations push services into clouds to maintain control.  Enslavement by facebook, twitter, instagram, search engines and cloud providers can be avoided by distributing the equivalent of the server parts of these services among users.  Everyone can run a tiny search engine or a tiny cloud at home.  How will the other users find your tiny cloud, you ask?  The old way is to use domain names.  Domain names allow others to find your services on the internet.  The problem with using domain names is that they can be shut down by other parties. Governments or private coorporations can decide which domain names have the right to exist and which do not.  The alternative is the OutNet.

OutNet is a distributed directory for distributed internet services running over TCP/IPv4.  It is designed to find distributed services on the internet.  Services such as "Distributed Rating System", "Distributed Messaging System", "Distributed BBS" and others.  OutNet helps you as the service provider to find other nodes providing same type of service.  It is your responsibility as a service provider to make your service work with other nodes.

This document describes two services OutNetList and OutNetDir.  Both services run over HTTP on different port numbers.

"OutNet List" provides a list of IPv4 addresses, corresponding port numbers and ages of nodes participating in the OutNet.  Age is a number of minutes since the server has last been seen on line.  This 16 bit value allows for nodes to be up to 45.5 days old. In addition, "OutNet List" provides information about a local "OutNet Dir" service.

"OutNet Dir" and is a service that lists other types of distributed services running locally.  "OutNet Dir" does not have to run on the same machine/IP where "OutNet List" is running.  One can choose not to run any local services other than "OutNet List" and disable this service.

Querying "OutNet List" port will return the following structure:

struct ResponsePocket {
    ushort dirPort; // port for querying "OutNet Dir" or 0 if it is not running
    vector<HostPort> list;
};

struct HostPort { // all fields are in the network byte order
    uint32 host;
    uint16 port;
    uint16 age;
};


* Fist HostPort in the OutNetList response can contain the address of the OutNetDir
* Response data is encoded using application octet-stream mime type
* Desig it to be transparent and undetectable by your ISP.
* OutNetDir (or should it be OutNetList?) should allow queries by service type
* change "running locally" to "run by the same individual or organization"
* how will the services provide IDs (public keys) of the service?