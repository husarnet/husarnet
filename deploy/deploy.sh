#!/bin/bash
source $(dirname "$0")/../util/bash-base.sh

if [ "$#" -ne 1 ]; then
  echo "Usage: ${0} <deploy-target>"
  exit 1
fi

deploy_target=${1}

case ${deploy_target} in
nightly)
  echo "Deploying to nightly repo"
  ;;

prod)
  echo "Deploying to prod repo"
  ;;

*)
  echo "Unknown destination. Please have in mind that this script MUST be run in a deployment-specific container."
  exit 2
  ;;
esac

pushd ${release_base}

golden_path="$HOME/golden"
golden_tar_path="${golden_path}/tgz"
golden_rpm_path="${golden_path}/rpm"
golden_pkg_path="${golden_path}/pkg"
working_path="/var/www/install"

linux_archs="amd64 i386 arm64 armhf riscv64"
key_id="87D016FBEC48A791AA4AF675197D62F68A4C7BD6"

echo "[==] Updating deployers"
docker pull ghcr.io/husarnet/husarnet:deploy-pkg

echo "[==] Reloading gpg-agent to make it run in a proper session"
gpgconf --kill all
gpgconf --launch gpg-agent

echo "[==] Adding versioned filenames"
for arch in ${linux_archs}; do
  for package_type in tar pkg deb rpm; do
    cp husarnet-linux-${arch}.${package_type} husarnet-${package_version}-${arch}.${package_type}
  done
done

echo "[==] Adding tar files"
mkdir -p ${golden_tar_path}
for arch in ${linux_archs}; do
  cp husarnet-${package_version}-${arch}.tar ${golden_tar_path}
  ln -fs husarnet-${package_version}-${arch}.tar ${golden_tar_path}/husarnet-latest-${arch}.tar
done

echo "[==] Adding pacman files"
mkdir -p ${golden_pkg_path}
for arch in ${linux_archs}; do
  if [[ ${arch} == "armhf" ]]; then
    archlinux_arch_name="armv7h"
  elif [[ ${arch} == "arm64" ]]; then
    archlinux_arch_name="aarch64"
  elif [[ ${arch} == "amd64" ]]; then
    archlinux_arch_name="x86_64"
  else
    archlinux_arch_name=${arch}
  fi

  mkdir -p ${golden_pkg_path}/${archlinux_arch_name}
  cp husarnet-${package_version}-${arch}.pkg ${golden_pkg_path}/${archlinux_arch_name}/husarnet-${package_version}-${arch}.pkg
  docker run --rm \
    --volume ${golden_pkg_path}:/release \
    --volume $(gpgconf --list-dirs agent-extra-socket):/root/.gnupg/S.gpg-agent:z \
    --volume $(gpgconf --list-dirs homedir)/pubring.kbx:/root/.gnupg/pubring.kbx:ro \
    ghcr.io/husarnet/husarnet:deploy-pkg \
    "${key_id}" "${archlinux_arch_name}" "husarnet-${package_version}-${arch}.pkg"
done

echo "[==] Adding rpm files"
mkdir -p ${golden_rpm_path}
for arch in ${linux_archs}; do
  cp husarnet-${package_version}-${arch}.rpm ${golden_rpm_path}
  rpmsign --define "_gpg_name contact@husarnet.com" --addsign ${golden_rpm_path}/husarnet-${package_version}-${arch}.rpm
done

echo "[==] Signing rpm repo"
createrepo_c ${golden_rpm_path}/
gpg -u ${key_id} --no-tty --batch --yes --detach-sign --armor ${golden_rpm_path}/repodata/repomd.xml

echo "[==] Adding deb files"
for arch in ${linux_archs}; do
  aptly repo add install-${deploy_target} husarnet-${package_version}-${arch}.deb
done

aptly snapshot create husarnet-${package_version} from repo install-${deploy_target}
aptly publish switch -component=husarnet -batch all husarnet-${package_version}

echo "[==] Building new repo"
rm -rf ${working_path}/* || true
mkdir -p ${working_path}/tgz
mkdir -p ${working_path}/yum
mkdir -p ${working_path}/rpm
mkdir -p ${working_path}/deb

cp -R ${golden_tar_path}/. ${working_path}/tgz/
cp -R ${golden_pkg_path}/. ${working_path}/pacman/
cp -R ${golden_rpm_path}/. ${working_path}/yum/
cp -R ${golden_rpm_path}/. ${working_path}/rpm/
cp -R $HOME/.aptly/public/. ${working_path}/deb/
cp -R ${base_dir}/deploy/static/. ${working_path}/

if [ "${deploy_target}" == "nightly" ]; then
  echo "[==] Make some extra files for the nightly repository."
  sed "s=install.husarnet=nightly.husarnet=" ${working_path}/install.sh >${working_path}/install-nightly.sh

  sed "s=install.husarnet=nightly.husarnet=" ${working_path}/husarnet_rpm.repo >${working_path}/husarnet-nightly_rpm.repo
  sed "s=install.husarnet=nightly.husarnet=" ${working_path}/husarnet_deb.repo >${working_path}/husarnet-nightly_deb.repo
  sed "s=husarnet.com/husarnet_rpm.repo=husarnet.com/husarnet-nightly_rpm.repo=" -i ${working_path}/install-nightly.sh
  sed "s=husarnet.com/husarnet_deb.repo=husarnet.com/husarnet-nightly_deb.repo=" -i ${working_path}/install-nightly.sh

  # if on nightly, we can also have mac
  echo "[==] Copy MacOS ARM64 binaries"
  tar -zcf husarnet-macos-${package_version}-arm64.tar.gz husarnet-macos-arm64 husarnet-daemon-macos-macos_arm64
  cp husarnet-macos-${package_version}-arm64.tar.gz ${working_path}/husarnet-macos-${package_version}-arm64.tar.gz

  echo "[==] Copy MacOS x86_64 binaries"
  tar -zcf husarnet-macos-${package_version}-amd64.tar.gz husarnet-macos-amd64 husarnet-daemon-macos-macos_amd64
  cp husarnet-macos-${package_version}-amd64.tar.gz ${working_path}/husarnet-macos-${package_version}-amd64.tar.gz
fi

echo "[==] Copy also windows installer exe"

cp husarnet-setup.exe ${working_path}/husarnet-${package_version}-setup.exe
ln -fs ${working_path}/husarnet-${package_version}-setup.exe ${working_path}/husarnet-latest-setup.exe

echo "[==] Done, and should work."

popd
