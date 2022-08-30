# This a development dockerfile and compose setup
# For a user specific one have a look at platforms/unix/docker
FROM ubuntu:20.04

RUN apt-get update && apt-get install -y apt-utils && apt-get install -y --no-install-recommends sudo
RUN echo 'debconf debconf/frontend select Noninteractive' | sudo debconf-set-selections

WORKDIR /husarnet
COPY . .
RUN ./util/prepare-all.sh

CMD ["sleep", "6000"]
