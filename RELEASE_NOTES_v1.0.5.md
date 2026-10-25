# GLToogle v1.0.5

## Highlights
- **The version comes from the `VERSION` file.** The program's version text (what the update check compares with the `VERSION` file on GitHub), the resource and the package all take it from there now, as hDesktop and Cricket do, so a release changes one file. The number was typed into the code, the Makefile and the resource file before, and one of them could be left behind.
- The resource's version numbers were in the wrong fields (the third number in the second place); they are right now.

## Install
Install `GLToogle-1.0.5-1-x86_64.hpkg` on 64-bit Haiku, for the whole system. (There is no 32-bit package: the Makefile builds for 64-bit Haiku only.)

## Tested on
- Haiku R1/beta6: the 64-bit package was built (on 64-bit Haiku) and shows v1.0.5 in the program and in the package. It was not run.
