#!/bin/bash
source $(dirname "$0")/bash-base.sh

golden_path="$HOME/golden"
golden_tar_path="${golden_path}/tgz"
golden_rpm_path="${golden_path}/rpm"
aptly_config_path="${base_dir}/deploy/aptly.conf"
working_path="/var/www/install"

echo "[==] Adding tar files"
mkdir -p ${golden_tar_path}
for arch in ${unix_archs}; do
  cp release/husarnet-${package_version}-${arch}.tar ${golden_tar_path}
  ln -fs husarnet-${package_version}-${arch}.tar ${golden_tar_path}/husarnet-latest-${arch}.tar
done

echo "[==] Adding rpm files"
mkdir -p ${golden_rpm_path}
for arch in ${unix_archs}; do
  cp release/husarnet-${package_version}-${arch}.rpm ${golden_rpm_path}
  rpmsign --define "_gpg_name contact@husarnet.com" --addsign ${golden_rpm_path}/husarnet-${package_version}-${arch}.rpm
done

echo "[==] Signing rpm repo"
createrepo_c ${golden_rpm_path}/
gpg -u 87D016FBEC48A791AA4AF675197D62F68A4C7BD6 --no-tty --batch --yes --detach-sign --armor ${golden_rpm_path}/repodata/repomd.xml

echo "[==] Adding deb files"
aptly repo add install-nightly release
aptly snapshot create husarnet-${package_version} from repo install-nightly
aptly publish switch -component=husarnet -batch all husarnet-${package_version}

echo "[==] Building new repo"
rm -rf ${working_path}/* || true
mkdir -p ${working_path}/tgz
mkdir -p ${working_path}/yum
mkdir -p ${working_path}/deb

cp -R ${golden_tar_path}/.  ${working_path}/tgz/
cp -R ${golden_rpm_path}/.  ${working_path}/yum/
cp -R $HOME/.aptly/public/. ${working_path}/deb/
cp -R ${base_dir}/deploy/static/. ${working_path}/

echo "[==] Make some extra files for the nightly repository."
sed "s=install.husarnet=nightly.husarnet=" ${working_path}/install.sh > ${working_path}/install-nightly.sh
sed "s=husarnet.com/husarnet.repo=husarnet.com/husarnet-nightly.repo=" -i ${working_path}/install-nightly.sh

sed "s=install.husarnet=nightly.husarnet=" ${working_path}/husarnet.repo > ${working_path}/husarnet-nightly.repo

echo "[==] Setting up permissions..."

chgrp -R www-data ${working_path}

echo "[==] Done, and should work."
