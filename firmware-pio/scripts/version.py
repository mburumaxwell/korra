import os

import os
import subprocess

def read_version_file(path="firmware-pio/version.txt"):
  if os.path.isfile(path):
    with open(path, "r") as f:
      return f.read().strip()
  return "0.1.0"

def get_git_short_sha():
  try:
    full_sha = subprocess.check_output(
      ["git", "rev-parse", "HEAD"],
      stderr=subprocess.DEVNULL
    ).decode().strip()
    return full_sha[:12]
  except subprocess.CalledProcessError:
    return "000000000000"

def get_version():
  base_version = read_version_file()
  version = f"{base_version}"

  return version

def write_version_header(version, output_path="firmware-pio/include/app_version.h"):
  os.makedirs(os.path.dirname(output_path), exist_ok=True)
  sha = get_git_short_sha()
  with open(output_path, "w") as f:
    f.write("// Auto-generated file, do not edit\n\n")
    f.write("#ifndef VERSION_H // VERSION_H\n\n")
    f.write(f'#define APP_VERSION_STRING "{version}"\n')
    f.write(f'\n')
    f.write(f'#define APP_BUILD_VERSION "{sha}"\n')
    f.write("\n#endif // VERSION_H\n")

version = get_version()
write_version_header(version)
print(f"Generated firmware-pio/include/app_version.h")
