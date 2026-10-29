"""Compatibility entry point: profile/source sets, not serialized runtime merge data."""
from pathlib import Path
exec((Path(__file__).parent/'activate_handyman.py').read_text())
