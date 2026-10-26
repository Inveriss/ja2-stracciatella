# JA2 data readers: SLF archives, 8-bit ETRLE STCI sprites, ja2pal.dat palettes.
import os
import struct

def slf_file(slf, want):
    f = open(slf, 'rb').read()
    n = struct.unpack_from('<i', f, 512)[0]
    base = len(f) - n * 280
    for i in range(n):
        e = f[base + i * 280: base + (i + 1) * 280]
        name = e[:256].split(b'\0')[0].decode().replace(chr(92), '/').lower()
        off, ln = struct.unpack_from('<II', e, 256)
        if name == want.lower():
            return f[off:off + ln]
    raise KeyError(want)

class Frame:
    def __init__(self, ox, oy, w, h, px):
        self.ox, self.oy, self.w, self.h, self.px = ox, oy, w, h, px  # px[y][x]: index or 0

def read_sti(d):
    assert d[:4] == b'STCI'
    ncol, nsub = struct.unpack_from('<IH', d, 24)
    p = 64
    pal = [tuple(d[p + 3 * i:p + 3 * i + 3]) for i in range(256)]; p += 768
    subs = [struct.unpack_from('<IIhhHH', d, p + 16 * i) for i in range(nsub)]
    data0 = p + 16 * nsub
    frames = []
    for off, ln, ox, oy, h, w in subs:
        src = d[data0 + off: data0 + off + ln]
        rows = []; row = []; i = 0
        while i < len(src):
            b = src[i]; i += 1
            if b == 0:
                row += [0] * (w - len(row)); rows.append(row[:w]); row = []
            elif b & 0x80:
                row += [0] * (b & 0x7F)
            else:
                row += list(src[i:i + b]); i += b
        while len(rows) < h: rows.append([0] * w)
        frames.append(Frame(ox, oy, w, h, rows))
    return pal, frames

def read_palreps(data_dir):
    f = open(os.path.join(data_dir, 'Binarydata.slf'), 'rb').read()
    n = struct.unpack_from('<i', f, 512)[0]; base = len(f) - n * 280
    for i in range(n):
        e = f[base + i * 280: base + (i + 1) * 280]
        if b'JA2PAL' in e[:256].upper():
            off, ln = struct.unpack_from('<II', e, 256); pal = f[off:off + ln]
    p = 0
    nr = struct.unpack_from('<I', pal, p)[0]; p += 4 + nr
    ranges = [(pal[p + 2 * i], pal[p + 2 * i + 1]) for i in range(nr)]; p += 2 * nr
    nrep = struct.unpack_from('<I', pal, p)[0]; p += 4
    reps = {}
    for i in range(nrep):
        t = pal[p]; p += 1
        ID = pal[p:p + 30].split(b'\0')[0].decode(); p += 30
        sz = pal[p]; p += 1
        reps[ID] = (t, [tuple(pal[p + 3 * k:p + 3 * k + 3]) for k in range(sz)]); p += 3 * sz
    return ranges, reps
