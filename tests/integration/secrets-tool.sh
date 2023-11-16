#!/bin/bash
# Copyright (c) 2024 Husarnet sp. z o.o.
# Authors: listed in project_root/README.md
# License: specified in project_root/LICENSE.txt
source $(dirname "$0")/../../util/bash-base.sh

function usage() {
    echo "Usage: secrets-tool.sh [encrypt|decrypt]"
    exit 1
}

if [ ${#} -ne 1 ]; then
    usage
fi

mode=${1}

password_path=${tests_base}/integration/secrets-password.bin
encrypted_path=${tests_base}/integration/secrets-encrypted.bin
decrypted_path=${tests_base}/integration/secrets-decrypted.sh
tmp_decrypted_path=${tests_base}/integration/secrets-decrypted-tmp.sh

openssl_options="-aes-256-cbc -pbkdf2"

function decrypt() {
    destination=$1

    openssl enc -d \
        ${openssl_options} \
        -pass file:${password_path} \
        -in ${encrypted_path} \
        -out ${destination}
}

function encrypt() {
    openssl enc \
        ${openssl_options} \
        -pass file:${password_path} \
        -in ${decrypted_path} \
        -out ${encrypted_path}
}

if [ ${mode} == "decrypt" ]; then
    decrypt ${decrypted_path}
    chmod +x ${decrypted_path}
    exit 0
fi

if [ ${mode} != "encrypt" ]; then
    usage
fi

if [ -f ${decrypted_path} ]; then
    echo "Decrypting secrets to a temporary location"

    function cleanup() {
        rm -f ${tmp_decrypted_path}
    }

    decrypt ${tmp_decrypted_path}

    if diff -q ${tmp_decrypted_path} ${decrypted_path} >/dev/null; then
        echo "Files have not changed, leaving as they were"
        cleanup
        exit 0
    else
        echo "Files differ, will encrypt"
    fi

    cleanup
fi

echo "Encrypting secrets"
encrypt
