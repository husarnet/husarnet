![Husarnet logo](docs/logo.svg)

# Husarnet Client

Husarnet is a Peer-to-Peer VPN to connect your laptops, servers and microcontrollers over the Internet with zero configuration.

Key features:

- **Low Latency** - thanks to Peer-to-Peer connection between devices. After establishing a connection, Husarnet infrastructure (Husarnet Base Servers) is used only as a failover proxy if P2P connection is not possible.
- **Zero Configuration** - after installing Husarnet Client, you can add new devices to a group with a single command `husarnet join <YOUR_JOINCODE> mydevname` (on Linux. See [the docs](https://husarnet.com/docs) for other platforms).
- **Low Reconfiguration Time** - in case of a network topology change (eg. transition between two Wi-Fi hotspots), Husarnet needs usually 1 - 3 seconds to reestablish Peer-to-Peer connection.
- **Lightweight** - it works not only on popular OSes (currently only Linux version is released, Windows, MacOS, Android coming soon) but even on ESP32 microcontrollers. That means you can P2P access your things without IoT server at all!
- **Secure & Private** - packets never leave connected devices unencrypted, Perfect Forward Secrecy (PFC) enabled by default.

Husarnet, in it's core, is **one big, automatically routed, IPv6 network**. Running Husarnet daemon creates a virtual network interface (`hnet0`) with an unique Husarnet IPv6 address and associated `fc94::/16` route. If you choose to disable the permission system, any node can reach your node using IPv6 `fc94:...` address, but if you choose to leave it enabled, we've prepared an extensive permissions system for you. You can have multiple virtual islands/groups, your devices can access multiple groups or you can even share access to those groups with other users!

The nodes are identified by their 112-bit **IPv6 addresses, that are based on the public keys of the node**. All connections are also authenticated by the IPv6 address. This property makes it possible to establish connection authenticity without any trusted third party, basing only on the IPv6 address! The connections are also always encrypted.

**Cryptography:** Husarnet uses X25519 from libsodium for key exchange, with ephemeral Curve25519 keys for forward secrecy. The hash of initial public key is validated to match the IPv6 address. The packets are encrypted using libsodium's ChaCha20-Poly1305 secretbox construction with a random 192-bit nonce.

**Runtime safety:** Husarnet is written in C++ using modern memory-safe constructs. Linux version drops all capabilities after initialization. It only retains access to `/etc/hosts` and `/etc/hostname` via a helper process.

If Husarnet instance is not connected to the Husarnet Dashboard, the whitelist (think of it as a crude firewall) and hostname table can only be changed by a local `root` user. All the other **configuration can be changed using the Husarnet Dashboard** after you `join` your device to a group there.

-------------

This is the main development repository for all of the Husarnet Client apps.

For more generic information please have a look at the [Husarnet Docs](https://husarnet.com/docs/manual-general/).

Typical issues preventing Peer-to-Peer connection (thus enjoing a low-latency) and their workarounds are described in the [Troubleshooting Guide](https://husarnet.com/docs/tutorial-troubleshooting).

If you need help and/or have questions regarding Husarnet, feel welcome to post on [Husarnet Community Forums](https://community.husarnet.com). Please, do not use GitHub Issues in this repository for reporting general usage problems and requests. Use the Issues only for technical questions and concerns regarding Husarnet Client building and development.

Following sections are targeted only for developers interested in forking, contributing to Husarnet, or simply wanting to understand the codebase. 

## Code

Husarnet Client is split in two separate binaries - `husarnet`, which is CLI written in Go and `husarnet-daemon`, which is written in C++. Daemon is the entity that plugs given device to Husarnet network; typically running as a service (e.g. via systemd). CLI allows to query, control and manage local daemon as well as manage Husarnet account (i.e. performing activities typically performed via the web GUI).

### How is the repository organized

 - `cli` contains Go code for `husarnet` binary.
 - `core` contains the majority of C++ code for `husarnet-daemon`.
 - `daemon` contains entry points and build definitions for various platforms Husarnet can run on.
 - `deploy` contains files needed for our DEB and RPM repositories
 - `platforms` contains auxiliary files needed for packaging and publishing Husarnet for different platforms.
 - `tests` contains unit (and other) tests (unit tests will run on x86_64 unix platform)
 - `util` contains all the scripts and utilities used for building, testing and deploying. Used by CI and in developers' daily work.

### Building

First run `./util/build-prepare.sh` which will install all required toolchains, etc - tested on Ubuntu 20.04. 

Then, if you want to build:
- CLI only: `./util/build-cli.sh`
- CLI and Daemon, all platforms: `./util/build-all.sh`
- Daemon only, specific platform: `./util/build-cmake.sh <architecture> <platform>`
    - example: `./util/build-cmake.sh amd64 unix`

Created binaries will be available in `build/` folder.

### Running tests
 - `./util/test-prepare.sh` - will install required tools
 - `./util/test-all.sh` - will run *ALL* tests
 - `./util/test-cppcheck.sh` - will run cppcheck
 - `./util/test-unit.sh` - will build and run unit tests. Assumes host machine is x86 and runs some form of Unix

### Creating a pull request

If you wish to submit a PR with a fix or enhancement, feel free to do so. Before submitting make sure that:

```
./util/build-all.sh
./util/test-all.sh
```

are passing without errors. Also make sure to describe your changes and the motivation in the PR description. After you submit the PR, wait for the review and feedback from the maintainers. Good idea is to examine [on-pull-request workflow](https://github.com/husarnet/husarnet/blob/master/.github/workflows/on-pull-request.yml) to see what steps will the CI perform in order to check compatibility of the changes.

## Contributors

This project was possible thanks to:
- [@zielmicha](https://github.com/zielmicha)
- [@m4tx](https://github.com/m4tx)
- [@andrzejwl](https://github.com/andrzejwl)
- [@pidpawel](https://github.com/pidpawel)
- [@konradpr](https://github.com/konradpr)
- [@ympek](https://github.com/ympek)

## License

Husarnet is dual-licensed:
- **GNU Public License** - for derivative projects - eg. exposing a generic VPN functionality.
- **Mozilla Public License** - for projects where you link Husarnet Client code with different project type than mentioned above - eg. using Husarnet Client SDK in ESP32 IoT project.

See LICENSE.txt for details.
