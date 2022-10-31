{
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

    builder:: function(container) {
      name: 'Builder run ' + container,
      run: 'docker compose -f builder/compose.yml up --exit-code-from ' + container + ' ' + container,
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
        {
          name: 'Read new version',
          run: 'echo "VERSION=$(cat version.txt)" >> $GITHUB_ENV',
        },
        {
          uses: 'stefanzweifel/git-auto-commit-action@v4',
          id: 'autocommit',
          with: {
            commit_message: 'Build version ${{ env.VERSION }}',
          },
        },
      ],
    },

    build_unix:: function(ref) {
      needs: [],

      'runs-on': 'ubuntu-latest',

      strategy: {
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
        $.steps.builder('unix_${{matrix.arch}}'),
        $.steps.push_artifacts('*${{matrix.arch}}*'),
      ],
    },

    build_windows:: function(ref) {
      needs: [],

      'runs-on': 'ubuntu-latest',

      steps: [
        $.steps.checkout(ref),
        $.steps.ghcr_login(),
        $.steps.builder('windows_win64'),
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
        $.steps.builder('format'),
        $.steps.builder('test'),
      ],
    },

    release:: function(target, ref) {
      needs: [
        'run_tests',
        'build_unix',
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
        {
          name: 'Deploy to Husarnet ' + target + ' repository',
          run: './deploy/deploy.sh ' + target,
        },
      ],
    },

    release_github:: function() {
      needs: [
        'run_tests',
        'build_unix',
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

    build_docker:: function(namespace, push, ref) {
      needs: [
        'build_unix',
      ],

      'runs-on': 'ubuntu-latest',

      strategy: {
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
          },
        },
        $.steps.docker_login(),
        $.steps.ghcr_login(),
        {
          name: 'Prepare build',
          run: './platforms/docker/build.sh ${{matrix.arch_alias}}',
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

  },
}
