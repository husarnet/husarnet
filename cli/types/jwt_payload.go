// Copyright (c) 2024 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
package types

import (
	"time"
)

type JWTPayload struct {
	username string
	exp      time.Time
	origIat  time.Time
}
