package utils

import (
	"fmt"
	"github.com/husarnet/husarnet/cli/v2/output"
	"github.com/husarnet/husarnet/cli/v2/types"
	"net/netip"
	"os"
	"regexp"
	"strings"
)

// Replace only last occurrence of given substring in a string. Return new string.
// If substring is not found, original string (haystack) is returned.
func ReplaceLastOccurrence(search string, replacement string, subject string) string {
	lastIdx := strings.LastIndex(subject, search)
	if lastIdx != -1 {
		newStr := subject[:lastIdx] + replacement + subject[lastIdx+len(search):]
		return newStr
	}
	return subject
}

func GetOwnHostname() string {
	hostname, err := os.Hostname()
	if err != nil {
		output.PrintWarning("was unable to determine the hostname of the machine")
		return "unnamed-device"
	}
	return hostname
}

var uuidv4Regex = regexp.MustCompile("^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$")

func LooksLikeUuidv4(maybeUuid string) bool {
	return uuidv4Regex.MatchString(maybeUuid)
}

func LooksLikeIpv6(maybeAddr string) bool {
	parsed, err := netip.ParseAddr(maybeAddr)
	return err == nil && parsed.Is6()
}

func FindGroupUuidByName(needle string, haystack types.Groups) (string, error) {
	for _, group := range haystack {
		if group.Name == needle {
			return group.Id, nil
		}
	}
	return "", fmt.Errorf("Couldn't find a group with name '%s'. Check the spelling.", needle)
}

func FindDeviceUuidByIp(needle string, haystack types.Devices) (string, error) {
	for _, dev := range haystack {
		if dev.Ip == needle {
			return dev.Id, nil
		}
	}
	return "", fmt.Errorf("Couldn't find device with ip '%s'. Are you sure IP is correct?", needle)
}

func FindDeviceUuidByHostname(hostname string, haystack types.Devices) (string, error) {
	// note this returns the first found match, and does not detect if there is ambiguity (TODO)
	for _, dev := range haystack {
		if dev.Hostname == hostname {
			return dev.Id, nil
		}
		for _, alias := range dev.Aliases {
			if alias == hostname {
				return dev.Id, nil
			}
		}
	}
	return "", fmt.Errorf("Couldn't find device named '%s'", hostname)
}

func EnvVarName(name string) string {
	return "HUSARNET_" + strings.ToUpper(name)
}
