{
  steps: {
    checkout:: function(ref) {
      name: 'Check out the repo',
      uses: 'actions/checkout@v3',
      with: {
        ref: ref,
      },
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
          run: 'echo "VERSION=$(cat version.txt)" >> $GITHUB_ENV\n',
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
        {
          name: 'Login to GHCR',
          uses: 'docker/login-action@v2',
          with: {
            registry: 'ghcr.io',
            username: '${{ github.actor }}',
            password: '${{ secrets.GITHUB_TOKEN }}',
          },
        },
        {
          name: 'Build',
          run: 'docker compose -f builder/compose.yml up --exit-code-from unix_${{matrix.arch}} unix_${{matrix.arch}}',
        },
        {
          name: 'Save artifacts',
          uses: 'actions/upload-artifact@v3',
          with: {
            name: 'packages',
            path: './build/release/',
            'if-no-files-found': 'error',
          },
        },
      ],
    },
    build_windows:: function(ref) {
      needs: [],

      'runs-on': 'ubuntu-latest',

      steps: [
        $.steps.checkout(ref),
        {
          name: 'Login to GHCR',
          uses: 'docker/login-action@v2',
          with: {
            registry: 'ghcr.io',
            username: '${{ github.actor }}',
            password: '${{ secrets.GITHUB_TOKEN }}',
          },
        },
        {
          name: 'Build',
          run: 'docker compose -f builder/compose.yml up --exit-code-from windows_win64 windows_win64',
        },
        {
          name: 'Save artifacts',
          uses: 'actions/upload-artifact@v3',
          with: {
            name: 'packages',
            path: './build/release/',
            'if-no-files-found': 'error',
          },
        },
      ],
    },
    build_windows_installer:: function(ref) {
      needs: ['build_windows'],

      'runs-on': 'windows-2019',
      steps: [
        $.steps.checkout(ref),
        {
          name: 'Fetch artifacts',
          uses: 'actions/download-artifact@v3',
          with: {
            name: 'packages',
          },
        },
        {
          name: 'Copy .exe and license to installer dir',
          run: 'copy husarnet-daemon-windows-win64.exe platforms\\windows\\husarnet-daemon.exe\ncopy husarnet-windows-win64.exe platforms\\windows\\husarnet.exe\ncopy LICENSE.txt platforms\\windows\n',
          shell: 'cmd',
        },
        {
          name: 'Building the installer',
          run: '"%programfiles(x86)%\\Inno Setup 6\\iscc.exe" platforms\\windows\\script.iss\n',
          shell: 'cmd',
        },
        {
          name: 'Upload installer',
          uses: 'actions/upload-artifact@v3',
          with: {
            name: 'windows_installer',
            path: 'platforms\\windows\\Output\\husarnet-setup.exe',
          },
        },
      ],
    },


    run_tests:: function(ref) {
      needs+: [],

      'runs-on': 'ubuntu-latest',
      steps: [
        $.steps.checkout(ref),
        {
          name: 'Login to GHCR',
          uses: 'docker/login-action@v2',
          with: {
            registry: 'ghcr.io',
            username: '${{ github.actor }}',
            password: '${{ secrets.GITHUB_TOKEN }}',
          },
        },
        {
          name: 'Run autoformatters',
          run: 'docker compose -f builder/compose.yml up --exit-code-from format format',
        },
        {
          name: 'Run tests',
          run: 'docker compose -f builder/compose.yml up --exit-code-from test test',
        },
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
        'nightly',
      ],

      steps: [
        $.steps.checkout(ref),
        {
          name: 'Fetch artifacts',
          uses: 'actions/download-artifact@v3',
          with: {
            name: 'packages',
          },
        },
        {
          name: 'Fetch Windows installer',
          uses: 'actions/download-artifact@v3',
          with: {
            name: 'windows_installer',
          },
        },
        {
          name: 'Deploy to Husarnet ' + target + ' repository',
          run: './deploy/deploy.sh ' + target,
        },
      ],
    },

    build_docker:: function(suffix, ref) {
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
        {
          name: 'Fetch artifacts',
          uses: 'actions/download-artifact@v3',
          with: {
            name: 'packages',
            path: './build/release/',
          },
        },
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
        {
          name: 'Login to Docker Registry',
          uses: 'docker/login-action@v2',
          with: {
            registry: 'docker.io',
            username: '${{ secrets.HNETUSER_DOCKERHUB_LOGIN }}',
            password: '${{ secrets.HNETUSER_DOCKERHUB_PASSWORD }}',
          },
        },
        {
          name: 'Login to GHCR',
          uses: 'docker/login-action@v2',
          with: {
            registry: 'ghcr.io',
            username: '${{ github.actor }}',
            password: '${{ secrets.GITHUB_TOKEN }}',
          },
        },
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
            push: true,
            tags: 'husarnet/husarnet' + suffix + ':${{matrix.arch_alias}}',
          },
        },
      ],
    },

    release_docker:: function(suffix, ref) {
      needs: [
        'run_tests',
        'build_docker',
      ],

      'runs-on': 'ubuntu-latest',

      steps: [
        $.steps.checkout(ref),
        {
          name: 'Login to Docker Registry',
          uses: 'docker/login-action@v2',
          with: {
            registry: 'docker.io',
            username: '${{ secrets.HNETUSER_DOCKERHUB_LOGIN }}',
            password: '${{ secrets.HNETUSER_DOCKERHUB_PASSWORD }}',
          },
        },
        {
          name: 'create manifest',
          run: |||
            docker manifest create husarnet/husarnet%(suffix):latest \\
            --amend husarnet/husarnet%(suffix):amd64 \\
            --amend husarnet/husarnet%(suffix):arm64 \\
            --amend husarnet/husarnet%(suffix):armhf

            docker manifest create husarnet/husarnet%(suffix):$(cat version.txt) \\
            --amend husarnet/husarnet%(suffix):amd64 \\
            --amend husarnet/husarnet%(suffix):arm64 \\
            --amend husarnet/husarnet%(suffix):armhf
          |||,
        },
        {
          name: 'push manifest',
          run: |||
            docker manifest push husarnet/husarnet%(suffix):latest
            docker manifest push husarnet/husarnet%(suffix):$(cat version.txt)
          |||,
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
        {
          name: 'Fetch linux packages',
          uses: 'actions/download-artifact@v3',
          with: {
            name: 'packages',
          },
        },
        {
          name: 'Fetch windows installer',
          uses: 'actions/download-artifact@v3',
          with: {
            name: 'windows_installer',
          },
        },
        {
          uses: 'marvinpinto/action-automatic-releases@latest',
          with: {
            repo_token: '${{ secrets.GITHUB_TOKEN }}',
            draft: true,
            prerelease: false,
            automatic_release_tag: '${{ github.ref_name }}',
            files: '*.deb\n*.tar\n*.rpm\n*.exe\n',
          },
        },
      ],
    },


  },
}
