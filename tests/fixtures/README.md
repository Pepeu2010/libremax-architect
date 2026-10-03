# Archived render reader

`cycles_schema1.py` is the LibreMax 0.12.0 render reader from commit
`f9cffec8020cd882c2d12c6987f622829ac8c9ef`, with its companion `cycles_lights.py`.
Both retain the project's GPL-3.0-or-later license. They are test fixtures and are
not installed as application resources.

`scripts/verify-instance-renders.py` runs the current native app with the production
reader and this archived reader. It checks real Blender/Cycles images, mesh sharing,
preservation after an intentional failure and pixel differences. The archived
reader accepts only schema 1, so this also verifies negotiation for old render jobs.
