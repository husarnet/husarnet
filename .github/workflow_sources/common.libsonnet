{
  manifestYamlDoc:: function(v)
    std.manifestYamlDoc(v, indent_array_in_object=true, quote_keys=false),

  steps: {
    checkout:: function(ref) {
      name: 'Check out the repo',
      uses: 'actions/checkout@v3',
      with: {
        ref: ref,
      },
    },

    docker_login:: function() {
      name: 'Login to Docker Registry',
      uses: 'docker/login-action@v2',
      with: {
        registry: 'docker.io',
        username: '${{ secrets.HNETUSER_DOCKERHUB_LOGIN }}',
        password: '${{ secrets.HNETUSER_DOCKERHUB_PASSWORD }}',
      },

    },

    ghcr_login:: function() {
      name: 'Login to GHCR',
      uses: 'docker/login-action@v2',
      with: {
        registry: 'ghcr.io',
        username: '${{ github.actor }}',
        password: '${{ secrets.GITHUB_TOKEN }}',
      },
    },

    push_artifacts:: function(expression) {
      name: 'Push artifacts',
      uses: 'actions/upload-artifact@v3',
      with: {
        name: 'packages',
        path: './build/release/' + expression,
        'if-no-files-found': 'error',
      },
    },

    pull_artifacts:: function() {
      name: 'Pull artifacts',
      uses: 'actions/download-artifact@v3',
      with: {
        name: 'packages',
        path: './build/release/',
      },
    },

    builder:: function(command) self.docker('ghcr.io/husarnet/husarnet:builder', command),

    docker:: function(container, command) {
      name: 'Docker run ' + container + ' ' + command,
      run: 'docker run --rm --privileged --volume $(pwd):/app ' + container + ' ' + command,
    },

    build_macos_daemon:: function(build_type, arch) {
      name: 'Build daemon natively on MacOS',
      run: './daemon/build.sh macos macos_' + arch + ' ' + build_type,
    },

    build_macos_cli:: function(arch) {
      name: 'Build CLI natively on MacOS',
      run: './cli/build.sh macos ' + arch,
    },

    read_version_to_env:: function() {
      name: 'Read new version',
      run: 'echo "VERSION=$(cat version.txt)" >> $GITHUB_ENV',
    },

  },

  jobs: {
    bump_version:: function(ref) {
      needs: [],

      'runs-on': 'ubuntu-latest',

      outputs: {
        commit_ref: '${{ steps.autocommit.outputs.commit_hash }}',
      },

      steps: [
        $.steps.checkout(ref),
        {
          uses: 'actions/setup-python@v4',
          with: {
            'python-version': '3.x',
          },
        },
        {
          name: 'Bump version',
          run: 'python ./util/version-bump.py',
        },
        $.steps.read_version_to_env(),
        {
          uses: 'stefanzweifel/git-auto-commit-action@v4',
          id: 'autocommit',
          with: {
            commit_message: 'Build version ${{ env.VERSION }}',
          },
        },
      ],
    },

    build_builder:: function(ref) {
      needs: [],

      'runs-on': 'ubuntu-latest',

      steps: [
        $.steps.checkout(ref) + {
          with+: {
            'fetch-depth': 22,  // This is a semi-random number. We don't want to fetch the whole repository, we only want "a couple of them"
          },
        },
        $.steps.ghcr_login(),
        {
          name: 'If there were any changes to builder - build it and push to ghcr',
          run: |||
            if git diff --name-only ${{ github.event.before }} ${{ github.event.after }} | grep -e "^builder/"; then
              ./builder/build.sh && ./builder/push.sh
            else
              echo "No changes to the builder found - refusing to rebuild"
            fi
          |||,
        },
      ],
    },

    build_deployers:: function(ref) {
      needs: [],

      'runs-on': 'ubuntu-latest',

      steps: [
        $.steps.checkout(ref) + {
          with+: {
            'fetch-depth': 22,  // This is a semi-random number. We don't want to fetch the whole repository, we only want "a couple of them"
          },
        },
        $.steps.ghcr_login(),
        {
          name: 'If there were any changes to deployers - build them and push to ghcr',
          run: |||
            if git diff --name-only ${{ github.event.before }} ${{ github.event.after }} | grep -e "^deploy/"; then
              ./deploy/update-deployers.sh push
            else
              echo "No changes to the deployer found - refusing to rebuild"
            fi
          |||,
        },
      ],
    },

    build_linux:: function(ref, build_type) {
      needs: [],

      'runs-on': 'ubuntu-latest',

      strategy: {
        'fail-fast': false,
        matrix: {
          arch: [
            'amd64',
            'i386',
            'arm64',
            'armhf',
            'riscv64',
          ],
        },
      },

      steps: [
        $.steps.checkout(ref),
        $.steps.ghcr_login(),
        $.steps.builder('/app/platforms/linux/build.sh ${{matrix.arch}} ' + build_type),
        $.steps.push_artifacts('*${{matrix.arch}}*'),
      ],
    },

    build_macos_arm64_natively:: function(ref, build_type) {
      needs: [],

      'runs-on': [
        'self-hosted',
        'macOS',
      ],

      steps: [
        $.steps.checkout(ref),
        $.steps.build_macos_daemon(build_type, 'arm64'),
        $.steps.build_macos_cli('arm64'),
        $.steps.push_artifacts('*macos*'),
      ],
    },

    build_macos_amd64_natively:: function(ref, build_type) {
      needs: [],

      'runs-on': [
        'macos-latest',
      ],

      steps: [
        $.steps.checkout(ref),
        {
          name: 'Install coreutils, as our scripts depend on them and zig + ninja for building',
          run: 'brew install coreutils zig ninja',
        },
        $.steps.build_macos_daemon(build_type, 'amd64'),
        $.steps.build_macos_cli('amd64'),
        $.steps.push_artifacts('*macos*'),
      ],
    },

    build_windows:: function(ref, build_type) {
      needs: [],

      'runs-on': 'ubuntu-latest',

      steps: [
        $.steps.checkout(ref),
        $.steps.ghcr_login(),
        $.steps.builder('/app/platforms/windows/build.sh ' + build_type),
        $.steps.push_artifacts('*win64*'),
      ],
    },

    build_windows_installer:: function(ref) {
      needs: ['build_windows'],

      'runs-on': 'windows-2019',

      steps: [
        $.steps.checkout(ref),
        $.steps.pull_artifacts(),
        {
          name: 'Copy .exe and license to installer dir',
          shell: 'cmd',
          run: |||
            copy build\release\husarnet-daemon-windows-win64.exe platforms\windows\husarnet-daemon.exe
            copy build\release\husarnet-windows-win64.exe platforms\windows\husarnet.exe
            copy LICENSE.txt platforms\windows
          |||,
        },
        {
          name: 'Building the installer',
          shell: 'cmd',
          run: |||
            "%programfiles(x86)%\Inno Setup 6\iscc.exe" platforms\windows\script.iss
            copy platforms\windows\Output\husarnet-setup.exe build\release\husarnet-setup.exe
          |||,
        },
        $.steps.push_artifacts('*setup*'),
      ],
    },

    run_tests:: function(ref) {
      needs: [],

      'runs-on': 'ubuntu-latest',

      steps: [
        $.steps.checkout(ref),
        $.steps.ghcr_login(),
        $.steps.builder('/app/daemon/format.sh'),
        $.steps.builder('/app/tests/test.sh'),
      ],
    },

    run_integration_tests:: function(ref, docker_project) {
      needs: [
        'build_linux',
        'build_docker',
      ],

      'runs-on': 'ubuntu-latest',

      strategy: {
        'fail-fast': false,
        matrix: {
          container_name: [
            docker_project + ':amd64',
            'ubuntu:18.04',
            'ubuntu:20.04',
            'ubuntu:22.04',
            'debian:oldstable',
            'debian:stable',
            'debian:testing',
            'fedora:37',
            'fedora:38',
          ],
          test_file: [
            'functional-basic',
            'join-workflow',
          ],
          include: [
            { container_name: docker_project + ':amd64', test_platform: 'docker' },
            { container_name: 'ubuntu:18.04', test_platform: 'ubuntu' },
            { container_name: 'ubuntu:20.04', test_platform: 'ubuntu' },
            { container_name: 'ubuntu:22.04', test_platform: 'ubuntu' },
            { container_name: 'debian:oldstable', test_platform: 'debian' },
            { container_name: 'debian:stable', test_platform: 'debian' },
            { container_name: 'debian:testing', test_platform: 'debian' },
            { container_name: 'fedora:37', test_platform: 'fedora' },
            { container_name: 'fedora:38', test_platform: 'fedora' },
          ],
        },
      },

      steps: [
        $.steps.checkout(ref),
        $.steps.pull_artifacts(),
        $.steps.ghcr_login(),
        $.steps.docker_login(),
        {
          name: 'Save a password for unlocking a shared secrets repository in a known place',
          run: "echo '${{ secrets.SHARED_SECRETS_PASSWORD }}' > tests/integration/secrets-password.bin",
        },
        $.steps.builder('/app/tests/integration/secrets-tool.sh decrypt'),
        $.steps.docker('${{matrix.container_name}}', '/app/tests/integration/runner.sh ${{matrix.test_platform}} ${{matrix.test_file}}'),
      ],
    },

    release:: function(target, ref) {
      needs: [
        'run_tests',
        'run_integration_tests',
        'build_linux',
        'build_macos_amd64_natively',
        'build_macos_arm64_natively',
        'build_windows_installer',
      ],

      'runs-on': [
        'self-hosted',
        'linux',
        target,
      ],

      steps: [
        $.steps.checkout(ref),
        $.steps.pull_artifacts(),
        $.steps.ghcr_login(),
        {
          name: 'Deploy to Husarnet ' + target + ' repository',
          run: './deploy/deploy.sh ' + target,
        },
      ],
    },

    release_github:: function() {
      needs: [
        'run_tests',
        'run_integration_tests',
        'build_linux',
        'build_macos_amd64_natively',
        'build_macos_arm64_natively',
        'build_windows_installer',
      ],

      'runs-on': 'ubuntu-latest',

      steps: [
        $.steps.pull_artifacts(),
        {
          uses: 'marvinpinto/action-automatic-releases@latest',
          with: {
            repo_token: '${{ secrets.GITHUB_TOKEN }}',
            draft: true,
            prerelease: false,
            automatic_release_tag: '${{ github.ref_name }}',
            files: |||
              ./build/release/*.deb
              ./build/release/*.tar
              ./build/release/*.rpm
              ./build/release/*setup.exe
            |||,
          },
        },
      ],
    },

    build_docker:: function(namespace, push, ref, build_type) {
      needs: [
        'build_linux',
      ],

      'runs-on': 'ubuntu-latest',

      strategy: {
        'fail-fast': false,
        matrix: {
          include: [
            {
              arch: 'linux/amd64',
              arch_alias: 'amd64',
            },
            {
              arch: 'linux/arm64/v8',
              arch_alias: 'arm64',
            },
            {
              arch: 'linux/arm/v7',
              arch_alias: 'armhf',
            },
          ],
        },
      },

      steps: [
        $.steps.checkout(ref),
        $.steps.pull_artifacts(),
        {
          name: 'Set up QEMU',
          uses: 'docker/setup-qemu-action@v2',
        },
        {
          name: 'Set up Docker Buildx',
          uses: 'docker/setup-buildx-action@v2',
          with: {
            version: 'latest',
            'driver-opts': 'image=moby/buildkit:v0.10.5',
          },
        },
        $.steps.docker_login(),
        $.steps.ghcr_login(),
        {
          name: 'Prepare build',
          run: './platforms/docker/build.sh ${{matrix.arch_alias}} ' + build_type,
        },
        {
          name: 'Build and push',
          uses: 'docker/build-push-action@v3',
          with: {
            context: '.',
            file: './platforms/docker/Dockerfile',
            platforms: '${{matrix.arch}}',
            push: push,
            tags: namespace + ':${{matrix.arch_alias}}',
          },
        },
      ],
    },

    release_docker:: function(namespace, ref) {
      needs: [
        'run_tests',
        'run_integration_tests',
        'build_docker',
      ],

      'runs-on': 'ubuntu-latest',

      steps: [
        $.steps.checkout(ref),
        $.steps.docker_login(),
        {
          name: 'create manifest',
          run: |||
            docker manifest create %(namespace)s:latest \
            --amend %(namespace)s:amd64 \
            --amend %(namespace)s:arm64 \
            --amend %(namespace)s:armhf

            docker manifest create %(namespace)s:$(cat version.txt) \
            --amend %(namespace)s:amd64 \
            --amend %(namespace)s:arm64 \
            --amend %(namespace)s:armhf
          ||| % {
            namespace: namespace,
          },
        },
        {
          name: 'push manifest',
          run: |||
            docker manifest push %(namespace)s:latest
            docker manifest push %(namespace)s:$(cat version.txt)
          ||| % {
            namespace: namespace,
          },
        },
      ],
    },

    update_in_homebrew_tap:: function(ref) {
      needs: [
        'release_nightly',
      ],

      'runs-on': 'ubuntu-latest',

      steps: [
        $.steps.checkout(ref),
        $.steps.read_version_to_env(),
        {
          uses: 'mislav/bump-homebrew-formula-action@v2.2',
          with: {
            'tag-name': '${{ env.VERSION }}',
            'homebrew-tap': 'husarnet/homebrew-tap-nightly',
            'download-url': 'https://nightly.husarnet.com/husarnet-macos-${{ env.VERSION }}-arm64.tar.gz',
            'commit-message': |||
              Husarnet for MacOS version {{version}}

              Automatic commit by https://github.com/mislav/bump-homebrew-formula-action
            |||,
          },
          env: {
            COMMITTER_TOKEN: '${{ secrets.HOMEBREW_TAP_COMMITER_TOKEN }}',
          },
        },
      ],

    },

  },
}
