#!/usr/bin/env python3
"""Simple version management script"""

import sys
import json
import os

# Read version from .env file or use defaults
def read_version_from_env():
    version = {
        "major": 1,
        "minor": 0,
        "patch": 1,
        "build": 1
    }
    
    env_path = os.path.join(os.path.dirname(os.path.dirname(__file__)), '.env')
    if os.path.exists(env_path):
        with open(env_path, 'r') as f:
            for line in f:
                line = line.strip()
                if line.startswith('VERSION_MAJOR='):
                    version['major'] = int(line.split('=')[1])
                elif line.startswith('VERSION_MINOR='):
                    version['minor'] = int(line.split('=')[1])
                elif line.startswith('VERSION_PATCH='):
                    version['patch'] = int(line.split('=')[1])
    
    return version

# Default version
VERSION = read_version_from_env()

def main():
    if "--export-only" in sys.argv:
        # Export version as shell variables
        print(f"export PROJECT_VERSION={VERSION['major']}.{VERSION['minor']}.{VERSION['patch']}")
        print(f"export PROJECT_VERSION_MAJOR={VERSION['major']}")
        print(f"export PROJECT_VERSION_MINOR={VERSION['minor']}")
        print(f"export PROJECT_VERSION_PATCH={VERSION['patch']}")
        print(f"export BUILD_NUMBER={VERSION['build']}")
        # Calculate AU version
        au_version = (VERSION['major'] << 16) | (VERSION['minor'] << 8) | VERSION['patch']
        print(f"export AU_VERSION_INT={au_version}")
    else:
        # Just print version for now
        print(f"Version {VERSION['major']}.{VERSION['minor']}.{VERSION['patch']} (build {VERSION['build']})")

if __name__ == "__main__":
    main()