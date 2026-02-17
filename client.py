import sys
import time
import requests
import json
import subprocess
import re

# --- SETTINGS ---
ESP_IP = "10.129.117.118"
ESP_PORT = 8085
API_KEY = "1234567890"
# ---------------------

ESP_URL = f"http://{ESP_IP}:{ESP_PORT}/update"


def check_sensors_installed():
    """Checks if lm-sensors is installed"""
    try:
        subprocess.run(['sensors', '-v'],
                       stdout=subprocess.PIPE,
                       stderr=subprocess.PIPE,
                       check=True)
        return True
    except (subprocess.CalledProcessError, FileNotFoundError):
        return False


def parse_sensors_output():
    """
    Parses the output of the 'sensors' command and extracts CPU, GPU temperatures and fan speed
    """
    try:
        result = subprocess.run(['sensors'],
                                stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE,
                                text=True,
                                check=True)
        output = result.stdout

        cpu_temp = 0.0
        gpu_temp = 0.0
        fan_speed = 0

        # Split the output into blocks (each chip is a separate block)
        lines = output.split('\n')

        current_chip = ""
        for line in lines:
            if line and not line.startswith(' ') and ':' not in line:
                current_chip = line.lower()
                continue

            line_lower = line.lower()

            if any(keyword in line_lower for keyword in [
                'core 0:', 'package id 0:', 'tctl:', 'cpu temperature:',
                'core temp:', 'cpu temp:', 'tdie:'
            ]) and cpu_temp == 0.0:
                match = re.search(r'[+\-]?([\d.]+)°[CF]', line)
                if match:
                    temp = float(match.group(1))
                    # Convert Fahrenheit to Celsius if needed
                    if '°f' in line_lower:
                        temp = (temp - 32) * 5 / 9
                    cpu_temp = temp
                    print(f"  ✓ CPU Temp found: {cpu_temp:.1f}°C (in line: {line.strip()})")

            if any(gpu_name in current_chip for gpu_name in ['amdgpu', 'radeon', 'nouveau', 'nvidia']):
                if 'edge:' in line_lower or 'temp1:' in line_lower or 'gpu temp:' in line_lower:
                    match = re.search(r'[+\-]?([\d.]+)°[CF]', line)
                    if match:
                        temp = float(match.group(1))
                        if '°f' in line_lower:
                            temp = (temp - 32) * 5 / 9
                        gpu_temp = temp
                        print(f"  ✓ GPU Temp found: {gpu_temp:.1f}°C (in line: {line.strip()})")

            if 'fan' in line_lower and 'rpm' in line_lower and fan_speed == 0:
                match = re.search(r'(\d+)\s*rpm', line_lower)
                if match:
                    fan_speed = int(match.group(1))
                    print(f"  ✓ Fan Speed found: {fan_speed} RPM (in line: {line.strip()})")

        if cpu_temp == 0.0:
            for line in lines:
                if 'temp1:' in line.lower() or 'temp2:' in line.lower():
                    match = re.search(r'[+\-]?([\d.]+)°[CF]', line)
                    if match:
                        temp = float(match.group(1))
                        if '°f' in line.lower():
                            temp = (temp - 32) * 5 / 9
                        if temp > 20:
                            cpu_temp = temp
                            print(f"  ⚠ CPU Temp (fallback): {cpu_temp:.1f}°C")
                            break

        return cpu_temp, gpu_temp, fan_speed

    except subprocess.CalledProcessError as e:
        print(f"  ✗ Error executing 'sensors': {e}")
        return 0.0, 0.0, 0
    except Exception as e:
        print(f"  ✗ Parsing error: {e}")
        return 0.0, 0.0, 0


def get_sensor_data():
    cpu_temp, gpu_temp, fan_speed = parse_sensors_output()

    return {
        "cpu_temp": cpu_temp,
        "gpu_temp": gpu_temp,
        "fan_speed": fan_speed
    }


def main():
    print("\n" + "=" * 50)
    print("Starting PC monitoring client (sensors)")
    print(f"Sending data to: {ESP_URL}")
    print("=" * 50 + "\n")

    if not check_sensors_installed():
        print("✗ ERROR: 'sensors' command not found!")
        print("\nInstall lm-sensors:")
        print("  Ubuntu/Debian: sudo apt-get install lm-sensors")
        print("  Fedora/RHEL:   sudo dnf install lm_sensors")
        print("  Arch:          sudo pacman -S lm_sensors")
        print("\nAfter installation, run: sudo sensors-detect")
        sys.exit(1)

    print("✓ 'sensors' command found\n")

    print("=== TEST RUN ===")
    test_data = get_sensor_data()
    print(f"\nResult: CPU={test_data['cpu_temp']:.1f}°C, "
          f"GPU={test_data['gpu_temp']:.1f}°C, "
          f"FAN={test_data['fan_speed']} RPM\n")

    if test_data['cpu_temp'] == 0 and test_data['gpu_temp'] == 0:
        print("⚠ WARNING: All sensors returned 0!")
        print("You may need to run: sudo sensors-detect")
        print("\nFull sensors output:")
        print("-" * 50)
        subprocess.run(['sensors'])
        print("-" * 50)
        print("\nContinue sending data? (y/n): ", end='')
        if input().lower() != 'y':
            sys.exit(0)

    print("\n=== STARTING MONITORING ===\n")

    iteration = 0
    while True:
        iteration += 1
        print(f"[Iteration {iteration}] Reading sensors...")

        sensor_data = get_sensor_data()

        headers = {
            "X-API-Key": API_KEY,
            "Content-Type": "application/json"
        }

        try:
            response = requests.post(
                ESP_URL,
                data=json.dumps(sensor_data),
                headers=headers,
                timeout=2
            )

            if response.status_code == 200:
                print(f"✓ Data sent: CPU={sensor_data['cpu_temp']:.0f}°C, "
                      f"GPU={sensor_data['gpu_temp']:.0f}°C, "
                      f"FAN={sensor_data['fan_speed']} RPM\n")
            else:
                print(f"✗ ESP responded with code: {response.status_code} - {response.text}\n")

        except requests.exceptions.RequestException as e:
            print(f"✗ Failed to connect to ESP8266")
            print(f"  Check the IP ({ESP_IP}) and Wi-Fi connection\n")

        time.sleep(1)


if __name__ == "__main__":
    main()