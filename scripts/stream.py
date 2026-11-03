import asyncio
from bleak import BleakScanner, BleakClient
import struct
import sys

WATCH_NAME = "ESPWatch"
CHR_UUID = "0a0bc9b6-3333-4f1c-b18a-58b11d03fe0a"

def on_data(_, data: bytearray):
  timestamp, steps, heart_rate, is_sleeping = struct.unpack("<IIB?", data)
  print(f"{timestamp},{steps},{heart_rate}, {is_sleeping}")

async def read_from_watch():
  dev = await BleakScanner.find_device_by_name(WATCH_NAME, timeout = 5)
  if dev is None:
    print("Device not found", file=sys.stderr)
    return
  print("Device found!", file=sys.stderr)
  print("timestamp,steps,heart rate,is_sleeping")
  async with BleakClient(dev, timeout = 5) as client:
    try:
      await client.start_notify(CHR_UUID, on_data)
      await asyncio.Event().wait() # run until ctrl-c
    except Exception as e:
      print(e, file=sys.stderr)
    finally:
      await client.disconnect()

if __name__ == "__main__":
  asyncio.run(read_from_watch())
