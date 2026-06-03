#!/usr/bin/env python3
import os
from pathlib import Path

# Get the current expanded home directory (e.g., /home/jane)
home = os.path.expanduser("~")
user = os.getlogin()
file = "compile_commands.json"
# Load
content = Path(file).read_text(encoding="utf-8")
# replace home with user's home directory
content = content.replace(home, f"/home/{user}")
# Save
Path(file).write_text(content, encoding="utf-8")
