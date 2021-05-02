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
https://en.wikipedia.org/wiki/Dynamic_DNS  

As computers get faster the need for large service providers should decrease. More can be done locally on your own computer.  Home users due to their sheer numbers have more computing power than any company.  Yet corporations maintain control of your data by building centralized services.  Use of centralized services can be avoided by distributing the equivalent of the server parts of these services among users.  Everyone can run a tiny search engine, a phone app store or a social network at home.  This model requires linking many small services in a distributed (P2P) network.

Participating in OutNet or projects like OutNet is the only way for you to take control over your information, your content and monetization of your content.  It is the only way of preventing large companies from owning your data.  With OutNet, you get to host your videos, your posts, your pictures and no one can tell you what to do, shut you down or take it away from you.  OutNet does not hide any illegal activities.  It only stops other from gaining control over you.

Currently domain names allow you to find services on the internet.  Using domain names is not suitable for distributing services.  When domain names are used, resourses must be maintained in a few central locations.  An example is a BitTorrent tracker.  Another problem is that most users on the internet do not have a registered domain name.  You have to pay for registering most domains.  You can not point your domain name to your home IP address since it may change except with dynamic DNS.  Domain names can be shut down by third parties.  With OutNet, instead of being regulated, detrimental aspects of the internet can be voted off by majority and get lost within the noise.


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

OutNet is an alternative to "private/regulated/controlled" discovery protocols like Kazaa or domain names.  OutNet is a free and open source distributed service directory network protocol (peer discovery).  It is designed to find conventional or distributed (P2P) services on the internet.  Services such as web pages, game servers, ftp servers, messengers, forums, video conferences, P2P and distributed services.  Another goals of OutNet is to decentralize the internet making it resistant to control and sensorship.  OutNet provides anonymity.  Instead of a domain or a user name, a public key is used to identify you, your services and services provided by others.  Public key is generated locally.  Your real name is no longer needed to provide or consume services on the internet.  Your IP address however will be visible to the world unless used in conjunction with a VPN.  OutNet is similar to the DNS system which allows finding services by name, however instead it allows finding services on the internet by a network protocol name, a service type or a public key.  OutNet is different from a blockchain and much simpler. Peers can have a partial view of the information.  There is similarity with GNUnet since OutNet provides peer discovery and authentication.  However unlike GNUnet OutNet does not provide any encryption or privacy.  The upside is there are NO dependencies on external components.


## Project status
OutNet is written in C++ 20. Project does not have ANY external dependencies.  Everything is built-in (source code available from ONE git repository).  Most features are implemented and currently being tested.  OutNet was started in February 2020 by a single developer.  Its most recent version is 0.1
It compiles using g++ 10.2 under linux, MinGW-w64 from msys2.org on windows and Visual Studio.  Visual Studio build is currently broken.  To compile type "make" in OutNet directory.  To compile tests, type "make" in OutNet/test directory.  "test" directory contains sample code for querying the OutNet service.

Apple requires an Apple ID to download Command Line Tools.  Apple ID registration requires submission of your phone number.  This project has not been ported to macOS for this reason.

OutNet's home is https://github.com/rand3289/OutNetMsg  
Other OutNet based services currently being implemented are available here:  
https://github.com/rand3289/OutNetMsg  


## Implementation
OutNet is implemented by a service with the same name that runs on your machine.  It gathers and provides a list of IPv4 addresses, corresponding port numbers and ages of nodes participating in the OutNet.  In addition, OutNet lists the types of remote services and local services you run such as your web sites, game servers and P2P services.

When OutNet starts, it tries to contact some of the known remote OutNet severs. It collects their information such as public keys and lists of services they advertise.  Local services can query OutNet to find a list of peers.  Querying OutNet returns a response that contains it's public key, a list of local services OutNet is advertising, a list of remote OutNet services it knows and services they advertise.  Response is signed by the private key.


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
https://en.wikipedia.org/wiki/Repository_(version_control)


Since one does not want to expose ALL available local services on the internet, OutNet does not discover local services.  Local services can register with OutNet or be added via configuration files.  Service descriptions visible to the world have to contain routable (public/external) IP addresses instead of host names.  If OutNet determines your service is behind a NAT router and IP is a non-routable IP, it will replace your non-routable IP with it's own external routable IP when listing your service.  In addition, OutNet will open a port in the router via UPnP protocol that will allow your service to accept connections.


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


* OutNet runs as a REST service over HTTP to bypass some firewalls and network restrictions.  It can run on different port numbers that can change over time.  Your other services do not have to run over HTTP or TCP.  The advantages of using http interfaces and presenting user interfaces as web applications however is that they ALL can be linked into one eco system.  Any of your services built on top of OutNet know about other services by querying OutNet service.  Any service can send messages to OutNetMsg or post notifications to OutNetTray.  Browser based OutNetMsg will provide links to all registered OutNet services that serve /favicon.ico via HTTP GET request.

* Your public local services can find their peers by querying a local OutNet instance.
* OutNet is capable of "opening a port in your router" via UPnP in order to be accessible from outside of your network (WAN side).
* It can "open" additional ports for your distributed services to accept connections.
* OutNet can sign responses with a private key and supply a public key for signature verification.
* OutNet can deny connections to external/routable IPs if frequency of requests coming from them is high.
* OutNet has built in blacklist support for filtering selected IPs and keys.
* Mechanisms/protocols used to implement OutNet are HTTP 1.1, UPnP and digital signatures.
* OutNet provides libraries to help you query the OutNet service and register your service with OutNet.
* To support your other services OutNet provides a library for signature creation and verification.  Your private key does not have to be shared with your other services.


Reading further requires understanding of the following concepts:  
https://en.wikipedia.org/wiki/Graphical_user_interface  
https://en.wikipedia.org/wiki/Library_(computing)  
https://en.wikipedia.org/wiki/Client_(computing)  
https://en.wikipedia.org/wiki/Whitelisting  
https://en.wikipedia.org/wiki/Media_type  
https://en.wikipedia.org/wiki/TCP_Wrappers  
https://en.wikipedia.org/wiki/Usenet  
https://en.wikipedia.org/wiki/Hyperlink  
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

* OutNet should be treated as a piece of an eco system.  It is the backbone on top of which all other services are built.  The simplest service one can write based on OutNet is a query tool to find certain types of services and launch a client that supports that service.  For example a query tool can get list of servers for your favorite game.  Present it to you in a GUI.  When you click a certain server it will start the game and give it a URL (ip and port) on the command line.


* There is a need for OutNet notification service.  E-mail is not shared by the OutNet service to prevent network-wide spam.  OutNetMsg receives messages addressed to your public key.  If it is from some one you trust (their public key is on your white list), it tries to open the message using the protocol/mime specified in the message.  OutNetMsg can display the message, offer to open/save the message or file, forward it to one of the services running on your system (for example, by acting as a TCP wrapper) or suggest you install a corresponding protocol handler / service.  For example it can open a Zoom conference.
It has to be able to manage other's public keys to be able to put them on your contact list.  It should be possible to create "groups" of public keys. It should be able to share public keys and public key lists with others and notify of updates to the list.  It should be able to send a message or a file to the list.  There should be a mechanism to request that your public key is added to someone's friend list or group list.  Messages from public keys not on your list will be discarded.  Only direct (non-public) messages will be handled by OutNetMsg.  Public messages or publicly shared files should be handled by other services.  OutNet service provides public key to IP:PORT mapping for OutNetMsg to find the recepients.  See https://github.com/rand3289/OutNetMsg for more info.


* While OutNetMsg takes care of direct communication, there is a need for distributed message board service similar to twitter, reddit, facebook or newsgroups.  Public messages and files exchanged by this service OutNetExchange (alternatively OutNetX or OutNetShare) are not addressed to an individual but reach users subscribed on a particular subject or key.  Subject (thread) can have a hierarchical structure for example: local.USA.SanFrancisco.pizza  similar to a usenet group (section 3.2 in rfc977).  software.x86.linux.games.myBlaster can be used to distribute software.  Alternatively a public key could become a subject allowing subscription to an individual source.  A file hash becomes a URL where documents are hyperlinked by using their hash.  A hybrid model can be implemented where large files are distributed using BitTorrent and small files or file metadata/hashes propagate similar to usenet articles.  OutNetExchange can duplicate some usenet features while avoiding these problems:
    + Spam (anyone could post / no signatures / no ratings).
    + Need for a usenet server (not everyone has it).
    + Use of a SINGLE server (all articles have to be in one place).
    + Maximum article size varied among servers.
    + NNTP is a text based protocol.


* Another significant aspect is a rating system.  Ratings is the basis of trust in a distrubuted system.  Two things things that need to be rated are data (text/images/video) and public keys.  

Each document, image or media file can be addressed by it's hash.  All one needs to increase its rating is to make that piece of data local.  By making it local you store it on your disk, allow others to find it by its hash and download it.  Content rating count (number of seeds) should be inversly proportional to it's size: larger files will always have less seeds however they should be highly prized.  On the other side small files will require more seeds to increase it's rating since the "price" of storing them on the hard drive is small.

You should be able to rate your interactions with owners of a public key.  Intention of this rating is different than the "Web of trust".  In OutNet the key comes first and the name is secondary.  The name is not important unless verified through personal communication.  The rating does not state if you know the entity or entity's name in real life.  It rates a specific type of transaction/interaction you had.  For example an instance of a running OutNet service can be rated.  An internet purchase of an item description signed by a key can be rated.  A software/release signed by a key can be rated.  Securyty (virus/trojan free) of the content can be ensured by the rating service.  Software or content releases have to be signed by a private key of the author.  Authors's public keys in turn will be rated by users.  The way you trust in Microsoft, Google or Apple's content distribution system, individual authors have to earn your thrust in their public keys.  Rating should always contain a subject as described in OutNetExchange since an owner of a key can provide multiple services.  For example sell physical items or services and at the same time distribute free software.  His web store should not be rated highly just because he makes great freeware video games.

Information on how governing bodies try to regulate electronic IDs can be found here: https://en.wikipedia.org/wiki/EIDAS


* OutNetSearch service is used to index information (keys, subjects, content, file hashes) distributed by local services and EXCHANGE it with other search services or local distributed services.  You control what gets indexed.


* OutNet is a network which should be able to self-regulate.  There is a need for a security service.  Cybersecurity organizations you trust can limit detrimental activity such as botnets via releasing blacklists, ratings or software that interacts with OutNet.  If you trust a blacklist signed by a certain public key, you can include it into your collection and limit propagation of certain protocols, host information or public keys.  Blacklist mechanisms are built into OutNet. However downloading and updating black lists should be done by other programs/services for flexibility.  For example you can trust a list provided by some organization on their web site that limits content equivalent to PG-13.  This will stop propagation of certain information through your node.  Other OutNet nodes will not be affected.  If you find the list blocks your favorite content, you can add it to a white lists to override it.  Adding an item to a black list or a white list can be shared as a rating for other users.


* Authentication service is needed to enable seamless authentication on any conventional (server based) internet resource using your public/private key pair.  Similar to authentication via amazon/google/yahoo/facebook.  However instead of contacting them, it will contact you.


* A system to associate OutNet public keys and cryptocurrency wallets. Could be done through OutNetMsg.


* A free distributed e-commerce platform can be built based on this infrastructure.


Reading further requires understanding of the following concepts:  
https://en.wikipedia.org/wiki/Service_Location_Protocol  
https://en.wikipedia.org/wiki/WS-Discovery  
https://en.wikipedia.org/wiki/Simple_Service_Discovery_Protocol  
https://en.wikipedia.org/wiki/Network_Information_Service  
https://en.wikipedia.org/wiki/Multicast_DNS  
https://en.wikipedia.org/wiki/Bonjour_(software)  

## Describing services
OutNet can be based on existing standards.  For example, list of services can be based on modified service description format of DNS-SD (section 4.1.2 of rfc6763), SLP (section 4.1 of rfc2608), WS-Discovery, SSDP, NIS, mDNS, Bonjour.  Service description has to contain at least the following fields: type of service (ex: printer), internet protocol (tcp/udp), actual protocol (ex:ipp), IP, port number, user defined name/description/path/attribute.  

Other possible fields: version, piority[0-9], type of data (service/device/block/stream/file/messages/image/audio/video/conference/game/ xml/json/yaml/soap/rpc/wsdl/feed(structured data)/list/key-value/url/ binary/text/unicode/ascii/base64/uuencoded/ printer/speaker/display/ blockchain/cryptocurrency/geolocation/weather/virtualreality/time).  

Proposed maximum field lengths: priority(char[1]), service class(char[16]), protocol(char[16]), ipproto(char[3]), IP(char[15]), port(char[5]), description or path(char[32]).  Maximum service description length in the range of 96 - 128 bytes.  (DNS-SD limits service names to 15 characters.  Key-value pairs are described in rfc6763 section 6.)

Service description encoding can be limited to printable ASCII characters.  User defined name/description/path can be UTF8.

Examples: "printer:lpr:8.8.8.8:54321:2nd floor printer"  
Same device, different protocol:  "printer:ipp:8.8.8.8:12345:2nd floor printer"  


## OutNet service query parameters
OutNet can be queried to return local info and/or a filtered list of discovered remote OutNet services.  For example a query can limit the results by service type, availability of "remote public keys" or what fields are included in response.

Returned fields can be any of the following:
* local public key of the service being queried (LKEY)
* signature (signing the whole message) (SIGN)
* current local datetime (TIME)
* local service list (LSVC)
The following are fields in the list of known remote OutNet services:
* remote OutNet service IP (IP)
* remote OutNet service PORT (PORT)
* remote record age - when was this remote OutNet service last seen (AGE)
* remote public key (RKEY)
* remote service list (RSVC)


Where as fields are controlled by including them in the SELECT query parameter, returned records are limited by FILTER parameters:
* local service type/protocol (exact string match)
* IP (range or exact match)
* port (range or exact match)
* age (range)
* remote service type/protocol (exact string match)
* remote service list count (RSVCC) (greater than #)
* remote public key (exact string match)
* remote public key count (RKEYC) (greater than #) One key per service is allowed but OutNet can potentially receive different keys from other OutNet services for a single IP.


Notes
* For numeric operators greater/less/equal allowed operands are an immediate plus one of AGE, RKEYC, IP, PORT, RSVCC
* For operator "string equal" allowed operands are a constant string plust one of RKEY, RSVC, LSVC
* REST call (HTTP GET) Example: SELECT=2036&FILTER=RANGE_GT_500,RANGE_LT_900,RSVC_EQ_HTTP,RKEYC_GT_0
* If any of the received bit fields or query parameters are not defined, they are ignored

* Design OutNet without protocol identifiers to be less detectable and less likely to be blocked.
* Reserve age values over 65500  ex: 0xFFFE = "IO error", 0xFFFD = "duplicate signature", 0xFFFC="coming soon", 0xFFFB="broken signature", 0xFFFA="unresponsive", 0xFFF9="wrong protocol", 0xFFF8="untrusted", 0xFFF0="offline", etc...


Reading further requires understanding of the following concepts:  
https://en.wikipedia.org/wiki/SSH_(Secure_Shell)  
https://en.wikipedia.org/wiki/Elliptic_Curve_Digital_Signature_Algorithm  
https://en.wikipedia.org/wiki/Netsplit  
https://en.wikipedia.org/wiki/Botnet  

## Generating public/private keys
Public and private keys will be generated automatically if you do not have files publicOutNet.key and secretOutNet.key in the OutNet service directory.  If one of the keys is missing, the keys will also be regenerated.  Never share your private key!  Make backup of you private and public keys.  You will NOT be able to recover the private key if it is lost.

## Securing against network splits
Small world network properties can be used to find relatively small semi-isolated connectivity islands that might be trying to split the network and exclude them.  Any fresh client connecting to such an island can become isolated from the rest of the OutNet.  It is important to trust your first connection source.  OutNet services returning HostInfo records without any overlap with existing records can receive a negative rating.  After verification of returned records the raiting should raise above 0.

## Concerns
* OutNet needs to verify HostInfo records received from other OutNet instances by contacting the host in the record.  An OutNet service raiting should be incremented for each verified record and decremented for each failed verification.  Records received from OutNet services with negative rating should not be shared with other OutNet services.
* Age of each record can be taken into account when rating
* Younger records should be validated first if record age is taken into account during rating.
* New records from highly rated services should be verified first.

## Securing agains Botnets
Health and stability of the internet should be every user's goal.  A botnet client can masquerade as a regular OutNet client.  To prevent any botnets from using this service, it is the responsibility of each user to add IP filters as problems come up.  OutNet has built in blacklist support for IPs and keys.

## TODO
* Ipmplement/fix all TODO in source code
* Figure out the best way to run outnet as a system service (OS dependent).  Write wrappers and leave console apps as is?

* How to associate metadata with file hashes in OutNetX?  Metadata can be subject or related hashes.
