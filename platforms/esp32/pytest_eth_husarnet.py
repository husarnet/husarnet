from dataclasses import dataclass
from typing import Callable, Dict, Any
import pytest
from pytest import Config
from pytest_embedded_idf.dut import IdfDut
from pytest_helpers import *

WEBSETUP_IP = "fc94:b01d:1803:8dd8:b293:5c7d:7639:932a"
INTERNET_PING_IP = "1.1.1.1"
LAN_PING_IP = get_lan_ip()
HUSARNET_PING_IP = get_husarnet_ip()

DUT_HUSARNET_HOSTNAME = "esp32-integration-test"

@pytest.mark.esp32
@pytest.mark.lan8720
@pytest.mark.ethernet_router
def test_lan_performance(
  dut: IdfDut,
  log_performance: Callable[[str, object], None]
)-> None:  
  # Ping
  launch_test_ping(dut, log_performance, "internet_ping_64B", INTERNET_PING_IP, data_size=64)
  launch_test_ping(dut, log_performance, "internet_ping_1200B", INTERNET_PING_IP, data_size=1200)
  launch_test_ping(dut, log_performance, "lan_ping_64B", LAN_PING_IP, data_size=64)
  
  # Iperf
  res = launch_test_iperf(dut, log_performance, "lan_iperf", LAN_PING_IP)
  
  if int(res['throughput']) < 1 * 1024 * 1024:
    raise RuntimeError("Throughput is too low")


@pytest.mark.esp32
@pytest.mark.lan8720
@pytest.mark.ethernet_router
@pytest.mark.parametrize('erase_nvs', ['y'], indirect=True)
def test_first_join(
  dut: IdfDut,
  log_performance: Callable[[str, object], None],
  join_code: str
)-> None:
  # Fetch join code from the CLI
  if join_code is None:
    raise RuntimeError("Husarnet join code was not provided to the runner")
  
  # Join Husarnet network for the first time
  launch_test(dut, "join")
  
  dut.expect_exact("Starting Husarnet client...")
  
  dut.expect_exact("Hostname:")
  dut.write(DUT_HUSARNET_HOSTNAME)
  
  dut.expect_exact("Join code:")
  dut.write(join_code)
  
  dut.expect_unity_test_output(timeout=90)  
  
  # Ping websetup to ensure that the Husarnet connection is working
  launch_test_ping(dut, log_performance, "websetup_ping", WEBSETUP_IP, data_size=64)
  
@pytest.mark.esp32
@pytest.mark.lan8720
@pytest.mark.ethernet_router
def test_husarnet_performance(
  dut: IdfDut,
  log_performance: Callable[[str, object], None],
  join_code: str
)-> None:
  # Fetch join code from the CLI
  if join_code is None:
    raise RuntimeError("Husarnet join code was not provided to the runner")
  
  # Rejoin Husarnet network
  launch_test(dut, "join")
  
  dut.expect_exact("Starting Husarnet client...")
  
  dut.expect_exact("Hostname:")
  dut.write(DUT_HUSARNET_HOSTNAME)
  
  dut.expect_exact("Join code:")
  dut.write(join_code)
  
  dut.expect_unity_test_output()
  
  launch_test_ping(dut, log_performance, "husarnet_ping_warmup", HUSARNET_PING_IP, data_size=64, duration=3)
  
  launch_test_ping(dut, log_performance, "husarnet_ping_64B", HUSARNET_PING_IP, data_size=64)
  launch_test_ping(dut, log_performance, "husarnet_ping_1200B", HUSARNET_PING_IP, data_size=1200)
  
  res = launch_test_iperf(dut, log_performance, "husarnet_iperf", HUSARNET_PING_IP)
  
  if int(res['throughput']) < 1 * 1024 * 1024:
    raise RuntimeError("Throughput is too low")
