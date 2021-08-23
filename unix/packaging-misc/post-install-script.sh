#!/bin/bash
set -euo pipefail

# Check whether system is *running* systemd
pidof -q systemd || false
if [ ! $? -eq 0 ]; then 
    echo "No systemd running in the system. Will not install start scripts."
    exit 0
fi

systemctl daemon-reload
systemctl enable husarnet

echo '
# husarnet() completion                                     -*- shell-script -*-
_husarnet_completions()
{
    if [ "${#COMP_WORDS[@]}" != "2" ]; then
            if [ "${#COMP_WORDS[@]}" == "3" ]; then
                if [ "${COMP_WORDS[1]}" == "whitelist" ]; then
                    COMPREPLY=($(compgen -W "add rm enable disable ls" "${COMP_WORDS[2]}"))
                fi
                if [ "${COMP_WORDS[1]}" == "host" ]; then
                    COMPREPLY=($(compgen -W "add rm" "${COMP_WORDS[2]}"))
                fi
                if [ "${COMP_WORDS[1]}" == "setup-server" ]; then
                    COMPREPLY=($(compgen -W "<address>"))
                fi
                if [ "${COMP_WORDS[1]}" == "join" ]; then
                    COMPREPLY=($(compgen -W "<join code>"))
                fi
            fi
            if [ "${#COMP_WORDS[@]}" == "4" ]; then
                if [ "${COMP_WORDS[1]}" == "whitelist" ] && [ "${COMP_WORDS[2]}" == "add" ] ; then
                    COMPREPLY=($(compgen -W "<address>"))
                fi
                if [ "${COMP_WORDS[1]}" == "whitelist" ] && [ "${COMP_WORDS[2]}" == "disable" ] ; then
                    COMPREPLY=($(compgen -W "--noinput"))
                fi
                if [ "${COMP_WORDS[1]}" == "whitelist" ] && [ "${COMP_WORDS[2]}" == "rm" ] ; then
                    COMPREPLY=($(compgen -W "<address>"))
                fi
                if [ "${COMP_WORDS[1]}" == "host" ]; then
                    COMPREPLY=($(compgen -W "<hostname>" ))
                    COMPREPLY="${COMPREPLY}"" "$(compgen -W "<address>" )
                fi
                if [ "${COMP_WORDS[1]}" == "join" ]; then
                    COMPREPLY=($(compgen -W "<hostname>" ))
                fi
            fi
        return
    fi
  COMPREPLY=($(compgen -W "status genid websetup whitelist setup-server host version status-json" "${COMP_WORDS[1]}"))
}

complete -F _husarnet_completions husarnet' > /usr/share/bash-completion/completions/husarnet
echo "========================================"
echo "Husarnet installed/upgraded."
echo "Please restart it with:"
echo ""
echo "  systemctl restart husarnet"
echo ""
echo "========================================"
