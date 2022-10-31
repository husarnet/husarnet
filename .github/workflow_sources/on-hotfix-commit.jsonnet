local common = import 'common.libsonnet';

std.manifestYamlDoc(
  {
    name: 'Release hotfix',
    on: {
      push: {
        branches: ['hotfix'],
      },
    },

    jobs: {
      ref:: 'hotfix',
      base:: {},

      build_unix: common.jobs.build_unix(self.ref) + self.base,
      build_windows: common.jobs.build_windows(self.ref) + self.base,
      build_windows_installer: common.jobs.build_windows_installer(self.ref) + self.base,
      run_tests: common.jobs.run_tests(self.ref) + self.base,
      release_nightly: common.jobs.release('main', self.ref) + self.base,
      build_docker: common.jobs.build_docker('', self.ref) + self.base,
      release_docker: common.jobs.release_docker('', self.ref) + self.base,
      release_github: common.jobs.release_github() + self.base,
    },
  }
)
