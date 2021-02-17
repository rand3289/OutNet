##  OutNet - you are the internet!

As computers get faster the need for centralized services should decrease, yet major corporations push services into clouds to maintain control.  Enslavement by facebook, twitter, instagram, search engines, cloud providers and software stores can be avoided by distributing the equivalent of the server parts of these services among users.  Everyone can run a tiny search engine or a tiny cloud at home.  How will the other users find your services?  Domain names allow others to find your services on the internet.  The problem with using domain namesor host names is that they can be shut down by third parties.  


The alternative is the OutNet.  OutNet is a distributed directory designed to find other conventional or distributed services on the internet.  Services such as web pages, game servers, ftp servers, messengers, forums, video conferences, P2P and distributed services.  Another goals of OutNet is to decentralize the internet making it resistant to control and sensorship.  A small bonus is you no longer have to pay to maintain your domain name.  OutNet provides anonymity.  Instead of a domain or a user name, a public key from a locally generated public/private key pair is used to identify you, your services and services provided by others.  Your real name is no longer needed to provide or consume services on the internet.  Your IP address however will be visible to the world unless used in conjunction with a VPN.


## Proposed service

OutNe is a service that runs on your machine.  It gathers and provides a list of IPv4 addresses, corresponding port numbers and ages of nodes participating in the OutNet.  Age is a number of minutes since the server has last been seen on line.  In addition, OutNet lists other types of services you run such as your web site, P2P or distributed services.  OutNet can be based on existing standards.  One such possibility is a modified service URL format in "Service Location Protocol" (section 4.1 of rfc2608) (https://en.wikipedia.org/wiki/Service_Location_Protocol).  OutNet URLs have to contain routable (public/external) IP addresses instead of host names.  In addition, one does not want to automatically expose all available local network services on the internet.  Example: service:msg:http://8.8.8.8:4321/messenger?pub_key_hash=  

  When OutNet starts, it tries to contact some of the known remote OutNet severs. It collects their information such as public keys and list of services they are advertising.  Remote OutNet servers should have the ability to query your server back over the same connection when contacted and send only a "delta" of the information.  Requests to OutNet should include a range of items to return ex: [0-900] inclusive.  Querying OutNet will return a response described below in pseudocode:

```cpp
struct Response {
    string publicKey;        // PGP/GPG public key - send this first so that remote can close connection if key is black listed
    uint64 dateTime;         // seconds from epoch to this message creation time (UTC/GMT).  Prevents replay attacks with old data.
    vector<string> services; // local services we are trying to advertise
    Statistics counts;       // counts of filtered records, IP:port:age, non-null public keys, local services, remote services etc.
    vector<HostInfo> list;   // list of remote OutNet instances
    string signature;        // PGP/GPG whole message signature - send this last so it can be computed while data is being sent
};

struct HostInfo { // all fields are in the network byte order
    uint32 host;  // IPv4 address
    uint16 port;  // IPv4 port number (1-65535, 0=reserved)
    uint16 age;   // in minutes (up to 45.5 days old) (reserve values over 65,500  ex: 0xFFFE = "IO error", 0xFFFD = "duplicate signature", 0xFFFC="coming soon", 0xFFFB="broken signature", 0xFFFA="unresponsive", 0xFFF9="wrong protocol", 0xFFF8="untrusted", etc...)
    string key;   // remote OutNet service' public key
    vector<string> rservices; // remote services
};
```


* OutNet runs over HTTP to bypass some firewalls and network restrictions.  It can run on different port numbers that can change over time.  Your other services do not have to run over HTTP.
* OutNet response data is binary and is encoded using application octet-stream mime type
* Design OutNet without protocol identifiers (packet/stream format signature) to be less detectable and less likely to be blocked.
* OutNet should have the capability to connect through proxies if it supports exchange of information in addition to queries.
* OutNet should including itself in the service list. This advertises the external/routable/public IP for other services to use.
* OutNet can deny connections to external/routable IPs if frequency of requests coming from them is high.
* There is NO e-mail field shared by the service to prevent network-wide spam. OutNetMsg should be used (see below).
* OutNet can sign the response with a private key and supply a public key for signature verification.


* Mechanisms/protocols required to implement this service are HTTP 1.1, UPnP and digital signatures.
* OutNet is capable of "opening a port in your router" via UPnP in order to be accessible from outside of your network.
* It can "open" additional ports for your distributed services to accept connections.
* OutNet can tell your other local services the "routable/public/external" IP.
* OutNet has a peer list for your other local distributed services.  They can find peers by querying it.
* To support other distributed services OutNet provides a library for signature creation and verification using private and public key. (Your private key does not have to be shared with your other services)


## Querying OutNet service

One way OutNet can be queried is to return a list of discovered OutNet services of a certain type and their EXPECTED public keys.  Here are other possible types of responses (number of records should always be limited by query's range):

* list of local services
* list of [IP:port:age]
* list of local services + list of [IP:port:age]
* list of [IP:port:age + remote public key + list of remote services]  where keys are NOT empty.
* list of [IP:port:age + remote public key + one type remote service]  query by service type.
* list of [IP:port:age + one type remote service]  query by service type.
* list of [IP:port:age]  query by age/age range
* [IP:port:age + list of remote services]  query by public key.


* Returned information is specified by passing a bit field called SELECTION:
    + local public key  (LKEY)
    + current datetime (TIME)
    + local service list (LSVC)
    + local service list limited by service type/protocol (LSVCL)
    + counts of filtered records, IP:port:age, non-null public keys, local services, remote services etc... (COUNTS)

    + IP   (IP)
    + port (PORT)
    + age  (AGE)
    + remote public key (RKEY)
    + remote service list (RSVC)
    + remote service list limited by service type/protocol (RSVCL)

    + signature (sign the whole message) (SIGN)

* Returned records are limited by a parameter called FILTER:
    + range of HostInfo records ex: [0-500] to be able to retrieve n records at a time (RANGE)
    + age range of HostInfo records
    + remote public key NOT empty (use RKEYC - Remote Key Count.  Can there be multiple keys?)
    + remote public key exact string match
    + remote service list NOT empty (use RSVCC - Remote Service Count)
    + remote service type/protocol exact string match

    + local service list NOT empty  (use LSVCC - Local  Service Count)
    + local service type/protocol exact string match


* For operators greater/less/equal allowed operands are RANGE, AGE, RKEYC, IP, PORT, RSVCC, [LSVCC]
* For operator string equal allowed operands are RKEY, RSVC, LSVC
* Examples: RKEYC_GT_0 or LSVCC_LT_10 or RSVC_EQ_HTTP or FILTER=RANGE_GT_500,RANGE_LT_900
* Create actual methods to filter records named RKEYC_GT(uint32 count) etc... switch() on the name of the function as if it was a uint64.

* SELECTION defines response format
* Query parameters define the subset of the returned information
* Requiring SELECTION allows for protocol extension/versioning.
* Handle SELECTION bit field internally as a 64 bit unsigned int.
* If any of the received bit fields or query parameters are not defined, they are ignored


##  Other Base Services

* There is a need for OutNet notification service.  OutNetMsg (OutNetDirect???) receives messages addressed to your public key.  If it is from some one you trust (their public key is on your white list), it tries to open the message using the protocol/mime specified in the message.  OutNetMsg can display the message, offer to open/save the message or file, forward it to one of the services running on your system (for example, by acting as a TCP wrapper) or suggest you install a corresponding protocol handler / service.  For example it might be possible to open a Zoom session this way.
It has to be able to manage other's public keys to be able to put them on your contact list.  It should be possible to create "groups" of public keys. It should be able to share public keys and public key lists with others and notify of updates.  Messages from public keys not on your list will be discarded.  Only direct (non-public) messages will be handled by OutNetMsg.  Public messages or publicly shared files should be handled by other services.  HTTPS can be used to ensure end-to-end encryption (no https proxies).


* While OutNetMsg takes care of direct communication, there is a need for distributed message board service similar to twitter, parler, reddit or newsgroups.  Public messages and files exchanged by this service (OutNetExchange/OutNetShare) are not addressed to an individual but reach users subscribed on a particular subject.  Subject (thread) can have a hierarchical structure for example: local.USA.SanFrancisco.pizza  similar to a usenet group (section 3.2 in rfc977).  software.x86.linux.games.myBlaster can be used to distribute software.  Alternatively a public key (or it's hash) could become a subject allowing subscription to an individual source.  A hybrid model can be implemented where large files are distributed using BitTorrent and small files or file meta data propagate similar to usenet articles.  OutNetExchange can duplicate some usenet features while avoiding these problems:
    + Spam (anyone could post / no signatures / no ratings).
    + Need for a usenet server (not everyone has it).
    + Use of a SINGLE server (all articles have to be in one place).
    + Variable article size limits.
    + NNTP is a text based protocol.


* Another significant service is a public key rating system.  Rating service lays at the center of trust in a distrubuted system.  You should be able to rate your interactions with owners of a public key.  Intention of this service is different than the "Web of trust" (https://en.wikipedia.org/wiki/Web_of_trust).  In OutNet the key comes first and the name is secondary.  The name is not important unless verified through personal communication.  The rating does not state if you know the entity or entity's name in real life.  It rates a specific type of transaction/interaction you had.  For example an instance of a running OutNet service can be rated.  An internet purchase of an item description signed by a key can be rated.  A software/release signed by a key can be rated.  Securyty (virus/trojan free) of the content can be ensured by the rating service.  Software or content releases have to be signed by a private key of the author.  Authors's public keys in turn will be rated by users.  The way you trust in Microsoft, Google or Apple's content distribution system, individual authors have to earn your thrust in their public keys.  Rating should always contain a subject as described in OutNetExchange since an owner of a key can provide multiple services. For example sell items or services and at the same time distribute free software.  His web store should not be rated highly just because he makes great freeware video games.


* OutNetSearch service is used to index information (keys, subjects, content, file hashes) distributed by local services and EXCHANGE it with other search services or local distributed services.

* Authentication service is needed to enable seamless authentication on any conventional (server based) internet resource using your public/private key pair.  Similar to authentication via amazon/google/yahoo/facebook.

* Cryptocurrency system to associate OutNet public keys and wallets?
