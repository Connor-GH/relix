# This is where software ports will go.

Currently, we have these ports:

Working:
- doomgeneric
- dash

Unsupported/WIP:
- mksh
- openlibm


# Doomgeneric
1. ``cd doomgeneric/``
2. ``git clone https://github.com/ozkl/doomgeneric``
3. apply the patches
4. ``make``
5. ``objcopy --remove-section .note.gnu.property doomgeneric`` and then tell the mkfs program about it in the root project's Makefile
6. Obtain a doom WAD (doom.wad or doom1.wad -- the latter is shareware)
7. ``/path/to/doomgeneric -iwad /path/to/doom.wad``

# dash
1. ``cd dash/``
2. ``wget https://git.kernel.org/pub/scm/utils/dash/dash.git/snapshot/dash-0.5.12.tar.gz && tar xvf dash-0.5.12.tar.gz``
3. apply the patches
4. ``make`` and then tell the mkfs program about it in the root project's Makefile
5. ``objcopy --remove-section .note.gnu.property dash`` and then tell the mkfs program about it in the root project's Makefile
6. Inside of Relix: ``/path/to/dash -i``
