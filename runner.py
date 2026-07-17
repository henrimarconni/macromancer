import subprocess
import time

command = ["bash", "-c", "cmake -B build && cd build && ./macromancer infile1:outfile1 infile2:outfile2"]

print("Starting loop. Press Ctrl+C to exit.\n")

try:
    while True:
        result = subprocess.run(command, capture_output=True, text=True)
        print(result.stdout.strip())
        if result.stderr:
            print(f"Error: {result.stderr.strip()}")
        time.sleep(5)

except KeyboardInterrupt:
    print("\nLoop stopped.")

