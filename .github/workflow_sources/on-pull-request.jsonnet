local common = import 'common.libsonnet';

common.manifestYamlDoc(
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
      docker_project:: 'husarnet/husarnet-nightly',
      build_type:: 'nightly',

      build_linux: common.jobs.build_linux(self.ref, self.build_type) + self.base,
      build_macos: common.jobs.build_macos(self.ref, self.build_type) + self.base,
      build_windows: common.jobs.build_windows(self.ref, self.build_type) + self.base,
      build_windows_installer: common.jobs.build_windows_installer(self.ref) + self.base,
      run_tests: common.jobs.run_tests(self.ref) + self.base,
      run_integration_tests: common.jobs.run_integration_tests(self.ref, self.docker_project) + self.base,
      build_docker: common.jobs.build_docker(self.docker_project, false, self.ref, self.build_type) + self.base,
    },
  }
)
