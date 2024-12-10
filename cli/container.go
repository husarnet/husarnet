package main

import "time"

type Container struct {
	Devices Devices
	Groups  Groups
}

type Device struct {
	Id          string    `json:"id"`
	Ip          string    `json:"ip"`
	Hostname    string    `json:"hostname"`
	Comment     string    `json:"comment"`
	LastContact time.Time `json:"lastContact"`
	Version     string    `json:"version"`
	UserAgent   string    `json:"userAgent"`
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

type Groups []Group
