// Copyright (c) 2022 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
package main

import (
	"fmt"
	"testing"
)

func TestAreSlicesEqual(t *testing.T) {
	var tests = []struct {
		first    []string
		second   []string
		expected bool
	}{
		{[]string{}, []string{}, true},
		{[]string{"a"}, []string{"a"}, true},
		{[]string{"a"}, []string{"b"}, false},
		{[]string{"a", "b", "c"}, []string{"a", "b"}, false},
		{[]string{"a", "b", "c"}, []string{"a", "b", "c"}, true},
		{[]string{"c", "a", "b"}, []string{"a", "b", "c"}, true},
		{[]string{"c", "b", "a"}, []string{"a", "b", "c"}, true},
		{[]string{"c", "b", "a"}, []string{"b", "a", "c"}, true},
	}

	for _, tt := range tests {
		testname := fmt.Sprintf("is %s equal to %s", tt.first, tt.second)
		t.Run(testname, func(t *testing.T) {
			ans := areSlicesEqual(tt.first, tt.second)
			if ans != tt.expected {
				t.Error("Test failed. Expected:", tt.expected, "and got:", ans)
			}
		})
	}
}

func TestFilterSlice(t *testing.T) {
	testAddressSet := []string{
		"fc94:b9e8:a45e:14f8:483d:b0f0:2269:6e1d",
		"fc94:a25d:f834:b020:f941:836b:276e:12eb",
		"fc94:be32:557b:199:9ee4:4e15:3f5:7fac",
		"fc94:a66:d26c:803e:395f:f935:d079:20c7",
		"fc94:5e64:d174:5c74:9b8d:8beb:eae4:9cee",
	}

	var tests = []struct {
		fragment          string
		expectedResultSet []string
	}{
		{"8beb", []string{"fc94:5e64:d174:5c74:9b8d:8beb:eae4:9cee"}},
		{"garbage", []string{}},
		{"fc94", testAddressSet},
		{"f9", []string{"fc94:a66:d26c:803e:395f:f935:d079:20c7", "fc94:a25d:f834:b020:f941:836b:276e:12eb"}},
	}

	for _, tt := range tests {
		testname := fmt.Sprintf("Test substring %s against testAddressSet", tt.fragment)
		t.Run(testname, func(t *testing.T) {
			ans := filterSlice(tt.fragment, testAddressSet)
			if !areSlicesEqual(ans, tt.expectedResultSet) {
				t.Error("Test failed. Expected ", tt.expectedResultSet, ", instead got ", ans)
			}
		})
	}
}

func TestReplaceLastOccurrence(t *testing.T) {
	var tests = []struct {
		testName    string
		needle      string
		replacement string
		haystack    string
		expected    string
	}{
		{"one substring occurrence", "lazy", "active", "The quick brown fox jumps over the lazy dog", "The quick brown fox jumps over the active dog"},
		{"more substring occurrences", "lazy", "brown", "The quick lazy brown fox jumps lazy-ly over the lazy lazy dog", "The quick lazy brown fox jumps lazy-ly over the lazy brown dog"},
		{"no substring occurrence", "fluffy", "active", "The quick brown fox jumps over the lazy dog", "The quick brown fox jumps over the lazy dog"},
		{"whole string is substring", "john", "jack", "john", "jack"},
	}

	for _, tt := range tests {
		t.Run(tt.testName, func(t *testing.T) {
			ans := replaceLastOccurrence(tt.needle, tt.replacement, tt.haystack)
			if ans != tt.expected {
				t.Error("Test failed. Expected:", tt.expected, "and got:", ans)
			}
		})
	}
}
