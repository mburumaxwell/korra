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

def parse_version(version):
  parts = version.split("+")
  base = parts[0]
  tweak = parts[1] if len(parts) > 1 else "0"
  major, minor, patch = (base.split(".") + ["0", "0"])[:3]
  return int(major), int(minor), int(patch), tweak

def write_version_header(version, output_path="firmware-pio/include/app_version.h"):
  os.makedirs(os.path.dirname(output_path), exist_ok=True)
  major, minor, patch, tweak = parse_version(version)
  version_number = (major << 16) | (minor << 8) | patch
  app_version = (major << 16) | (minor << 8)
  extended_str = f"{major}.{minor}.{patch}+{tweak}"
  tweak_str = extended_str
  sha = get_git_short_sha()

  with open(output_path, "w") as f:
    f.write("// Auto-generated file, do not edit\n\n")
    f.write("#ifndef _APP_VERSION_H_\n")
    f.write("#define _APP_VERSION_H_\n\n")

    f.write(f"#define APPVERSION                   0x{app_version:05x}\n")
    f.write(f"#define APP_VERSION_NUMBER           0x{version_number:05x}\n")
    f.write(f"#define APP_VERSION_MAJOR            {major}\n")
    f.write(f"#define APP_VERSION_MINOR            {minor}\n")
    f.write(f"#define APP_PATCHLEVEL               {patch}\n")
    f.write(f"#define APP_TWEAK                    {tweak}\n")
    f.write(f'#define APP_VERSION_STRING           "{major}.{minor}.{patch}"\n')
    f.write(f'#define APP_VERSION_EXTENDED_STRING  "{extended_str}"\n')
    f.write(f'#define APP_VERSION_TWEAK_STRING     "{tweak_str}"\n\n')

    f.write(f'#define APP_BUILD_VERSION {sha}\n\n')

    f.write("#endif // _APP_VERSION_H_\n")

version = read_version_file()
write_version_header(version)
print(f"Generated firmware-pio/include/app_version.h")
