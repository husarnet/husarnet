FROM archlinux:latest
RUN pacman -Syu --noconfirm

ENV GPG_TTY=/dev/console
ENV GPG_AGENT_INFO=/tmp/gpg-agent/socket

COPY entrypoint.sh /opt/entrypoint.sh
ENTRYPOINT ["/opt/entrypoint.sh"]
