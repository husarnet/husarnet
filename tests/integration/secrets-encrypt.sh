#!/bin/bash
source $(dirname "$0")/../../util/bash-base.sh

if [ -f ${tests_base}/integration/secrets-decrypted.sh ]; then
    echo "Decrypting secrets to a temporary location"

    function cleanup() {
        rm -f ${tests_base}/integration/secrets-decrypted-tmp.sh
    }

    openssl enc -d \
        -aes-256-cbc \
        -pbkdf2 \
        -pass file:${tests_base}/integration/secrets-password.bin \
        -in ${tests_base}/integration/secrets-encrypted.bin \
        -out ${tests_base}/integration/secrets-decrypted-tmp.sh

    if diff -q ${tests_base}/integration/secrets-decrypted-tmp.sh ${tests_base}/integration/secrets-decrypted.sh > /dev/null; then
        echo "Files have not changed, leaving as they were"
        cleanup
        exit 0
    fi

    cleanup
fi

echo "Encrypting secrets"

openssl enc \
    -aes-256-cbc \
    -pbkdf2 \
    -pass file:${tests_base}/integration/secrets-password.bin \
    -in ${tests_base}/integration/secrets-decrypted.sh \
    -out ${tests_base}/integration/secrets-encrypted.bin
