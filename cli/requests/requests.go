// Copyright (c) 2024 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt
package requests

import (
	"errors"
	"fmt"
	"github.com/husarnet/husarnet/cli/v2/constants"

	"github.com/husarnet/husarnet/cli/v2/types"
)

func FetchUserInfo() (*types.ApiResponse[types.UserResponse], error) {
	resp := callDashboardApi[types.UserResponse]("GET", "/web/user")
	return respOrError(&resp)
}

func FetchGroups() (*types.ApiResponse[types.Groups], error) {
	resp := callDashboardApi[types.Groups]("GET", "/web/groups")
	return respOrError(&resp)
}

func FetchGroupByUuid(uuid string) (*types.ApiResponse[types.GroupDetails], error) {
	resp := callDashboardApi[types.GroupDetails]("GET", "/web/groups/"+uuid)
	return respOrError(&resp)
}

func CreateGroup(params types.GroupCrudInput) (*types.ApiResponse[types.Group], error) {
	resp := callDashboardApiWithInput[types.GroupCrudInput, types.Group]("POST", "/web/groups", params)
	return respOrError(&resp)
}

func UpdateGroup(uuid string, params types.GroupCrudInput) (*types.ApiResponse[types.Group], error) {
	resp := callDashboardApiWithInput[types.GroupCrudInput, types.Group]("PUT", "/web/groups/"+uuid, params)
	return respOrError(&resp)
}

func DeleteGroup(uuid string) (*types.ApiResponse[any], error) {
	resp := callDashboardApi[any]("DELETE", "/web/groups/"+uuid)
	return respOrError(&resp)
}

func Claim(params types.ClaimParams) (*types.ApiResponse[any], error) {
	resp := callDashboardApiWithInput[types.ClaimParams, any]("POST", "/device/manage/claim", params)
	return respOrError(&resp)
}

func UnclaimSelf() (*types.ApiResponse[any], error) {
	resp := callDashboardApi[any]("POST", "/device/manage/unclaim")
	return respOrError(&resp)
}

func UpdateDevice(uuid string, params types.DeviceCrudInput) (*types.ApiResponse[types.Device], error) {
	resp := callDashboardApiWithInput[types.DeviceCrudInput, types.Device]("PUT", "/web/devices/"+uuid, params)
	return respOrError(&resp)
}

func UnclaimDevice(uuid string) (*types.ApiResponse[any], error) {
	resp := callDashboardApi[any]("POST", "/web/devices/unclaim/"+uuid)
	return respOrError(&resp)
}

func FetchDevices() (*types.ApiResponse[types.Devices], error) {
	resp := callDashboardApi[types.Devices]("GET", "/web/devices")
	return respOrError(&resp)
}

func FetchDeviceByUuid(uuid string) (*types.ApiResponse[types.Device], error) {
	resp := callDashboardApi[types.Device]("GET", "/web/devices/"+uuid)
	return respOrError(&resp)
}

func AttachDetach(params types.AttachDetachInput, op constants.DeviceOp) (*types.ApiResponse[any], error) {
	var endpoint string
	if op == constants.OpAttach {
		endpoint = "/web/groups/attach-device"
	} else if op == constants.OpDetach {
		endpoint = "/web/groups/detach-device"
	} else {
		return nil, errors.New("invalid operation")
	}
	resp := callDashboardApiWithInput[types.AttachDetachInput, any]("POST", endpoint, params)
	return respOrError(&resp)
}

func RotateClaimToken() (*types.ApiResponse[types.UserResponse], error) {
	resp := callDashboardApi[types.UserResponse]("POST", "/web/settings/rotate-claim-token")
	return respOrError(&resp)
}

func createError[T any](resp *types.ApiResponse[T]) error {
	if resp.Type != "success" {
		if len(resp.Errors) > 0 {
			return errors.New(fmt.Sprintf("API request failed. Message: %s", resp.Errors[0]))
		}
		return errors.New(fmt.Sprintf("API request failed."))
	}
	return nil
}

func respOrError[T any](resp *types.ApiResponse[T]) (*types.ApiResponse[T], error) {
	err := createError[T](resp)
	if err != nil {
		return nil, err
	}
	return resp, nil
}
