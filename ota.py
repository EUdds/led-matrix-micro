import argparse
import requests

def parse_args():
    parser = argparse.ArgumentParser(description="OTA Update Script")
    
    parser.add_argument(
        "-f",
        "--file",
        type=str,
        required=True,
        help="Path to the firmware file to be uploaded",
    )

    parser.add_argument(
        "-i",
        "--ip",
        type=str,
        required=True,
        help="IP address of the device to update",
    )

    parser.add_argument(
        "-p",
        "--port",
        type=int,
        default=80,
        help="Port of the device to update (default: 80)",
    )

    return parser.parse_args()


def main():
    args = parse_args()

    url = f"http://{args.ip}:{args.port}/update"
    
    with open(args.file, 'rb') as firmware_file:
        firmware_data = firmware_file.read()
        headers = {'Content-Type': 'application/octet-stream',
                   'Content-Length': str(len(firmware_data))}
        response = requests.post(url, data=firmware_data, headers=headers)

    if response.status_code == 200:
        print("Firmware update successful!")
    else:
        print(f"Failed to update firmware: {response.status_code} - {response.text}")


if __name__ == "__main__":
    main()