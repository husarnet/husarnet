// Copyright (c) 2025 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
package requests

import (
	"errors"
	"fmt"

	"github.com/husarnet/husarnet/cli/v2/constants"

	"github.com/husarnet/husarnet/cli/v2/types"
)

var apiPrefix = "/v3"

func FetchUserInfo() (*types.ApiResponse[types.UserResponse], error) {
	resp := callDashboardApi[types.UserResponse]("GET", apiPrefix+"/web/user")
	return respOrError(&resp)
}

func FetchGroups() (*types.ApiResponse[types.Groups], error) {
	resp := callDashboardApi[types.Groups]("GET", apiPrefix+"/web/groups")
	return respOrError(&resp)
}

func FetchGroupByUuid(uuid string) (*types.ApiResponse[types.GroupDetails], error) {
	resp := callDashboardApi[types.GroupDetails]("GET", apiPrefix+"/web/groups/"+uuid)
	return respOrError(&resp)
}

func CreateGroup(params types.GroupCrudInput) (*types.ApiResponse[types.Group], error) {
	resp := callDashboardApiWithInput[types.GroupCrudInput, types.Group]("POST", apiPrefix+"/web/groups", params)
	return respOrError(&resp)
}

func UpdateGroup(uuid string, params types.GroupCrudInput) (*types.ApiResponse[types.Group], error) {
	resp := callDashboardApiWithInput[types.GroupCrudInput, types.Group]("PUT", apiPrefix+"/web/groups/"+uuid, params)
	return respOrError(&resp)
}

func DeleteGroup(uuid string) (*types.ApiResponse[any], error) {
	resp := callDashboardApi[any]("DELETE", apiPrefix+"/web/groups/"+uuid)
	return respOrError(&resp)
}

func Claim(params types.ClaimParams) (*types.ApiResponse[any], error) {
	resp := callDashboardApiWithInput[types.ClaimParams, any]("POST", apiPrefix+"/device/manage/claim", params)
	return respOrError(&resp)
}

func UnclaimSelf() (*types.ApiResponse[any], error) {
	resp := callDashboardApi[any]("POST", apiPrefix+"/device/manage/unclaim")
	return respOrError(&resp)
}

func UpdateDevice(uuid string, params types.DeviceCrudInput) (*types.ApiResponse[types.Device], error) {
	resp := callDashboardApiWithInput[types.DeviceCrudInput, types.Device]("PUT", apiPrefix+"/web/devices/"+uuid, params)
	return respOrError(&resp)
}

func UnclaimDevice(uuid string) (*types.ApiResponse[any], error) {
	resp := callDashboardApi[any]("POST", apiPrefix+"/web/devices/unclaim/"+uuid)
	return respOrError(&resp)
}

func FetchDevices() (*types.ApiResponse[types.Devices], error) {
	resp := callDashboardApi[types.Devices]("GET", apiPrefix+"/web/devices")
	return respOrError(&resp)
}

func FetchDeviceByUuid(uuid string) (*types.ApiResponse[types.Device], error) {
	resp := callDashboardApi[types.Device]("GET", apiPrefix+"/web/devices/"+uuid)
	return respOrError(&resp)
}

func AttachDetach(params types.AttachDetachInput, op constants.DeviceOp) (*types.ApiResponse[any], error) {
	endpoint := apiPrefix
	switch op {
	case constants.OpAttach:
		endpoint += "/web/groups/attach-device"
	case constants.OpDetach:
		endpoint += "/web/groups/detach-device"
	default:
		return nil, errors.New("invalid operation")
	}
	resp := callDashboardApiWithInput[types.AttachDetachInput, any]("POST", endpoint, params)
	return respOrError(&resp)
}

func RotateClaimToken() (*types.ApiResponse[types.UserResponse], error) {
	resp := callDashboardApi[types.UserResponse]("POST", apiPrefix+"/web/settings/rotate-claim-token")
	return respOrError(&resp)
}

func createError[T any](resp *types.ApiResponse[T]) error {
	if resp.Type != "success" {
		if len(resp.Errors) > 0 {
			return fmt.Errorf("API request failed. Message: %s", resp.Errors[0])
		}
		return fmt.Errorf("API request failed (no error message provided)")
	}
	return nil
}

func respOrError[T any](resp *types.ApiResponse[T]) (*types.ApiResponse[T], error) {
	err := createError(resp)
	if err != nil {
		return nil, err
	}
	return resp, nil
}
