# This is where software ports will go.

Currently, we have these ports:

- doomgeneric
- dash


# Doomgeneric
1. cd doomgeneric/
2. git clone [doomgeneric repository]
3. apply the patches
4. ``make``
5. ``objcopy --remove-section .note.gnu.property doomgeneric`` and then tell the mkfs program about it in the root project's Makefile
6. Obtain a doom WAD (doom.wad or doom1.wad -- the latter is shareware)
7. ``doomgeneric -iwad /path/to/doom.wad``

# dash
1. cd dash/
2. git clone [dash repository on kernel.org]
3. apply the patches
4. ``make`` and then tell the mkfs program about it in the root project's Makefile
5. ``objcopy --remove-section .note.gnu.property dash`` and then tell the mkfs program about it in the root project's Makefile
6. ``dash -i``
