import re
import subprocess
import time
import csv
import logging
import socket
import requests
from typing import Callable
from dataclasses import dataclass, asdict
from pytest_embedded_idf.dut import IdfDut
from pytest_embedded_idf.unity_tester import READY_PATTERN_LIST

PING_STATISTICS_PATTERN = re.compile(
        r'(?P<transmitted>\d+) packets transmitted, (?P<received>\d+) received, time (?P<time>\d+) ms')

PING_RTT_PATTERN = re.compile(
        r'rtt min\/avg\/max = (?P<min>\d+)\/(?P<avg>\d+.\d+)\/(?P<max>\d+) ms')

IPERF_LOG_FILE = ".iperf.log"
IPERF_TIMEOUT = 20
IPERF_CSV_FIELDS = [
  "timestamp",
  "source_address",
  "source_port",
  "destination_address",
  "destination_port",
  "group_id",
  "interval",
  "transferred_bytes",
  "throughput"
]

# Launch a test by selecting it by name.
# This way is considered obsolete by Espressif, but as far
# as I know it's the only way to run an interactive test
# without blocking on waiting for the unity test output.
def launch_test(dut: IdfDut, test_name: str) -> None:
  dut.expect_exact(READY_PATTERN_LIST)
  dut.write(f'"{test_name}"')

@dataclass
class PingResult:
  transmitted: int = 0
  received: int = 0
  time: int = 0
  min: int = 0
  avg: float = 0
  max: int = 0
  
  def dict(self):
    return {k: str(v) for k, v in asdict(self).items()}

def launch_test_ping(
  dut: IdfDut,
  log_performance: Callable[[str, object], None],
  name: str,
  target_ip: str,
  data_size: int = 64,
  duration: int = 15
) -> PingResult:
  launch_test(dut, "ping")
  
  # Fill in the ping parameters
  dut.expect_exact("Target IP address:")
  dut.write(target_ip)
  
  dut.expect_exact("Data size:")
  dut.write(str(data_size))
  
  dut.expect_exact("Duration:")
  dut.write(str(duration))
  
  # Parse the ping statistics
  result = PingResult()
  
  match = dut.expect(PING_STATISTICS_PATTERN, timeout = duration + 3)
  if match is not None:
    result.transmitted = int(match.group("transmitted"))
    result.received = int(match.group("received"))
    result.time = int(match.group("time"))
  else:
    raise RuntimeError("Ping statistics not found")
  
  match = dut.expect(PING_RTT_PATTERN, timeout = 3)
  if match is not None:
    result.min = int(match.group("min"))
    result.avg = float(match.group("avg"))
    result.max = int(match.group("max"))
  else:
    raise RuntimeError("Ping RTT statistics not found")
  
  dut.expect_unity_test_output()
  
  log_performance(name, result.dict())
  
  return result

def launch_test_iperf(
  dut: IdfDut,
  log_performance: Callable[[str, object], None],
  name: str,
  target_ip: str
) -> dict[str, str]:
  launch_test(dut, "iperf_client")
  
  # Pass the target IP address
  dut.expect_exact("Target IP address:")
  dut.write(target_ip)
  
  output = []
  
  # Start the iperf server
  with open(IPERF_LOG_FILE, 'w') as f:
    process = subprocess.Popen(['iperf', '-s', '-y', 'C', '-i', '3', '-P', '1', '-V'], stdout=f, stderr=f)
    for _ in range(IPERF_TIMEOUT):
      if process.poll() is not None:
          break
      time.sleep(1)
    else:
      process.terminate()
      raise RuntimeError("iperf server did not finish in time")
    
  # Parse the iperf output
  with open(IPERF_LOG_FILE, 'r') as f:
    reader = csv.DictReader(f, fieldnames=IPERF_CSV_FIELDS, delimiter=',')
    
    for row in reader:
      logging.info("IPERF: " + str(row))
      output.append(row)
      
  if len(output) == 0:
    raise RuntimeError("No iperf output")
  
  # Log performance
  log_performance(name, output[-1])
  
  dut.expect_unity_test_output()
  
  return output[-1]

# Get runner's local lan IP address
def get_lan_ip() -> str:
  sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
  # Connecting UDP socket does not generate any traffic
  sock.connect(('8.8.8.8', 53))
  return sock.getsockname()[0] # Return IP address

# Get runner's Husarnet IP address
def get_husarnet_ip() -> str:
  # Connect to the local Husarnet daemon API
  r = requests.get('http://127.0.0.1:16216/api/status')
  return r.json()["result"]["local_ip"]
