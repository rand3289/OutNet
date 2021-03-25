##  OutNet - you are the internet!

Reading further requires understanding of the following concepts:  
https://en.wikipedia.org/wiki/Network_service  
https://en.wikipedia.org/wiki/Server  
https://en.wikipedia.org/wiki/Search_engine  
https://en.wikipedia.org/wiki/App_store  
https://en.wikipedia.org/wiki/Social_network  
https://en.wikipedia.org/wiki/Peer-to-peer  
https://en.wikipedia.org/wiki/Distributed_computing  
https://en.wikipedia.org/wiki/Domain_name  
https://en.wikipedia.org/wiki/BitTorrent_tracker  

As computers get faster the need for large service providers should decrease. More can be done locally on your own computer.  Home users due to their sheer numbers have more computing power than any company.  Yet corporations maintain control of your data by building centralized services.  Use of centralized services can be avoided by distributing the equivalent of the server parts of these services among users.  Everyone can run a tiny search engine, a phone app store or a social network at home.  This model requires linking many small services in a distributed (P2P) network.

Currently domain names allow you to find services on the internet.  There are problems with using domain names for distributing services.  A list of computer names or IPs must be maintained in one location.  An example is a BitTorrent tracker.  Most users on the internet do not have a registered domain name.  You have to pay for registering most domains.  You can not point your domain name to your home IP address since it may change.  Domain names can be shut down by third parties.  I believe instead of being regulated, detrimental aspects of the internet should be voted off by majority.

Reading further requires understanding of the following concepts:  
https://en.wikipedia.org/wiki/Kazaa
https://en.wikipedia.org/wiki/Digital_signature  
https://en.wikipedia.org/wiki/Public-key_cryptography  
https://en.wikipedia.org/wiki/Communication_protocol  
https://en.wikipedia.org/wiki/Open_source  
https://en.wikipedia.org/wiki/Domain_Name_System  
https://en.wikipedia.org/wiki/Blockchain  
https://en.wikipedia.org/wiki/Distributed_hash_table  
https://en.wikipedia.org/wiki/IPv4  
https://en.wikipedia.org/wiki/GNUnet  

The alternative to private discovery protocols like Kazaa and domain names is the OutNet.  OutNet is an open source distributed service directory network protocol (peer discovery).  It is designed to find conventional or distributed (P2P) services on the internet.  Services such as web pages, game servers, ftp servers, messengers, forums, video conferences, P2P and distributed services.  Another goals of OutNet is to decentralize the internet making it resistant to control and sensorship.  OutNet provides anonymity.  Instead of a domain or a user name, a public key  is used to identify you, your services and services provided by others.  Public key is generated locally.  Your real name is no longer needed to provide or consume services on the internet.  Your IP address however will be visible to the world unless used in conjunction with a VPN.  OutNet is similar to the DNS system which allows finding services by name however it allows finding services on the internet by network protocol, service type or a public key.  OutNet is different from a blockchain and much simpler. Peers can have a partial view of the information.  There is similarity with GNUnet since OutNet provides peer discovery and authentication.  However unlike GNUnet OutNet does not provide any encryption or privacy.  The upside is there are no dependencies on other services.


## Proposed implementation
OutNet is implemented by a service with the same name that runs on your machine.  It gathers and provides a list of IPv4 addresses, corresponding port numbers and ages of nodes participating in the OutNet.  In addition, OutNet lists the types of remote services and local services you run such as your web sites, game servers and P2P services.

When OutNet starts, it tries to contact some of the known remote OutNet severs. It collects their information such as public keys and lists of services they advertise.  Local services can query OutNet to find a list of peers.  Querying OutNet returns a response described below in pseudocode:

```cpp
struct Response {
    char publicKey[256];      // RSA public key for this service
    vector<string> services;  // local services we are trying to advertise
    vector<HostInfo> list;    // list of remote (other) service instances this service is aware of. See HostInfo below.
    char signature[256];      // RSA whole message digital signature
};

struct HostInfo {             // all fields are in the network byte order
    uint32 host;              // IPv4 address
    uint16 port;              // IPv4 port number (1-65535, 0=reserved)
    uint16 age;               // in minutes (up to 45.5 days old) since the server has been seen on line
    char key[256];            // remote service' public key
    vector<string> rservices; // remote services being advertised
};
```

Reading further requires understanding of the following concepts:  
https://en.wikipedia.org/wiki/Private_network  
https://en.wikipedia.org/wiki/Network_address_translation  
https://en.wikipedia.org/wiki/Router_(computing)  
https://en.wikipedia.org/wiki/Port_(computer_networking)  
https://en.wikipedia.org/wiki/Open_port  
https://en.wikipedia.org/wiki/Universal_Plug_and_Play  
https://en.wikipedia.org/wiki/Connection-oriented_communication  
https://en.wikipedia.org/wiki/GNU_Compiler_Collection
https://en.wikipedia.org/wiki/Mingw-w64


Since one does not want to expose ALL available local services on the internet, OutNet does not discover local services.  Local services can register with OutNet or be added via configuration files.  Service descriptions have to contain routable (public/external) IP addresses instead of host names.  If OutNet determines your service is behind a NAT router and IP is a non-routable IP, it will replace your non-routable IP with it's own external routable IP when listing your service.  In addition, OutNet will open a port in the router via UPnP protocol that will allow your service to accept connections.

## Project status
This project is work in progress.  All communications are implemented but not tested.  Two major things remain: signature and UPnP protocol.  Currently project does not have ANY dependencies.  It compiles using g++ 10.2 under linux, MinGW-w64 from msys2.org on windows and Visual Studio.  Fuck Apple for requiring an Apple ID to download Command Line Tools.  This project does not run under macOS because of that.

Reading further requires understanding of the following concepts:  
https://en.wikipedia.org/wiki/Hypertext_Transfer_Protocol  
https://en.wikipedia.org/wiki/RSA_(cryptosystem)  
https://en.wikipedia.org/wiki/OpenBSD  
https://en.wikipedia.org/wiki/Representational_state_transfer  
https://en.wikipedia.org/wiki/Firewall  
https://en.wikipedia.org/wiki/Transmission_Control_Protocol  
https://en.wikipedia.org/wiki/URL  
https://en.wikipedia.org/wiki/Multicast  
https://en.wikipedia.org/wiki/Unicast  
https://en.wikipedia.org/wiki/Information_retrieval  
https://en.wikipedia.org/wiki/Wide_area_network  

* Mechanisms/protocols required to implement OutNet are HTTP 1.1, UPnP and digital signatures.
* OutNet runs as a REST service over HTTP to bypass some firewalls and network restrictions.  It can run on different port numbers that can change over time.  Your other services do not have to run over HTTP or TCP.
* OutNet can sign responses with a private key and supply a public key for signature verification.
* OutNet can deny connections to external/routable IPs if frequency of requests coming from them is high.
* OutNet has a peer list for your public local services.  They can find their peers by querying a local OutNet instance.
* OutNet should include itself in the service list.  This advertises the external/routable/public IP for other services to use.
* OutNet is capable of "opening a port in your router" via UPnP in order to be accessible from outside of your network (WAN side).
* It can "open" additional ports for your distributed services to accept connections.


Reading further requires understanding of the following concepts:  
https://en.wikipedia.org/wiki/Service_Location_Protocol  
https://en.wikipedia.org/wiki/WS-Discovery  
https://en.wikipedia.org/wiki/Simple_Service_Discovery_Protocol  
https://en.wikipedia.org/wiki/Network_Information_Service  
https://en.wikipedia.org/wiki/Multicast_DNS  
https://en.wikipedia.org/wiki/Bonjour_(software)  

## Describing services
OutNet can be based on existing standards.  For example, list of services can be based on modified service description format of DNS-SD (section 4.1.2 of rfc6763), SLP (section 4.1 of rfc2608), WS-Discovery, SSDP, NIS, mDNS, Bonjour.  Service description has to contain at least the following fields: type of service (ex: printer), actual protocol (ex:ipp), IP, port number, user defined name/description/path/attribute.  

Other possible fields: version, internet protocol (tcp / udp / multicast etc...), piority[0-9] or type of data (service/device/block/stream/file/messages/image/audio/video/conference/game/ xml/json/yaml/soap/rpc/wsdl/feed(structured data)/list/key-value/url/ binary/text/unicode/ascii/base64/uuencoded/ printer/speaker/display/ blockchain/cryptocurrency/geolocation/weather/virtualreality/time).  

Proposed maximum field lengths: priority(char[1]), service class(char[16]), protocol(char[16]), ipproto(char[3]), IP(char[15]), port(char[5]), description or path(char[32]).  Maximum service description length in the range of 96 - 128 bytes.  (DNS-SD limits service names to 15 characters.  Key-value pairs are described in rfc6763 section 6.)

Service description encoding can be limited to printable ASCII characters.  User defined name/description/path can be UTF8.

Examples: "printer:lpr:8.8.8.8:54321:2nd floor printer"  
Same device, different protocol:  "printer:ipp:8.8.8.8:12345:2nd floor printer"  


## OutNet service query parameters
OutNet can be queried to return local info and/or a filtered list of discovered remote OutNet services.  For example a query can limit the results by service type, availability of "remote public keys" or what fields are included in response.

Returned fields can be any of the following:
* local public key  (LKEY)
* current datetime (TIME)
* local service list (LSVC)
* IP   (IP)
* port (PORT)
* age  (AGE)
* remote public key (RKEY)
* remote service list (RSVC)
* signature (sign the whole message) (SIGN)
* remote service list filtered by service type/protocol or return all services


Where as fields are controlled by including them in the SELECT parameter, returned records are limited by FILTER parameters:
* local service type/protocol exact string match
* IP range or equal
* port range or equal
* age range of HostInfo records
* remote public key count (RKEYC) (one key per service is allowed but OutNet can receive different keys from other OutNet services)
* remote public key exact string match
* remote service list count (RSVCC)
* remote service type/protocol exact string match


Notes
* For numeric operators greater/less/equal allowed operands are an immediate plus one of RANGE, AGE, RKEYC, IP, PORT, RSVCC
* For operator "string equal" allowed operands are a constant string plust one of RKEY, RSVC, LSVC
* REST call (http get) Example: SELECT=2036&FILTER=RANGE_GT_500,RANGE_LT_900,RSVC_EQ_HTTP,RKEYC_GT_0
* If any of the received bit fields or query parameters are not defined, they are ignored

* Design OutNet without protocol identifiers to be less detectable and less likely to be blocked.
* Reserve age values over 65500  ex: 0xFFFE = "IO error", 0xFFFD = "duplicate signature", 0xFFFC="coming soon", 0xFFFB="broken signature", 0xFFFA="unresponsive", 0xFFF9="wrong protocol", 0xFFF8="untrusted", 0xFFF0="offline", etc...


Reading further requires understanding of the following concepts:  
https://en.wikipedia.org/wiki/SSH_(Secure_Shell)  
https://en.wikipedia.org/wiki/Elliptic_Curve_Digital_Signature_Algorithm  
https://en.wikipedia.org/wiki/Netsplit  
https://en.wikipedia.org/wiki/Botnet  

## Generating public/private keys
To generate public and private key pair we use ssh-keygen utility made available by installing OpenSSH.  By changing to service directory and issuing the following command:  
ssh-keygen -t ecdsa -b 521 -f ./key_ecdsa  
Press enter twice to skip passphrase creation.  Two files will be generated key_ecdsa and key_ecdsa.pub  Service will look for these file names upon startup.  Never share your private key!


## Securing against network splits

* Small world network properties can be used to find relatively small semi-isolated connectivity islands that might be trying to split the network and eliminate them.  This might require user interaction.
* OutNet needs to verify HostInfo records received from other OutNet instances by contacting the host in the record.  An OutNet service raiting should be incremented for each verified record and decremented for each failed verification.  Records received from OutNet services with negative rating should not be shared with other OutNet services.
* Age of each record can be taken into account when rating
* Younger records should be validated first if record age is taken into account during rating.
* New records from highly rated services should be verified first.
* OutNet services returning HostInfo records without any overlap with existing records should receive negative rating (- # of records???) After the verification the raiting will raise above 0.

## Securing agains Botnets
Health and stability of the internet should be every user's goal.  A botnet client can masquerade as a regular OutNet client.  To prevent any botnets from using this service, it is the responsibility of each user to add IP filters as problems come up.  OutNet has built in blacklist support for IPs and keys.

Reading further requires understanding of the following concepts:  
https://en.wikipedia.org/wiki/Library_(computing)  
https://en.wikipedia.org/wiki/Client_(computing)  
https://en.wikipedia.org/wiki/Whitelisting  
https://en.wikipedia.org/wiki/Media_type  
https://en.wikipedia.org/wiki/TCP_Wrappers  
https://en.wikipedia.org/wiki/Usenet  
https://en.wikipedia.org/wiki/Messaging_spam  
https://en.wikipedia.org/wiki/Network_News_Transfer_Protocol  
https://en.wikipedia.org/wiki/Web_of_trust  
https://en.wikipedia.org/wiki/Search_engine_indexing  
https://en.wikipedia.org/wiki/Authentication  
https://en.wikipedia.org/wiki/Hash_function  
https://en.wikipedia.org/wiki/Cryptographic_hash_function  
https://en.wikipedia.org/wiki/Cryptocurrency  
https://en.wikipedia.org/wiki/E-commerce  

##  Future Base Services

* Although OutNet can work by itself, it should be treated as a piece of an eco system.  Other services working with OutNet provide functionality or mitigate problems.


* OutNet query tool.  The simplest thing one can write based on OutNet is a query tool to find certain types of services and launch a client that supports that service.  For example a query tool can query the OutNet service and request a list of servers for your favorite game.  Present it for your selection in a UI.  When you click a certain server it will start the game and give it a URL (ip and port) on the command line.


* There is a need for OutNet notification service.  E-mail is not shared by the OutNet service to prevent network-wide spam.  OutNetMsg  (OutNetDirect???) receives messages addressed to your public key.  If it is from some one you trust (their public key is on your white list), it tries to open the message using the protocol/mime specified in the message.  OutNetMsg can display the message, offer to open/save the message or file, forward it to one of the services running on your system (for example, by acting as a TCP wrapper) or suggest you install a corresponding protocol handler / service.  For example it might be possible to open a Zoom conference this way.
It has to be able to manage other's public keys to be able to put them on your contact list.  It should be possible to create "groups" of public keys. It should be able to share public keys and public key lists with others and notify of updates.  It should be able to send a message or a file to the list.  Messages from public keys not on your list will be discarded.  Only direct (non-public) messages will be handled by OutNetMsg.  Public messages or publicly shared files should be handled by other services.  OutNet provides public key to IP:PORT mapping for your service to find the recepients.


* While OutNetMsg takes care of direct communication, there is a need for distributed message board service similar to twitter, parler, reddit or newsgroups.  Public messages and files exchanged by this service OutNetExchange (OutNetX OutNetShare) are not addressed to an individual but reach users subscribed on a particular subject.  Subject (thread) can have a hierarchical structure for example: local.USA.SanFrancisco.pizza  similar to a usenet group (section 3.2 in rfc977).  software.x86.linux.games.myBlaster can be used to distribute software.  Alternatively a public key (or it's hash) could become a subject allowing subscription to an individual source.  A hybrid model can be implemented where large files are distributed using BitTorrent and small files or file meta data propagate similar to usenet articles.  OutNetExchange can duplicate some usenet features while avoiding these problems:
    + Spam (anyone could post / no signatures / no ratings).
    + Need for a usenet server (not everyone has it).
    + Use of a SINGLE server (all articles have to be in one place).
    + Maximum article size varied among servers.
    + NNTP is a text based protocol.


* Another significant service is a public key rating system.  OutNetRate rating service lays at the center of trust in a distrubuted system.  You should be able to rate your interactions with owners of a public key.  Intention of this service is different than the "Web of trust".  In OutNet the key comes first and the name is secondary.  The name is not important unless verified through personal communication.  The rating does not state if you know the entity or entity's name in real life.  It rates a specific type of transaction/interaction you had.  For example an instance of a running OutNet service can be rated.  An internet purchase of an item description signed by a key can be rated.  A software/release signed by a key can be rated.  Securyty (virus/trojan free) of the content can be ensured by the rating service.  Software or content releases have to be signed by a private key of the author.  Authors's public keys in turn will be rated by users.  The way you trust in Microsoft, Google or Apple's content distribution system, individual authors have to earn your thrust in their public keys.  Rating should always contain a subject as described in OutNetExchange since an owner of a key can provide multiple services.  For example sell items or services and at the same time distribute free software.  His web store should not be rated highly just because he makes great freeware video games.  Information on how governing bodies try to regulate electronic IDs can be found here: https://en.wikipedia.org/wiki/EIDAS


* To support other distributed services OutNet provides a library for signature creation uning private key and verification using public key.  Your private key does not have to be shared with your other services.  In addition OutNet needs to provide libraries to register your service with OutNet and help you query the OutNet service.


* OutNet registration helper.  To keep things simple OutNet accepts service registrations via files dropped into a directory.  This way services can be manually added to OutNet.  There should be a registration helper that allows any service to register itself by signing it's URL with the same private key OutNet is using and sending it to registration helper via multicast.  At this point they receive OutNet's unicast address for querying.  In turn, registration helper creates a file in OutNet service directory describing the new service.  Some types of services can be discovered automatically by the registration helper.


* OutNet is a network which should be able to self-regulate.  There is a need for a security service.  Cybersecurity organizations you trust can limit detrimental activity such as botnets via releasing blacklists, ratings or software that interacts with OutNet.  If you trust a blacklist signed by a certain public key, you can include it into your collection and limit propagation of certain protocols, host information or public keys.  Blacklist mechanisms are built into OutNet. However downloading and updating black lists should be done by other programs/services for flexibility.  For example you can trust a list provided by some organization on their web site that limits content equivalent to PG-13.  This will stop propagation of certain information through your node.  Other OutNet nodes will not be affected.  If you find the list blocks your favorite content, you can add it to a white lists to override it.  Adding an item to a black list or a white list can be shared as a rating for other users.


* OutNetSearch service is used to index information (keys, subjects, content, file hashes) distributed by local services and EXCHANGE it with other search services or local distributed services.  You control what gets indexed.

* Authentication service is needed to enable seamless authentication on any conventional (server based) internet resource using your public/private key pair.  Similar to authentication via amazon/google/yahoo/facebook.  However instead of contacting them, it will contact you.

* A system to associate OutNet public keys and cryptocurrency wallets.

* A free distributed e-commerce platform can be built based on this infrastructure.
