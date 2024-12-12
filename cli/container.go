package main

import "time"

type Container struct {
	Devices Devices
	Groups  Groups
}

type Device struct {
	Id          string    `json:"id"`
	Ip          string    `json:"ip"`
	Emoji       string    `json:"emoji"`
	Hostname    string    `json:"hostname"`
	UserAgent   string    `json:"userAgent"`
	Version     string    `json:"version"`
	Comment     string    `json:"comment"`
	Aliases     []string  `json:"aliases"`
	LastContact time.Time `json:"lastContact"`
}

type Devices []Device

type UserSettings struct {
	ClaimToken string `json:"claimToken"`
}

type UserResponse struct {
	Settings UserSettings `json:"settings"`
}

type Group struct {
	Id      string `json:"id"`
	Emoji   string `json:"emoji"`
	Name    string `json:"name"`
	Comment string `json:"comment"`
}

type JoinCode struct {
	Token string `json:"token"`
}

type GroupDetails struct {
	Group             Group    `json:"group"`
	AttachableDevices []Device `json:"attachableDevices"`
	JoinCode          JoinCode `json:"joinCode"`
}

type GroupCrudInput struct {
	Name    string `json:"name"`
	Emoji   string `json:"emoji"`
	Comment string `json:"comment"`
}

type DeviceCrudInput struct {
	Emoji    string   `json:"emoji"`
	Hostname string   `json:"hostname"`
	Comment  string   `json:"comment"`
	Aliases  []string `json:"aliases"`
}

type Groups []Group
