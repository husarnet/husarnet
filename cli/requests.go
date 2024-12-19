package main

import (
	"errors"
	"fmt"
)

func createError[T any](resp *ApiResponse[T]) error {
	if resp.Type != "success" {
		if len(resp.Errors) > 0 {
			return errors.New(fmt.Sprintf("API request failed. Message: %s", resp.Errors[0]))
		}
		return errors.New(fmt.Sprintf("API request failed."))
	}
	return nil
}

func fetchGroups() (*ApiResponse[Groups], error) {
	resp := callDashboardApi[Groups]("GET", "/web/groups")
	err := createError(&resp)
	if err != nil {
		return nil, err
	}
	return &resp, nil
}

func fetchGroupByUuid(uuid string) (*ApiResponse[GroupDetails], error) {
	resp := callDashboardApi[GroupDetails]("GET", "/web/groups/"+uuid)
	err := createError(&resp)
	if err != nil {
		return nil, err
	}
	return &resp, nil
}

func reqCreateGroup(params GroupCrudInput) (*ApiResponse[Group], error) {
	resp := callDashboardApiWithInput[GroupCrudInput, Group]("POST", "/web/groups", params)
	err := createError(&resp)
	if err != nil {
		return nil, err
	}
	return &resp, nil
}

func reqUpdateGroup(uuid string, params GroupCrudInput) (*ApiResponse[Group], error) {
	resp := callDashboardApiWithInput[GroupCrudInput, Group]("PUT", "/web/groups/"+uuid, params)
	err := createError(&resp)
	if err != nil {
		return nil, err
	}
	return &resp, nil
}

func reqDeleteGroup(uuid string) (*ApiResponse[any], error) {
	resp := callDashboardApi[any]("DELETE", "/web/groups/"+uuid)
	err := createError(&resp)
	if err != nil {
		return nil, err
	}
	return &resp, nil
}

func reqClaim(params ClaimParams) (*ApiResponse[GroupDetails], error) {
	resp := callDashboardApiWithInput[ClaimParams, any]("POST", "/device/manage/claim", params)
	if resp.Type == "success" {
		printSuccess("Claim request was successful")
		if len(resp.Warnings) > 0 {
			for _, warning := range resp.Warnings {
				printWarning(warning)
			}
		}
	} else {
		err := createError(&resp)
		if err != nil {
			return nil, err
		}
	}
	return nil, nil
}

func reqUnclaimSelf() (*ApiResponse[any], error) {
	resp := callDashboardApi[any]("POST", "/device/manage/unclaim")
	if resp.Type == "success" {
		printSuccess("Unclaim request was successful")
		if len(resp.Warnings) > 0 {
			for _, warning := range resp.Warnings {
				printWarning(warning)
			}
		}
	} else {
		err := createError(&resp)
		if err != nil {
			return nil, err
		}
	}
	return nil, nil

}

func reqUpdateDevice(uuid string, params DeviceCrudInput) (*ApiResponse[Device], error) {
	resp := callDashboardApiWithInput[DeviceCrudInput, Device]("PUT", "/web/devices/"+uuid, params)
	err := createError(&resp)
	if err != nil {
		return nil, err
	}
	return &resp, nil
}

func fetchDevices() (*ApiResponse[Devices], error) {
	resp := callDashboardApi[Devices]("GET", "/web/devices")
	err := createError(&resp)
	if err != nil {
		return nil, err
	}
	return &resp, nil
}

func fetchDeviceByUuid(uuid string) (*ApiResponse[Device], error) {
	resp := callDashboardApi[Device]("GET", "/web/devices/"+uuid)
	err := createError(&resp)
	if err != nil {
		return nil, err
	}
	return &resp, nil
}
