# Prototype: find the weapon in JA2 soldier frames (grey palette ramp), its
# axis and grip, erase it and draw another weapon shape.
import math
from collections import deque
from ja2sprite import read_sti, slf_file, read_palreps  # noqa: F401

GUN_CORE = set(range(197, 203))   # 197..202: the weapon's greys
OUTLINE  = 203                    # darkest grey: weapon edge, also body outline
SHADOW   = 254
RANGES = {'hair': (245, 250), 'pants': (205, 219), 'skin': (235, 244), 'vest': (220, 234), 'boots': (173, 187)}

def part(v):
    if v == 0: return None
    if v == SHADOW: return 'shadow'
    for k, (s, e) in RANGES.items():
        if s <= v <= e: return k
    if v in GUN_CORE: return 'gun'
    if v == OUTLINE: return 'outline'
    return 'other'

N8 = [(-1, -1), (0, -1), (1, -1), (-1, 0), (1, 0), (-1, 1), (0, 1), (1, 1)]

def weapon_mask(f):
    """Weapon pixels: the grey core, plus outline pixels touching it that don't
    border a body part on the far side (the gun's own dark edge)."""
    px, w, h = f.px, f.w, f.h
    core = {(x, y) for y in range(h) for x in range(w) if px[y][x] in GUN_CORE}
    # blobs of the grey core; the weapon is the biggest group of blobs lying
    # within GAP pixels of each other (a hand over the gun splits it)
    GAP = 5
    seen, blobs = set(), []
    for p in sorted(core):
        if p in seen: continue
        blob, q = [], deque([p]); seen.add(p)
        while q:
            x, y = q.popleft(); blob.append((x, y))
            for dx, dy in N8:
                n = (x + dx, y + dy)
                if n in core and n not in seen: seen.add(n); q.append(n)
        blobs.append(blob)
    def near(a, b):
        return any(abs(x1 - x2) <= GAP and abs(y1 - y2) <= GAP for x1, y1 in a for x2, y2 in b)
    groups = []
    for b in blobs:
        hit = [g for g in groups if any(near(b, o) for o in g)]
        for g in hit: groups.remove(g)
        groups.append(sum(hit, []) + [b])
    best = max(groups, key=lambda g: sum(len(b) for b in g)) if groups else []
    keep = {p for b in best for p in b}
    # blobs further away but lying on the weapon's line (a muzzle cut off by
    # a gap) belong to it too
    if len(keep) >= 6:
        (cx, cy), (ux, uy), _, _ = axis(keep)
        for b in blobs:
            if b[0] in keep: continue
            if all(abs(-(x - cx) * uy + (y - cy) * ux) <= 1.5 for x, y in b):
                keep.update(b)
    mask = set(keep)
    for (x, y) in list(keep):
        for dx, dy in N8:
            nx, ny = x + dx, y + dy
            if 0 <= nx < w and 0 <= ny < h and px[ny][nx] == OUTLINE:
                mask.add((nx, ny))
    return mask

def axis(mask):
    """Principal axis of the weapon pixels: centre, unit direction, extent."""
    n = len(mask)
    cx = sum(x for x, _ in mask) / n; cy = sum(y for _, y in mask) / n
    sxx = sum((x - cx) ** 2 for x, _ in mask) / n
    syy = sum((y - cy) ** 2 for _, y in mask) / n
    sxy = sum((x - cx) * (y - cy) for x, y in mask) / n
    ang = 0.5 * math.atan2(2 * sxy, sxx - syy)
    ux, uy = math.cos(ang), math.sin(ang)
    us = [(x - cx) * ux + (y - cy) * uy for x, y in mask]
    vs = [-(x - cx) * uy + (y - cy) * ux for x, y in mask]
    return (cx, cy), (ux, uy), (min(us), max(us)), (min(vs), max(vs))

def grip(f, mask):
    """Hand points: weapon pixels touching skin."""
    pts = []
    for (x, y) in mask:
        for dx, dy in N8:
            nx, ny = x + dx, y + dy
            if 0 <= nx < f.w and 0 <= ny < f.h and part(f.px[ny][nx]) == 'skin':
                pts.append((x, y)); break
    return pts


BODY = ('vest', 'pants', 'skin', 'hair', 'boots', 'other')

def erase(f, mask):
    """Removes the weapon: a pixel mostly surrounded by body takes the commonest
    body index around it (the body seen behind the gun), else it goes clear."""
    px = [row[:] for row in f.px]
    todo = set(mask)
    for p in todo:
        px[p[1]][p[0]] = -1
    for _ in range(4):
        changed = False
        for (x, y) in sorted(todo):
            if px[y][x] != -1: continue
            nb = [px[y + dy][x + dx] for dx, dy in N8
                  if 0 <= x + dx < f.w and 0 <= y + dy < f.h and px[y + dy][x + dx] > 0
                  and part(px[y + dy][x + dx]) in BODY]
            if len(nb) >= 4:
                px[y][x] = max(set(nb), key=nb.count); changed = True
        if not changed: break
    for (x, y) in todo:
        if px[y][x] == -1: px[y][x] = 0
    return px

# The new weapon, in its own frame: u along the barrel (0 = stock end,
# 1 = muzzle of the OLD weapon's length), v across it in pixels (+ = up on
# screen). A long rifle with a scope and a curved magazine.
LENGTH = 1.35
def new_weapon_shape(u, v, L):
    t = u / L                      # 0..LENGTH
    if t < 0 or t > LENGTH: return None
    if t < 0.28:                   # stock, tapering
        return 'body' if -1.2 <= v <= 0.9 - 0.6 * (0.28 - t) / 0.28 else None
    if t < 0.70:                   # receiver
        if -1.0 <= v <= 1.0: return 'body'
        if 0.38 < t < 0.62 and 1.0 < v <= 2.2: return 'scope'
        if 0.52 < t < 0.62 and -3.2 + (t - 0.52) * 6 <= v < -1.0: return 'mag'
        return None
    if t < 0.82:                   # handguard
        return 'body' if -0.8 <= v <= 0.6 else None
    return 'barrel' if -0.4 <= v <= 0.4 else None   # long barrel

SHADES = {'scope': (199, 201), 'body': (200, 202), 'mag': (201, 202), 'barrel': (201, 202)}

def draw_weapon(f, px, mask, behind):
    """Draws the new weapon along the old one's axis: same stock end, same
    angle, LENGTH times its length; hands (skin, hair) stay over it, and a
    weapon behind the body only shows where nothing else is."""
    (cx, cy), (ux, uy), (u0, u1), _ = axis(mask)
    # the muzzle is the end away from the body
    body = [(x, y) for y in range(f.h) for x in range(f.w) if part(f.px[y][x]) in ('vest', 'pants')]
    bx = sum(x for x, _ in body) / len(body); by = sum(y for _, y in body) / len(body)
    e0 = (cx + ux * u0, cy + uy * u0); e1 = (cx + ux * u1, cy + uy * u1)
    if math.hypot(e0[0] - bx, e0[1] - by) > math.hypot(e1[0] - bx, e1[1] - by):
        ux, uy = -ux, -uy; u0, u1 = -u1, -u0
    sx, sy = cx + ux * u0, cy + uy * u0          # stock end
    L = max(u1 - u0, 4.0)
    nx, ny = uy, -ux                              # across; make + point up
    if ny > 0: nx, ny = -nx, -ny
    out = [row[:] for row in px]
    shape = {}
    for y in range(f.h):
        for x in range(f.w):
            hits = []
            for ox, oy in ((0.25, 0.25), (0.75, 0.25), (0.25, 0.75), (0.75, 0.75)):
                dx, dy = x + ox - 0.5 - sx, y + oy - 0.5 - sy
                k = new_weapon_shape(dx * ux + dy * uy, dx * nx + dy * ny, L)
                if k: hits.append((k, dx * nx + dy * ny))
            if len(hits) >= 2:
                shape[(x, y)] = hits
    for (x, y), hits in shape.items():
        cur = out[y][x]
        p = part(cur)
        if p in ('skin', 'hair'): continue
        if behind and cur > 0 and (x, y) not in mask and p != 'shadow': continue
        edge = any((x + dx, y + dy) not in shape for dx, dy in N8)
        kind = max(set(k for k, _ in hits), key=[k for k, _ in hits].count)
        v = sum(vv for _, vv in hits) / len(hits)
        lit, dark = SHADES[kind]
        out[y][x] = OUTLINE if edge and v < 0 else (lit if v > 0 else dark)
    return out, (sx, sy), (ux, uy), L
