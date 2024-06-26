{
  manifestYamlDoc:: function(v)
    std.manifestYamlDoc(v, indent_array_in_object=true, quote_keys=false),

  steps: {
    checkout:: function(ref) {
      name: 'Check out the repo',
      uses: 'actions/checkout@v4',
      with: {
        ref: ref,
      },
    },

    docker_login:: function() {
      name: 'Login to Docker Registry',
      uses: 'docker/login-action@v3',
      with: {
        registry: 'docker.io',
        username: '${{ secrets.HNETUSER_DOCKERHUB_LOGIN }}',
        password: '${{ secrets.HNETUSER_DOCKERHUB_PASSWORD }}',
      },

    },

    ghcr_login:: function() {
      name: 'Login to GHCR',
      uses: 'docker/login-action@v3',
      with: {
        registry: 'ghcr.io',
        username: '${{ github.actor }}',
        password: '${{ secrets.GITHUB_TOKEN }}',
      },
    },

    push_artifacts:: function(expression, key) {
      name: 'Push artifacts',
      uses: 'actions/upload-artifact@v4',
      with: {
        name: key,
        path: expression,
        'if-no-files-found': 'error',
      },
    },

    pull_artifacts:: function(key_expression) {
      name: 'Pull artifacts',
      uses: 'actions/download-artifact@v4',
      with: {
        pattern: key_expression,
        path: './build/release/',
        'merge-multiple': true,
      },
    },

    builder:: function(command) self.docker('ghcr.io/husarnet/husarnet:builder', command),

    docker:: function(container, command) {
      name: 'Docker run ' + container + ' ' + command,
      run: 'docker run --rm --privileged --tmpfs /var/lib/husarnet:rw,exec --volume $(pwd):/app ' + container + ' ' + command,
    },

    read_version_to_env:: function() {
      name: 'Read new version',
      run: 'echo "VERSION=$(cat version.txt)" >> $GITHUB_ENV',
    },

    secrets_prepare:: function() {
      name: 'Save a password for unlocking a shared secrets repository in a known place',
      run: |||
        chmod 777 tests # So builder can write to it too
        rm -f tests/secrets-decrypted.sh
        echo '${{ secrets.SHARED_SECRETS_PASSWORD }}' > tests/secrets-password.bin
      |||,
    },

    secrets_decrypt:: function()
      $.steps.builder('/app/tests/secrets-tool.sh decrypt'),

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
          uses: 'actions/setup-python@v5',
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
          uses: 'stefanzweifel/git-auto-commit-action@v5',
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
        $.steps.push_artifacts('./build/release/' + '*${{matrix.arch}}*', 'release-linux-${{matrix.arch}}'),
      ],
    },

    build_macos:: function(ref, build_type) {
      needs: [],

      'runs-on': 'ubuntu-latest',

      strategy: {
        'fail-fast': false,
        matrix: {
          arch: [
            'amd64',
            'arm64',
          ],
        },
      },

      steps: [
        $.steps.checkout(ref),
        $.steps.ghcr_login(),
        $.steps.builder('/app/platforms/macos/build.sh ${{matrix.arch}} ' + build_type),
        $.steps.push_artifacts('./build/release/' + '*${{matrix.arch}}*', 'release-macos-${{matrix.arch}}'),
      ],
    },

    build_windows:: function(ref, build_type) {
      needs: [],

      'runs-on': 'ubuntu-latest',

      steps: [
        $.steps.checkout(ref),
        $.steps.ghcr_login(),
        $.steps.builder('/app/platforms/windows/build.sh ' + build_type),
        $.steps.push_artifacts('./build/release/' + '*win64*', 'release-windows-win64'),
      ],
    },

    build_windows_installer:: function(ref) {
      needs: ['build_windows'],

      'runs-on': 'windows-2019',

      steps: [
        $.steps.checkout(ref),
        $.steps.pull_artifacts('release-windows-win64'),
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
        $.steps.push_artifacts('./build/release/' + '*setup*', 'release-windows-setup'),
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
            'ubuntu:24.04',
            'debian:oldstable',
            'debian:stable',
            'debian:testing',
            'fedora:38',
            'fedora:39',
            'fedora:40',
            'fedora:41',
          ],
          test_file: [
            'functional-basic',
            'join-workflow',
            'hooks-basic',
            'hooks-rw',
          ],
          include: [
            { container_name: docker_project + ':amd64', test_platform: 'docker' },
            { container_name: 'ubuntu:18.04', test_platform: 'ubuntu' },
            { container_name: 'ubuntu:20.04', test_platform: 'ubuntu' },
            { container_name: 'ubuntu:22.04', test_platform: 'ubuntu' },
            { container_name: 'ubuntu:24.04', test_platform: 'ubuntu' },
            { container_name: 'debian:oldstable', test_platform: 'debian' },
            { container_name: 'debian:stable', test_platform: 'debian' },
            { container_name: 'debian:testing', test_platform: 'debian' },
            { container_name: 'fedora:38', test_platform: 'fedora' },
            { container_name: 'fedora:39', test_platform: 'fedora' },
            { container_name: 'fedora:40', test_platform: 'fedora' },
            { container_name: 'fedora:41', test_platform: 'fedora' },
          ],
        },
      },

      steps: [
        $.steps.checkout(ref),
        $.steps.pull_artifacts('release-linux-amd64'),
        $.steps.ghcr_login(),
        $.steps.docker_login(),
        $.steps.secrets_prepare(),
        $.steps.secrets_decrypt(),
        $.steps.docker('${{matrix.container_name}}', '/app/tests/integration/runner.sh ${{matrix.test_platform}} ${{matrix.test_file}} ${{matrix.container_name}}'),
      ],
    },

    build_and_test_esp32:: function(ref) {
      needs: [],

      'runs-on': 'esp32',

      steps: [
        $.steps.checkout(ref),
        $.steps.ghcr_login(),
        $.steps.docker_login(),
        $.steps.secrets_prepare(),
        $.steps.secrets_decrypt(),
        $.steps.builder('/app/platforms/esp32/ci.sh'),
        $.steps.push_artifacts('./platforms/esp32/junit.xml', 'esp32-test-results.xml'),
      ],
    },

    release:: function(target, ref) {
      needs: [
        'run_tests',
        'run_integration_tests',
        'build_linux',
        'build_macos',
        'build_windows_installer',
      ],

      'runs-on': [
        'self-hosted',
        'linux',
        'install-' + target,
      ],

      steps: [
        $.steps.checkout(ref),
        $.steps.pull_artifacts('release-*'),
        $.steps.ghcr_login(),
        {
          name: 'Deploy to Husarnet ' + target + ' repository',
          run: '/deploy-all.sh ' + target,
        },
      ],
    },

    release_github:: function() {
      needs: [
        'run_tests',
        'run_integration_tests',
        'build_linux',
        'build_macos',
        'build_windows_installer',
      ],

      'runs-on': 'ubuntu-latest',

      steps: [
        $.steps.pull_artifacts('release-*'),
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
        $.steps.pull_artifacts('release-linux-${{matrix.arch_alias}}'),
        {
          name: 'Set up QEMU',
          uses: 'docker/setup-qemu-action@v3',
        },
        {
          name: 'Set up Docker Buildx',
          uses: 'docker/setup-buildx-action@v3',
          with: {
            version: 'latest',
            'driver-opts': 'image=moby/buildkit:v0.10.5',
          },
        },
        $.steps.docker_login(),
        $.steps.ghcr_login(),
        {
          name: 'Build and push',
          uses: 'docker/build-push-action@v5',
          with: {
            context: '.',
            file: './platforms/docker/Dockerfile',
            platforms: '${{matrix.arch}}',
            'build-args': 'HUSARNET_ARCH=${{matrix.arch_alias}}',
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
          uses: 'mislav/bump-homebrew-formula-action@v3',
          with: {
            'tag-name': '${{ env.VERSION }}',
            'homebrew-tap': 'husarnet/homebrew-tap-nightly',
            'download-url': 'https://nightly.husarnet.com/macos/arm64/husarnet-${{ env.VERSION }}-arm64.tgz',
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
