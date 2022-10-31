local common = import 'common.libsonnet';

std.manifestYamlDoc(
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
        needs+: ['bump_version'],
      },
      docker_project:: 'husarnet/husarnet-nightly',

      bump_version: common.jobs.bump_version('master'),
      build_unix: common.jobs.build_unix(self.ref) + self.base,
      build_windows: common.jobs.build_windows(self.ref) + self.base,
      build_windows_installer: common.jobs.build_windows_installer(self.ref) + self.base,
      run_tests: common.jobs.run_tests(self.ref) + self.base,
      release: common.jobs.release('nightly', self.ref) + self.base,
      build_docker: common.jobs.build_docker(self.docker_project, true, self.ref) + self.base,
      release_docker: common.jobs.release_docker(self.docker_project, self.ref) + self.base,
    },
  }
)
