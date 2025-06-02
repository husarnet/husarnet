// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
package constants

import (
	"runtime"
)

const CliVersion string = "2.0.314"

const UserAgent string = "Husarnet-CLI" + "," + runtime.GOOS + "," + runtime.GOARCH + "," + CliVersion
