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
working_path="/var/www/install"

unix_archs="amd64 i386 arm64 armhf riscv64"

echo "[==] Adding versioned filenames"
for arch in ${unix_archs}; do
  for package_type in tar deb rpm; do
    cp husarnet-unix-${arch}.${package_type} husarnet-${package_version}-${arch}.${package_type}
  done
done

echo "[==] Adding tar files"
mkdir -p ${golden_tar_path}
for arch in ${unix_archs}; do
  cp husarnet-${package_version}-${arch}.tar ${golden_tar_path}
  ln -fs husarnet-${package_version}-${arch}.tar ${golden_tar_path}/husarnet-latest-${arch}.tar
done

echo "[==] Adding rpm files"
mkdir -p ${golden_rpm_path}
for arch in ${unix_archs}; do
  cp husarnet-${package_version}-${arch}.rpm ${golden_rpm_path}
  rpmsign --define "_gpg_name contact@husarnet.com" --addsign ${golden_rpm_path}/husarnet-${package_version}-${arch}.rpm
done

echo "[==] Signing rpm repo"
createrepo_c ${golden_rpm_path}/
gpg -u 87D016FBEC48A791AA4AF675197D62F68A4C7BD6 --no-tty --batch --yes --detach-sign --armor ${golden_rpm_path}/repodata/repomd.xml

echo "[==] Adding deb files"
for arch in ${unix_archs}; do
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

cp -R ${golden_tar_path}/.  ${working_path}/tgz/
cp -R ${golden_rpm_path}/.  ${working_path}/yum/
cp -R ${golden_rpm_path}/.  ${working_path}/rpm/
cp -R $HOME/.aptly/public/. ${working_path}/deb/
cp -R ${base_dir}/deploy/static/. ${working_path}/


if [ "${deploy_target}" == "nightly" ]; then
  echo "[==] Make some extra files for the nightly repository."
  sed "s=install.husarnet=nightly.husarnet=" ${working_path}/install.sh > ${working_path}/install-nightly.sh

  sed "s=install.husarnet=nightly.husarnet=" ${working_path}/husarnet_rpm.repo > ${working_path}/husarnet-nightly_rpm.repo
  sed "s=install.husarnet=nightly.husarnet=" ${working_path}/husarnet_deb.repo > ${working_path}/husarnet-nightly_deb.repo
  sed "s=husarnet.com/husarnet_rpm.repo=husarnet.com/husarnet-nightly_rpm.repo=" -i ${working_path}/install-nightly.sh
  sed "s=husarnet.com/husarnet_deb.repo=husarnet.com/husarnet-nightly_deb.repo=" -i ${working_path}/install-nightly.sh
fi

echo "[==] Copy also windows installer exe"

cp husarnet-setup.exe ${working_path}/husarnet2alpha-unstable-${package_version}-setup.exe
ln -fs ${working_path}/husarnet2alpha-unstable-${package_version}-setup.exe ${working_path}/husarnet2alpha-unstable-latest-setup.exe

echo "[==] Done, and should work."

popd
