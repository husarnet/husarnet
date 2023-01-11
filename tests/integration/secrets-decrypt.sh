#!/bin/bash
source $(dirname "$0")/../../util/bash-base.sh

echo "Decrypting secrets"

openssl enc -d \
    -aes-256-cbc \
    -pbkdf2 \
    -pass file:${tests_base}/integration/secrets-password.bin \
    -in ${tests_base}/integration/secrets-encrypted.bin \
    -out ${tests_base}/integration/secrets-decrypted.sh
