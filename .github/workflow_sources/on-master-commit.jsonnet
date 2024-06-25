local common = import 'common.libsonnet';

common.manifestYamlDoc(
  {
    name: 'Release nightly',
    on: {
      push: {
        branches: ['master'],
      },
      workflow_dispatch: {},  // Allow starting the workflow manually
    },

    jobs: {
      ref:: '${{ needs.bump_version.outputs.commit_ref }}',
      base:: {
        needs+: ['bump_version', 'build_builder'],
      },
      docker_project:: 'husarnet/husarnet-nightly',

      bump_version: common.jobs.bump_version('master'),
      build_builder: common.jobs.build_builder(self.ref) + {
        needs+: ['bump_version'],
      },
      build_type:: 'nightly',

      build_linux: common.jobs.build_linux(self.ref, self.build_type) + self.base,
      build_macos: common.jobs.build_macos(self.ref, self.build_type) + self.base,
      build_windows: common.jobs.build_windows(self.ref, self.build_type) + self.base,
      build_windows_installer: common.jobs.build_windows_installer(self.ref) + self.base,
      run_tests: common.jobs.run_tests(self.ref) + self.base,
      run_integration_tests: common.jobs.run_integration_tests(self.ref, self.docker_project) + self.base,
      build_and_test_esp32: common.jobs.build_and_test_esp32(self.ref) + self.base,

      release_nightly: common.jobs.release('nightly', self.ref) + self.base,
      build_docker: common.jobs.build_docker(self.docker_project, true, self.ref, self.build_type) + self.base,
      release_docker_nightly: common.jobs.release_docker(self.docker_project, self.ref) + self.base,
      update_in_homebrew_tap: common.jobs.update_in_homebrew_tap(self.ref) + self.base,
    },
  }
)
