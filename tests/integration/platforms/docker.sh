apt update

# Test prerequisites
apt install -y --no-install-recommends --no-install-suggests \
    jq curl iputils-ping openssl
