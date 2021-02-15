##  OutNet - you are the internet!

As computers get faster the need for centralized services should decrease, yet major corporations push services into clouds to maintain control.  Enslavement by facebook, twitter, instagram, search engines, cloud providers and software stores can be avoided by distributing the equivalent of the server parts of these services among users.  Everyone can run a tiny search engine or a tiny cloud at home.  How will the other users find your services?  Domain names allow others to find your services on the internet.  The problem with using domain namesor host names is that they can be shut down by third parties.  


The alternative is the OutNet.  OutNet is a distributed directory designed to find other conventional or distributed services on the internet.  Services such as web pages, ftp servers, messengers, forums, video conferences, P2P and distributed services.  Another goals of OutNet is to decentralize the internet making it resistant to control and sensorship.  A small bonus is you no longer have to pay to maintain your domain name.  OutNet provides anonymity.  Instead of a domain or a user name, a public key from a locally generated public/private key pair is used to identify you, your services and services provided by others.  Your real name is no longer needed to provide or consume services on the internet.  Your IP address however will be visible to the world unless used in conjunction with a VPN.


## Proposed services

This document describes two services OutNetList and OutNetDir.  Both services run on your machines.  One can choose not to run OutNetDir or any local services other than OutNetList.  

OutNetDir is a service that lists other types of services you run such as your web site, P2P or distributed services.  OutNetDir can be based on existing standards.  One such possibility is a modified service URL format in "Service Location Protocol" (section 4.1 of rfc2608) (https://en.wikipedia.org/wiki/Service_Location_Protocol).  OutNet URLs have to contain routable (public/external) IP addresses instead of host names.  In addition, one does not want to automatically expose all available local network services on the internet.  Example: service:msg:http://8.8.8.8:4321/messenger?pub_key_hash=  


OutNetList provides a list of IPv4 addresses, corresponding port numbers and ages of nodes participating in the OutNet.  Age is a number of minutes since the server has last been seen on line.  In addition, OutNetList provides information about the local OutNetDir service.  When OutNetList starts, it tries to contact all known remote OutNetList and OutNetDir severs. It collects their information and public keys.  Remote OutNetList servers should have the ability to query your server back over the same connection when contacted and send only a "delta" of the information.  Querying OutNetList will return a SignedResponse or an UnsignedResponse described below in pseudocode:

```cpp
struct SignedResponse {
    string publicKey;  // PGP/GPG public key
    string signature;  // PGP/GPG signature
    SignedMessage msg; // message sigened by the digital signature
};

struct SignedMessage {
    uint32 dateTime;         // time since epoch in seconds of when this message was created
    string name;             // Name of person or entity. Public key hashes can be used instead.  MD5 might work since hashing a key.
    HostPort OutNetDir;      // points to the OutNetDir service or filled with zeros. (port=0 if unused)
    vector<HostPort> list;   // list of remote OutNetList instances
};

struct UnsignedResponse {
    HostPort OutNetDir;      // points to the local OutNetDir service or filled with zeros.
    vector<HostPort> list;   // list of remote OutNetList instances
};

struct HostPort { // all fields are in the network byte order
    uint32 host;  // IPv4 address
    uint16 port;  // IPv4 port number (1-65535, 0=reserved)
    uint16 age;   // in minutes (up to 45.5 days old) (reserve the values over 65,500   ex: 0xFFFE = "coming soon", 0xFFFF = "do not use")
};
```


* OutNetList and OutNetDir run over HTTP to bypass some firewalls and network restrictions.  They can run on different port numbers that can change over time.  Your other services do not have to run over HTTP.
* OutNetList Response data is binary and is encoded using application octet-stream mime type
* Design OutNet without protocol identifiers (packet/stream format signature) to be less detectable and less likely to be blocked.
* OutNetDir does not support HTTP proxies since being behind a proxy prohibits someone from connecting to you and your services.  OutNetList should have the capability to connect through proxies if it supports exchange of information in addition to queries.
* OutNetDir should avoid including itself in the service list, however OutNetList should be included if available.
* OutNetDir does not have to run on the same machine/IP where OutNetList is running.
* OutNetList can deny connections to outside IPs if frequency of requests coming from them is high.
* There is NO e-mail field shared by any services to prevent network-wide spam. OutNetMsg should be used (see below).
* OutNetDir and OutNetList should sign the response with their private key and supply a public key for signature verification.  A signed response must contain a GMT date/time stamp.


* Mechanisms/protocols required to implement these services are HTTP 1.1, UPnP and digital signatures.
* OutNetList and OutNetDir are capable of "opening a port in your router" via UPnP in order to be accessible from outside of your network.
* In addition OutNetDir is capable of "opening" additional ports for your distributed services to accept connections.
* OutNetList (and OutNetDir?) can also tell your other local services the "routable/public/external" IP.
* OutNetList has a peer list for your other local distributed services.  They can find peers by querying OutNetList.
* To support other distributed services OutNet provides a library for signature creation and verification using private and public key. (Your private key does not have to be shared with your other services)


##  Other Base Services

* There is a need for OutNet notification service.  OutNetMsg (OutNetDirect???) receives messages addressed to your public key.  If it is from some one you trust (their public key is on your white list), it tries to open the message using the protocol/mime specified in the message.  OutNetMsg can display the message, offer to open/save the message or file, forward it to one of the services running on your system (for example, by acting as a TCP wrapper) or suggest you install a corresponding protocol handler / service.  For example it might be possible to open a Zoom session this way.
It has to be able to manage other's public keys to be able to put them on your contact list.  It should be possible to create "groups" of public keys. It should be able to share public keys and public key lists with others and notify of updates.  Messages from public keys not on your list will be discarded.  Only direct (non-public) messages will be handled by OutNetMsg.  Public messages or publicly shared files should be handled by other services.  HTTPS can be used to ensure end-to-end encryption (no https proxies).


* While OutNetMsg takes care of direct communication, there is a need for distributed message board service similar to twitter, parler, reddit or newsgroups.  Public messages and files exchanged by this service (OutNetExchange/OutNetShare) are not addressed to an individual but reach users subscribed on a particular subject.  Subject (thread) can have a hierarchical structure for example: local.USA.SanFrancisco.pizza  similar to a usenet group (section 3.2 in rfc977).  software.x86.linux.games.myBlaster can be used to distribute software.  Alternatively a public key (or it's hash) could become a subject allowing subscription to an individual source.
OutNetShare tries to duplicate usenet while removing some of it's problems:  Need for a usenet server (not everyone has it).  Use of a SINGLE server (all articles had to be in one place).  Variable article size limits.  NNTP is a text based protocol.  Spam (anyone could post / no signatures / no ratings).


* Another significant service is a public key rating system.  Rating service lays at the center of trust in a distrubuted system.  You should be able to rate your interactions with owners of a public key.  Intention of this service is different than the "Web of trust" (https://en.wikipedia.org/wiki/Web_of_trust).  In OutNet the key comes first and the name is secondary.  The name is not important unless verified through personal communication.  The rating does not state if you know the entity or entity's name in real life.  It rates a specific type of transaction/interaction you had.  For example an instance of a running OutNetList service can be rated.  An internet purchase of an item description signed by a key can be rated.  A software/release signed by a key can be rated.  Securyty (virus/trojan free) of the content can be ensured by the rating service.  Software or content releases have to be signed by a private key of the author.  Authors's public keys in turn will be rated by users.  The way you trust in Microsoft, Google or Apple's content distribution system, individual authors have to earn your thrust in their public keys.  Rating should always contain a subject as described in OutNetExchange since an owner of a key can provide multiple services. For example sell items or services and at the same time distribute free software.  His web store should not be rated highly just because he makes great freeware video games.


* OutNetSearch service is used to index information (subjects+content) distributed by local services and share it with other search services or local distributed services.  For example, query OutNetList by service type / by timestamp (all information newer than t seconds) / by other OutNetList service "public key" or "public key hash".  The later will allow you to find/access the services a particular key owner provides.


* Authentication service is needed to enable seamless authentication on any conventional (server based) internet resource using your public/private key pair.  Similar to authentication via amazon/google/yahoo/facebook.


* Cryptocurrency payment system?
