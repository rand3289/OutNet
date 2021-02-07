OutNet - you are the internet!

As computers get faster the need for centralized services should decrease, yet major corporations push services into clouds to maintain control.  Enslavement by facebook, twitter, instagram, search engines and cloud providers can be avoided by distributing the equivalent of the server parts of these services among users.  Everyone can run a tiny search engine or a tiny cloud at home.  How will the other users find your services, you ask?  The old way is to use domain names.  Domain names allow others to find your services on the internet.  The problem with using domain names is that they can be shut down by other parties. Governments or private coorporations can decide which domain names have the right to exist and which do not.  The alternative is the OutNet.  Instead of a name a public key from a locally generated public/private key pair is used to identify services.

OutNet is a distributed directory for distributed internet services running over TCP/IPv4.  It is designed to find distributed services on the internet.  Services such as web pages, ftp servers, messengers, forums, video conferences, "Distributed Rating System" and others.  OutNet helps you as the service provider to find other nodes providing same type of service.  However, it is your responsibility as a service provider to make your service work with other nodes.

This document describes two services OutNetList and OutNetDir.  Both services run over HTTP on different port numbers.  Port numbers can change.  Mechanisms/protocols required to implement these services are HTTP 1.1, UPnP and digital signatures.

OutNetList provides a list of IPv4 addresses, corresponding port numbers and ages of nodes participating in the OutNet.  Age is a number of minutes since the server has last been seen on line.  This 16 bit value allows for nodes to be up to 45.5 days old. In addition, OutNetList provides information about the local OutNetDir service.

OutNetDir and is a service that lists other types of distributed services running locally.  OutNetDir does not have to run on the same machine/IP where OutNetList is running.  One can choose not to run any local services other than OutNetList and disable OutNetDir.

Querying OutNetList port will return the following information:

struct Response {
    string signature; // PGP/GPG signature
    string publicKey; // PGP/GPG public key
    SignedMessage msg; // message sigened by digital signature
}; // no e-mail field to prevent network-wide spam!

struct SignedMessage{
    uint32 dateTime;         // time since epoch in seconds (code for overflow using modulo arithmetics)
    string name;             // Name of person or entity. Public key hashes can be used instead.  MD5 might work since hashing a key.
    HostPort OutNetDir;      // points to the local OutNetDir service or filled with zeros.
    HostPort OutNetDirLocal; // is it needed??? non-routable ip of the OutNetDir service (local/behind the router 192.168.x.x or 10.x.x.x)
    vector<HostPort> list;   // list of remote OutNetList instances
};

struct HostPort { // all fields are in the network byte order
    uint32 host;
    uint16 port;
    uint16 age; // in minutes (up to 45.5 days old) (reserve the values over 65,500 ???)
};


* Response data is binary and is encoded using application octet-stream mime type
* Design it to be undetectable by ISPs so it less likely to be blocked.
* change "running locally" in the text above to "run by the same individual or organization"


* OutNetDir and OutNetList can sign the response with their private key and supply a public key for signature verification.  Response must contain a GMT date/time as part of the signed response.  Do not provide "time zone" information to increase privacy.  As another privacy measure, OutNetList and OutNetDir are not required to sign their responses.


* When OutNetList starts, it tries to contact all known remote OutNetList and OutNetDir severs. It collects their information and public keys.  Remote OutNetList servers should have the ability to query your server back over the same connection when contacted and send only a "delta" of the information.
* Should OutNetList allow queries by service type?
* Should OutNetList support querying by timestamp (all information newer than t seconds)?
* Should OutNetList allow queries by "public key" or "public key hash"?  Make a separate service OutNetID for searching "public key <--> IP" mapping?
* OutNetList becomes a peer list for your other local distributed services.  They can find peers by querying OutNetList.


* OutNetList and OutNetDir are capable of "opening a port in your router" via UPnP in order to be accessible from outside of your network.
* In addition OutNetDir is capable of "opening" additional ports for your distributed services to accept connections.
* OutNetList (and OutNetDir?) can also tell your services the "real/external" IP.
* To support other distributed services OutNet provides a library for signature creation and verification using private and public key. (Your private key does not have to be shared with your other services???)


* OutNetList and OutNetDir run over HTTP to bypass some firewalls and network restrictions.  Your distributed services do not have to run over HTTP.
* OutNetDir does not support HTTP proxies since being behind a proxy prohibits someone from connecting to you.  OutNetList should have the capability to connect through proxies in the future since it supports exchange of information in addition to queries.


********************************************  Other Base Services **************************************

* As described above, OutNet is able to advertise many existing services such as your web site, ftp, ssh and some P2P software, however there is a need for OutNet notification service.  OutNetMsg receives messages addressed to your public key.  If it is from some one you trust (their public key is on your white list), it tries to open the message using the protocol/mime specified in the message.  OutNetMsg can display the message, offer to open the message or file, forward it to one of the services running on your system (for example, by acting as a TCP wrapper) or suggest you install a corresponding protocol handler / service.  For example it might be possible to open a Zoom session this way.  It has to be able to manage other's public keys to be able to put them on your contact list.  Messages from public keys not on your list will be discarded.  Only direct messages will be handled by OutNetMsg.  Messages distributed to multiple recepients have to be handled by corresponding services.


* Another significant service is a public key rating system.  You should be able to rate your interactions with owners of a public key.  It might be possible to combine the rating sytem service with OutNetMsg since both manage public keys.  Intention of this service is different than the "Web of trust" (https://en.wikipedia.org/wiki/Web_of_trust).  In OutNet the key comes first and the name is secondary.  The name is not important unless verified through personal communication.  The rating does not state if you know the entity in real life or what type of transaction/interaction you had maintaining privacy.
