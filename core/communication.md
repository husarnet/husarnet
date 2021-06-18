# Security considerations

This layer is not trying to encrypt or otherwise protect data it handles.
This is a job of the upper layer. We use signatures only to fight denial-of service attacks (e.g. to prevent someone from registering as us in the base server).

However, we do not try to protect against active man-in-the-middle during his activity - he can anyway DoS us.

# Base server connection
Connect TCP to several base servers. Connection first to establish wins.
This connection is used for most messages to base server.

# Instant reconnect
When our IP address changes, we send it to the base server. Base server pushes this info to all interested parties.

# Message send

If direct connection is available, send it there.
If NAT init is confirmed, send it via UDP to base server.
Else send it via TCP to base server.

Attempt direct connection establishment.

# Direct connection establishment
UDP IP-host pairs retrieved from base server
UDP IP-host pair from link-local workflow

every 25 seconds ping every address pair
first one to respond wins

# Connection workflows
## NAT init workflow
(during init) tcp host->base: get NAT cookie
(during init) tcp base->host: NAT cookie

host->base: NAT init, signed NAT cookie (every 25 seconds)
base->host: NAT init confirm (confirms the UDP works)
host->base: complete 3-way handshake

## Direct IP connection
the NAT init message also contains local IPs

## IPv6 link-local workflow
multicast (unsigned) message I'm here (every 25 seconds)
src: link-local IP, source port
dest: ff02:88bb:31e4:95f7:2b87:6b52:e112:19ac
content: host ID

# Packets
## Host<->host

Half duplex session establishment!

Me HELLO
- my device ID
- his device ID
- my pubkey
- token
- signed by me

His HELLO-REPLY
- my device ID
- his device ID
- his pubkey
- token
- signed by him

Any DATA
- just data, without anything else

## Host<->base

UDP-Host NAT-INIT
- cookie (8 bytes)
- seq (8 bytes)
- signature

UDP-Base NAT-OK

TCP-Base HELLO
- cookie (8 bytes)

TCP-Host REQUEST-INFO
- device ID

TCP-Base DEVICE-ADDRESSES (sent for REQUEST-INFO or when config changes)
- device ID
- addresses

TCP/UDP-Host SEND-DATA
- dest device ID (14 bytes)
- data

TCP/UDP-Host RECV-DATA
- source device ID (14 bytes)
- data

TCP-Host INFO (every 25 seconds)
- list of internal IP addresses and ports

TCP-Base PONG
