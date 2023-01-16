apt update

apt install -y --no-install-recommends --no-install-suggests \
    /app/build/release/husarnet-unix-amd64.deb

# Test prerequisites
apt install -y --no-install-recommends --no-install-suggests \
    jq curl iputils-ping openssl
