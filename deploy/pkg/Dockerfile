FROM archlinux:latest

RUN pacman -Syu --noconfirm

RUN mkdir --mode=0700 /root/.gnupg && chown root:root /root/.gnupg

COPY entrypoint.sh /opt/entrypoint.sh
ENTRYPOINT ["/opt/entrypoint.sh"]
