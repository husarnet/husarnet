yum install -y \
    /app/build/release/husarnet-unix-amd64.rpm

# Test prerequisites
yum install -y \
    jq curl iputils openssl
