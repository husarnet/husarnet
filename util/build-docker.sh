#!/bin/bash
source $(dirname "$0")/bash-base.sh

push=0
if [ "$#" -eq 1 ]; then
    if [ $1 == "push" ]; then
        echo "=== WILL PUSH RESULTS TO PUBLIC REPO ==="
        push=1
        sleep 5
    else
        echo "Usage: $0 [push]"
        exit 1
    fi
fi

docker_misc="${base_dir}/unix/docker-misc"

pushd "${release_base}"

cp ${docker_misc}/husarnet-docker.sh ${release_base}/husarnet-docker

builder_name="husarnet-builder"

if [ $(docker buildx ls | grep ${builder_name} | wc -l) -eq 0 ]; then
    docker buildx create --name ${builder_name}
fi

function convert_arch {
    case $1 in
        i386)
            echo "linux/386"
            ;;
        armhf)
            echo "linux/arm/v7"
            ;;
        arm64)
            echo "linux/arm64/v8"
            ;;
        *)
            echo "linux/${arch}"
            ;;
    esac
}

# Various platforms are limited in various ways when it comes to Docker support, so we are limiting ourselves to these
docker_archs="amd64 armhf arm64"

for arch in ${docker_archs}; do
    # verbose flag: --progress=plain
    docker buildx build \
        --builder ${builder_name} \
        --file "${docker_misc}/Dockerfile" \
        --build-arg build_arch=${arch} \
        --platform $(convert_arch ${arch}) \
        --tag docker.io/husarnet/husarnet:$(if [ $push -eq 1 ]; then echo "${arch}-"; fi)latest \
        $(if [ $arch == "amd64" ] && [ $push -ne 1 ]; then echo "--load"; fi) \
        $(if [ $push -eq 1 ]; then echo "--push"; fi) \
        .
done

if [ $push -eq 1 ]; then
    images=$(
        for arch in ${docker_archs}; do
            echo "husarnet/husarnet:${arch}-latest"
        done
    )

    docker manifest rm husarnet/husarnet:latest || true
    docker manifest create husarnet/husarnet:latest ${images}

    docker manifest annotate husarnet/husarnet:latest "husarnet/husarnet:amd64-latest" --arch "amd64"
    docker manifest annotate husarnet/husarnet:latest "husarnet/husarnet:armhf-latest" --arch "arm" --variant "v7"
    docker manifest annotate husarnet/husarnet:latest "husarnet/husarnet:arm64-latest" --arch "arm64" --variant "v8"

    docker manifest push husarnet/husarnet:latest
fi

popd
