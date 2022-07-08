// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
package main

import (
	"sort"
	"strings"
)

// returns true if first slice has equal len() and exactly the same elements as the other one
func areSlicesEqual(first []string, second []string) bool {
	count := len(first)
	if count != len(second) {
		return false
	}

	// yes, inefficient implementation, but
	// the func is only used in tests so far - so no worries yet
	// it will be good refactoring excercise - always look on the bright side of life
	firstCopy := make([]string, count)
	secondCopy := make([]string, count)
	copy(firstCopy, first)
	copy(secondCopy, second)
	sort.Strings(firstCopy)
	sort.Strings(secondCopy)

	for i := 0; i < count; i++ {
		if firstCopy[i] != secondCopy[i] {
			return false
		}
	}
	return true
}

// loops over a slice and returns only the elements that contain given string
func filterSlice(fragment string, slice []string) []string {
	filtered := []string{}

	for i := 0; i < len(slice); i++ {
		if strings.Contains(slice[i], fragment) {
			filtered = append(filtered, slice[i])
		}
	}

	return filtered
}
