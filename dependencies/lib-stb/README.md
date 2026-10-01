# stb_image

Bundled copy of `stb_image.h` from https://github.com/nothings/stb, used by
`src/sgp/PNG.cc` to decode PNG files.

- Version: v2.30
- Upstream commit: `2c980bb59875b0d32144a71867fbdebb2f77cd20`
- File: https://raw.githubusercontent.com/nothings/stb/2c980bb59875b0d32144a71867fbdebb2f77cd20/stb_image.h
- License: MIT or public domain, at your choice (see `LICENSE`, copied from
  the end of `stb_image.h`)

The file is unmodified. To update it, replace `stb_image.h` with a newer
upstream version and update the commit hash above.

To build against a system copy instead, configure with
`-DLOCAL_STB_LIB=OFF -DSTB_INCLUDE_DIR=<directory containing stb_image.h>`.
