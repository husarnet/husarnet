![Husarnet logo](docs/logo.svg)

# Husarnet Client

Husarnet is a Peer-to-Peer VPN to connect your laptops, servers and microcontrollers over the Internet with zero configuration.

Key features:

- **Low Latency** - thanks to Peer-to-Peer connection between devices. After establishing a connection, Husarnet Infrastructure (Husarnet Base Servers) is used only as a failover proxy if P2P connection is not possible.
- **Zero Configuration** - after installing Husarnet Client, you can add new devices to your network with a single command `husarnet join <YOUR_JOINCODE> mydevname` (on Linux. See docs for other platforms).
- **Low Reconfiguration Time** - in case of a network topology change (eg. transition between two Wi-Fi hotspots), Husarnet needs usually 1 - 3 seconds to reastablish a new Peer-to-Peer connection.
- **Lightweight** - it works not only on popular OSes (currently only Linux version is released, Windows, MacOS, Android coming soon) but even on ESP32 microcontrollers. That means you can P2P access your things without IoT server at all!
- **Secure & Private** - packets never leave connected devices unencrypted, Perfect Forward Secrecy (PFC) enabled by default.

Husarnet, in it's core, is **one big, automatically routed, IPv6 network**. Running Husarnet daemon creates a virtual network interface (`hnet0`) with an unique Husarnet IPv6 address and associated `fc94::/16` route. If you choose to disable the permission system, any node can reach your node using IPv6 `fc94:...` address, but if you choose to leave it enabled, we've prepared an extensive permissions system for you. You can have multiple virtual islands/networks, your devices can access multiple networks or you can even share access to those networks with other users!

The nodes are identified by their 112-bit **IPv6 addresses, that are based on the public keys of the node**. All connections are also authenticated by the IPv6 address. This property makes it possible to establish connection authenticity without any trusted third party, basing only on the IPv6 address! The connections are also always encrypted.

**Cryptography:** Husarnet uses X25519 from libsodium for key exchange, with ephemeral Curve25519 keys for forward secrecy. The hash of initial public key is validated to match the IPv6 address. The packets are encrypted using libsodium's ChaCha20-Poly1305 secretbox construction with a random 192-bit nonce.

**Runtime safety:** Husarnet is written in C++ using modern memory-safe constructs. Linux version drops all capabilities after initialization. It only retains access to `/etc/hosts` and `/etc/hostname` via a helper process.

If Husarnet instance is not connected to the Husarnet Dashboard, the whitelist (think of it as a crude firewall) and hostname table can only be changed by a local `root` user. All the other **configuration can be changed using the Husarnet Dashboard** after you `join` your device to a network there.

-------------

This is the main development repository for all of the Husarnet Client apps.

For more generic information please have a look at the [Husarnet Docs](https://husarnet.com/docs/manual-general/).

Typical issues preventing Peer-to-Peer connection (thus enjoing a low-latency) and their workarounds are described in the [Troubleshooting Guide](https://husarnet.com/docs/tutorial-troubleshooting).

## Repository organization

 - `util` directory - all the scripts and utilities used for building and testing. CI config should be referencing those in order to make local testing easier
 - `unix` directory - main dir for unix platform code
 - `tests` directory - unit (and other) tests (unit tests will run on x86_64 unix platform)
 - `deploy` directory - various files needed for deployment - like the static files in our repositories

 ## Building and publishing
 - `./util/build-prepare.sh` - will install all required toolchains, etc - tested on Ubuntu 20.10
 - If you want to bump the current version (i.e. as a preparation for release) - `./util/version-bump.py` (keep in mind that all merges/commits to the default branch will to that automatically)
 - If you want to build:
    - all platforms: `./util/build-all.sh`
    - specific platform:
        - platforms using cmake: `./util/build-cmake.sh <architecture> <platform>`
 - CI will be doing these steps automatically when ran on default branch:
    - bump and commit the new version "number"
    - build for all platforms and architectures (when available)
    - test all platforms and architectures (when available)
    - publish to the internal repositories
    - wait for manual confirmation to publish in the public repo
 - CI will be doing these steps then ran on any other branch:
    - build for all platforms and architectures (when available)
    - test all platforms and architectures (when available)

## Running tests
 - `./util/test-prepare.sh` - will install required tools
 - `./util/test-all.sh` - will run *ALL* tests
 - `./util/test-cppcheck.sh` - will run cppcheck
 - `./util/test-unit.sh` - will build and run unit tests. Assumes host machine is x86 and runs some form of Unix
 
## CI extra notes
 - `./util/prepare-all.sh` - this will prepare both build and test environments

## Contributors

This project was possible thanks to:
- @zielmicha
- @m4tx
- @andrzejwl
- @pidpawel
- @konradpr
- @ympek

## License

Husarnet is dual-licensed:
- **GNU Public License** - for derivative projects - eg. exposing a generic VPN functionality.
- **Mozilla Public License** - for projects where you link Husarnet Client code with different project type than mentioned above - eg. using Husarnet Client SDK in ESP32 IoT project.

See LICENSE.txt for details.
