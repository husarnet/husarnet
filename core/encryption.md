We use a simple protocol based on libsodium key exchange and cryptobox.

The cryptographic protocol currently doesn't protect against replays - this isn't actually a real problem - TCP does that. We will probably add the replay protection in future as an additonal layer of security.

The protocol provides forward secrecy by using ephermeral Curve25519 keys.
