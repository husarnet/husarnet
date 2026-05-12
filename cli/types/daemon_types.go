package types

import "net/netip"

type DaemonResponse[ResultType any] struct {
	Status string
	Error  string
	Result ResultType
}

func (response *DaemonResponse[ResultType]) IsOk() bool {
	return response.Status == "success" || response.Status == "ok"
}

type BaseConnectionStatus struct {
	Address netip.Addr
	Port    int
	Type    string // TODO long-term - make it enum maybe
}

type PeerStatus struct {
	HusarnetAddress  netip.Addr     `json:"husarnet_address"`
	LinkLocalAddress netip.AddrPort `json:"link_local_address"`

	IsActive         bool `json:"is_active"`
	IsReestablishing bool `json:"is_reestablishing"`
	IsSecure         bool `json:"is_secure"`
	IsTunelled       bool `json:"is_tunelled"`

	// TODO reenable this after latency support is readded
	// LatencyMs int `json:"latency_ms"` // TODO long-term figure out how to convert it to time.Duration

	SourceAddresses   []netip.AddrPort `json:"source_addresses"`
	TargetAddresses   []netip.AddrPort `json:"target_addresses"`
	UsedTargetAddress netip.AddrPort   `json:"used_target_address"`
}

type DaemonLiveData struct {
	Health              DaemonHealth         `json:"health"`
	BaseConnection      BaseConnectionStatus `json:"base_connection"`
	DashboardConnection bool                 `json:"dashboard_connection"`
	LocalIP             netip.Addr           `json:"local_ip"`
	Peers               []DaemonPeerInfo     `json:"peers"`
}

type ClaimInfo struct {
	Owner    string   `json:"owner"`
	Hostname string   `json:"hostname"`
	Aliases  []string `json:"aliases"`
}

type DashboardPeerInfo struct {
	Address  netip.Addr `json:"address"`
	Hostname string     `json:"hostname"`
	Aliases  []string   `json:"aliases"`
}

type DaemonPeerInfo struct {
	Address          netip.Addr `json:"address"`
	IsActive         bool       `json:"is_active"`
	IsReestablishing bool       `json:"is_reestablishing"`
	IsTunelled       bool       `json:"is_tunelled"`
	IsSecure         bool       `json:"is_secure"`
}

type DeviceFeatures struct {
	AccountAdmin bool `json:"AccountAdmin"`
	SyncHostname bool `json:"SyncHostname"`
}

type DashboardConfig struct {
	IsClaimed bool `json:"is_claimed"`

	ClaimInfo ClaimInfo           `json:"claim_info"`
	Features  DeviceFeatures      `json:"features"`
	Peers     []DashboardPeerInfo `json:"peers"`
}

type LicenseData struct {
	BaseServers []netip.Addr `json:"base_server_addresses"`
	ApiServers  []netip.Addr `json:"api_servers"`
	EbServers   []netip.Addr `json:"eb_servers"`
}

type UserConfig struct {
	Whitelist []netip.Addr `json:"whitelist"`
}

type EnvConfig struct {
	InstanceFqdn  string `json:"instance_fqdn"`
	LogVerbosity  string `json:"log_verbosity"`
	HooksEnabled  bool   `json:"hooks_enabled"`
	DaemonApiHost string `json:"daemon_api_host"`
	DaemonApiPort int    `json:"daemon_api_port"`
}

type DaemonConfig struct {
	Dashboard DashboardConfig `json:"dashboard"`
	Env       EnvConfig       `json:"env"`
	License   LicenseData     `json:"license"`
	User      UserConfig      `json:"user_config"`
}

type DaemonHealth struct {
	Summary bool `json:"summary"`
}

type DaemonStatus struct {
	LiveData  DaemonLiveData `json:"live"`
	Config    DaemonConfig   `json:"config"`
	Version   string         `json:"version"`
	UserAgent string         `json:"user_agent"`

	// HooksEnabled  bool   `json:"hooks_enabled"`
	// LocalHostname string `json:"local_hostname"`

	// IsJoined         bool            `json:"is_joined"`
	// IsReady          bool            `json:"is_ready"`
	// IsReadyToJoin    bool            `json:"is_ready_to_join"`
	// ConnectionStatus map[string]bool `json:"connection_status"`

	// UserSettings map[string]string     `json:"user_settings"` // TODO long-term - think about a better structure (more importantly enums) to hold this if needed
	// HostTable    map[string]netip.Addr `json:"host_table"`
	// Peers        []PeerStatus
}
