local common = import 'common.libsonnet';

std.manifestYamlDoc(
  {
    name: 'Pull request workflow',
    on: {
      pull_request: {
        branches: ['master', 'hotfix'],
      },
    },

    jobs: {
      ref:: '${{ github.ref }}',
      base:: {},

      build_unix: common.jobs.build_unix(self.ref) + self.base,
      build_windows: common.jobs.build_windows(self.ref) + self.base,
      build_windows_installer: common.jobs.build_windows_installer(self.ref) + self.base,
      run_tests: common.jobs.run_tests(self.ref) + self.base,
      build_docker: common.jobs.build_docker('-nightly', self.ref) + self.base,
    },
  }
)
