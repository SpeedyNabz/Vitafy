"""Exercise preference persistence across clean processes in disposable data."""
import os
import subprocess
import sys
import tempfile

with tempfile.TemporaryDirectory(prefix='vitafy-prefs-') as directory:
    for phase in ('write','read'):
        subprocess.run([sys.argv[1],directory+os.sep,phase],check=True)
