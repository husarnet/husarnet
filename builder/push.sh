#!/bin/bash
source $(dirname "$0")/../util/bash-base.sh

docker image push ghcr.io/husarnet/husarnet:builder
