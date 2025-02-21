# This is where software ports will go.

Currently, we have these ports:

- doomgeneric


# Doomgeneric
1. cd doomgeneric/
2. git clone [doomgeneric repository]
3. apply the patches
4. ``make`` and then tell the mkfs program about it in the root project's Makefile
5. Obtain a doom WAD (doom.wad or doom1.wad -- the latter is shareware)
6. ``doomgeneric -iwad /path/to/doom.wad``
