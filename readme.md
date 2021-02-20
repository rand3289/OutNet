##  OutNet - you are the internet!

As computers get faster the need for centralized services should decrease, yet major corporations push services into clouds to maintain control.  Enslavement by facebook, twitter, instagram, search engines, cloud providers and software stores can be avoided by distributing the equivalent of the server parts of these services among users.  Everyone can run a tiny search engine or a tiny cloud at home.  How will the other users find your services?  Domain names allow others to find your services on the internet.  The problem with using domain namesor host names is that they can be shut down by third parties.  Most users on the internet do not have a registered domain name.


The alternative is the OutNet.  OutNet is an open source distributed directory protocol designed to find conventional or distributed services on the internet.  Services such as web pages, game servers, ftp servers, messengers, forums, video conferences, P2P and distributed services.  Another goals of OutNet is to decentralize the internet making it resistant to control and sensorship.  A small bonus is you no longer have to pay to maintain your domain name.  OutNet provides anonymity.  Instead of a domain or a user name, a public key from a locally generated public/private key pair is used to identify you, your services and services provided by others.  Your real name is no longer needed to provide or consume services on the internet.  Your IP address however will be visible to the world unless used in conjunction with a VPN.


## Proposed service

OutNet is implemented by a service with the same name that runs on your machine.  It gathers and provides a list of IPv4 addresses, corresponding port numbers and ages of nodes participating in the OutNet.  In addition, OutNet lists the types of remote services and local services you run such as your web sites, game servers and P2P services.

When OutNet starts, it tries to contact some of the known remote OutNet severs. It collects their information such as public keys and list of services they advertise.  Local services can query OutNet to find a list of peers.  Querying OutNet will return a response described below in pseudocode:

```cpp
struct Response {
    string publicKey;         // RSA public key - send this first so that remote can close connection if key is black listed
    uint64 dateTime;          // seconds from epoch to this message creation time (UTC/GMT).  Prevents replay attacks with old data.
    vector<string> services;  // local services we are trying to advertise
    Statistics counts;        // counts of total IP:port:age records, non-null public keys, local services, remote services etc.
    vector<HostInfo> list;    // list of remote OutNet instances
    string signature;         // RSA whole message digital signature - send this last so it can be computed while data is being sent
};

struct HostInfo {             // all fields are in the network byte order
    uint32 host;              // IPv4 address
    uint16 port;              // IPv4 port number (1-65535, 0=reserved)
    uint16 age;               // in minutes (up to 45.5 days old) since the server has been seen on line
    string key;               // remote OutNet service' public key
    vector<string> rservices; // remote services
};
```

Since one does not want to expose ALL available local services on the internet, OutNet does not discover local services.  Local services can register with OutNet or be added via configuration files.  Service descriptions have to contain routable (public/external) IP addresses instead of host names.  If OutNet determines your service is behind a NAT router and IP is a non-routable IP, it will replace your non-routable IP with it's own external routable IP when listing your service.  In addition, OutNet will open a port in the router via UPnP protocol that will allow your service to accept connections.


OutNet can be based on existing standards.  For example, list of services can be based on modified service description format of DNS-SD (section 4.1.2 of rfc6763), SLP (section 4.1 of rfc2608), Bonjour, SSDP or MDNS.  Service description has to contain at least the following fields: type of service (ex: printer), actual protocol (ex:ipp), IP, port number, user defined name/description/path/attribute.  

Other possible fields: version, internet protocol (tcp or udp), piority[0-9] or type of data (service/device/block/stream/file/messages/image/audio/video/conference/game/ xml/json/yaml/soap/rpc/wsdl/feed(structured data)/list/key-value/url/ binary/text/unicode/ascii/base64/uuencoded/ printer/speaker/display/ blockchain/cryptocurrency/geolocation/weather/virtualreality/time).  

Maximum field lengths are TBD.  DNS-SD limits service names to 15 characters.  Proposed lengths: priority(char[1]), service class(char[16]), protocol(char[16]), ipproto(char[3]), IP(char[15]), port(char[5]), description or path(char[32]).  Maximum service description length in the range of 96 - 128 bytes.

Since no host names are used, service description encoding can be limited to printable ASCII characters.  

Examples: "printer:lpr:8.8.8.8:54321:2nd floor printer"  
Same device, different protocol:  "printer:ipp:8.8.8.8:54321:2nd floor printer"  
Key-value pairs are described in rfc6763 section 6.  


* Mechanisms/protocols required to implement OutNet are HTTP 1.1, UPnP and digital signatures (RSA or BSD signify/minisign).
* OutNet runs as a REST service over HTTP to bypass some firewalls and network restrictions.  It can run on different port numbers that can change over time.  Your other services do not have to run over HTTP or TCP.
* OutNet can sign responses with a private key and supply a public key for signature verification.
* OutNet can deny connections to external/routable IPs if frequency of requests coming from them is high.
* Any service can register itself with OutNet by signing it's URL with the same private key OutNet is using and sending the signed URL to the local instance of OutNet.
* Local services can register with OutNet using multicast at which point they receive OutNet's unicast address for querying.
* OutNet has a peer list for your other local services.  They can find their peers by querying a local OutNet instance.
* OutNet should include itself in the service list.  This advertises the external/routable/public IP for other services to use.
* OutNet is capable of "opening a port in your router" via UPnP in order to be accessible from outside of your network.
* It can "open" additional ports for your distributed services to accept connections.
* To support other distributed services OutNet provides a library for signature creation uning private key and verification using public key.  Your private key does not have to be shared with your other services.


## OutNet service query parameters

OutNet can be queried to return local info and/or a filtered list of discovered remote OutNet services.  For example a query can limit the results by service type, availability of "remote public keys" or what fields are included in response.  Requests to OutNet can include a range of items to return ex: [0-900] inclusive.  Records in the response are always ordered by age.

Returned fields are specified by passing a "bit field" called SELECT represented by a number:
* local public key  (LKEY=1)
* current datetime (TIME=2)
* local service list (LSVC=4)
* counts of filtered records, IP:port:age, non-null public keys, local services, remote services etc... (COUNTS=8)
* IP   (IP=16)
* port (PORT=32)
* age  (AGE=64)
* remote public key (RKEY=128)
* remote service list (RSVC=256)
* signature (sign the whole message) (SIGN=512)
* remote service list filtered by service type/protocol (RSVCL=1024)
* local service list filtered by service type/protocol (LSVCL=2048)


Where as fields are controlled by SELECT parameter, returned records are limited by FILTER parameter:
* local service type/protocol exact string match
* range of HostInfo records ex: [0-500] to be able to retrieve n records at a time (RANGE)
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


## Implementation hints

* SELECT allows for protocol extension/versioning.
* Internally handle SELECT bit field as a 64 bit unsigned int.
* If any of the received bit fields or query parameters are not defined, they are ignored
* host:port pair forms an ID for any record
* Create actual methods to filter records named RKEYC_GT(uint32 count) etc... switch() on the name of the function as if it was a uint64

* OutNet response data is binary to save bandwidth and is encoded using application octet-stream mime type
* Design OutNet without protocol identifiers (packet/stream format signature) to be less detectable and less likely to be blocked.
* Add the ability to query your server back over the same connection when contacted and send only a "delta" of the information.
* Reserve age values over 65500  ex: 0xFFFE = "IO error", 0xFFFD = "duplicate signature", 0xFFFC="coming soon", 0xFFFB="broken signature", 0xFFFA="unresponsive", 0xFFF9="wrong protocol", 0xFFF8="untrusted", 0xFFF0="offline", etc...


### Security against network splits

* Small world network properties can be used to find relatively small semi-isolated connectivity islands that might be trying to split the network and eliminate them.  This might require user interaction.
* OutNet needs to verify HostInfo records received from other OutNet instances by contacting the host in the record.  An OutNet service raiting should be incremented for each verified record and decremented for each failed verification.  Records received from OutNet services with negative rating should not be shared with other OutNet services.
* Age of each record can be taken into account when rating ???
* Younger records should be validated first if record age is taken into account during rating.
* New records from highly rated services should be verified first.
* OutNet services returning HostInfo records without any overlap with existing records should receive negative rating (- # of records???) After the verification the raiting will raise above 0.


##  Future Base Services

* There is a need for OutNet notification service.  One of the reasons is there is NO e-mail field shared by the OutNet service to prevent network-wide spam.  OutNetMsg (OutNetDirect???) receives messages addressed to your public key.  If it is from some one you trust (their public key is on your white list), it tries to open the message using the protocol/mime specified in the message.  OutNetMsg can display the message, offer to open/save the message or file, forward it to one of the services running on your system (for example, by acting as a TCP wrapper) or suggest you install a corresponding protocol handler / service.  For example it might be possible to open a Zoom session this way.
It has to be able to manage other's public keys to be able to put them on your contact list.  It should be possible to create "groups" of public keys. It should be able to share public keys and public key lists with others and notify of updates.  Messages from public keys not on your list will be discarded.  Only direct (non-public) messages will be handled by OutNetMsg.  Public messages or publicly shared files should be handled by other services.  HTTPS can be used to ensure end-to-end encryption (no https proxies).


* While OutNetMsg takes care of direct communication, there is a need for distributed message board service similar to twitter, parler, reddit or newsgroups.  Public messages and files exchanged by this service (OutNetExchange/OutNetX/OutNetShare) are not addressed to an individual but reach users subscribed on a particular subject.  Subject (thread) can have a hierarchical structure for example: local.USA.SanFrancisco.pizza  similar to a usenet group (section 3.2 in rfc977).  software.x86.linux.games.myBlaster can be used to distribute software.  Alternatively a public key (or it's hash) could become a subject allowing subscription to an individual source.  A hybrid model can be implemented where large files are distributed using BitTorrent and small files or file meta data propagate similar to usenet articles.  OutNetExchange can duplicate some usenet features while avoiding these problems:
    + Spam (anyone could post / no signatures / no ratings).
    + Need for a usenet server (not everyone has it).
    + Use of a SINGLE server (all articles have to be in one place).
    + Variable article size limits.
    + NNTP is a text based protocol.


* Another significant service is a public key rating system.  OutNetRate rating service lays at the center of trust in a distrubuted system.  You should be able to rate your interactions with owners of a public key.  Intention of this service is different than the "Web of trust" (https://en.wikipedia.org/wiki/Web_of_trust).  In OutNet the key comes first and the name is secondary.  The name is not important unless verified through personal communication.  The rating does not state if you know the entity or entity's name in real life.  It rates a specific type of transaction/interaction you had.  For example an instance of a running OutNet service can be rated.  An internet purchase of an item description signed by a key can be rated.  A software/release signed by a key can be rated.  Securyty (virus/trojan free) of the content can be ensured by the rating service.  Software or content releases have to be signed by a private key of the author.  Authors's public keys in turn will be rated by users.  The way you trust in Microsoft, Google or Apple's content distribution system, individual authors have to earn your thrust in their public keys.  Rating should always contain a subject as described in OutNetExchange since an owner of a key can provide multiple services.  For example sell items or services and at the same time distribute free software.  His web store should not be rated highly just because he makes great freeware video games.


* OutNetSearch service is used to index information (keys, subjects, content, file hashes) distributed by local services and EXCHANGE it with other search services or local distributed services.

* Authentication service is needed to enable seamless authentication on any conventional (server based) internet resource using your public/private key pair.  Similar to authentication via amazon/google/yahoo/facebook.

* Cryptocurrency system to associate OutNet public keys and wallets.
