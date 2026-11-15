"""MediaPipe world landmarks -> local bone transforms for the Rookie (Manny-named) skeleton.
All maths in UE component space (cm, +X = character left, +Y = forward, +Z = up). Quats are (x,y,z,w), v' = q v q^-1."""
import sys, json, numpy as np, cv2

pose_path, skel_path, out_path, dbg_path = sys.argv[1:5]
END = int(sys.argv[5]) if len(sys.argv) > 5 else 336

# ---------- quaternion helpers ----------
def qmul(a, b):
    ax, ay, az, aw = a; bx, by, bz, bw = b
    return np.array([aw*bx + ax*bw + ay*bz - az*by, aw*by - ax*bz + ay*bw + az*bx, aw*bz + ax*by - ay*bx + az*bw, aw*bw - ax*bx - ay*by - az*bz])
def qinv(q): return np.array([-q[0], -q[1], -q[2], q[3]])
def qrot(q, v):
    u = q[:3]; return v + 2 * np.cross(u, np.cross(u, v) + q[3] * v)
def qnorm(q): return q / np.linalg.norm(q)
def unit(v): return v / (np.linalg.norm(v) + 1e-9)
def arc(a, b):
    a, b = unit(a), unit(b); d = float(np.dot(a, b))
    if d < -0.9999:
        ax = unit(np.cross(a, [1, 0, 0]) if abs(a[0]) < 0.9 else np.cross(a, [0, 1, 0])); return np.array([*ax, 0.0])
    return qnorm(np.array([*np.cross(a, b), 1 + d]))
def basis(p, s):
    x = unit(p); z = unit(np.cross(x, s)); y = np.cross(z, x); return np.stack([x, y, z], axis=1)
def m2q(m):
    t = np.trace(m)
    if t > 0:
        s = np.sqrt(t + 1) * 2; q = [(m[2,1]-m[1,2])/s, (m[0,2]-m[2,0])/s, (m[1,0]-m[0,1])/s, s/4]
    else:
        i = int(np.argmax(np.diag(m))); j, k = (i+1) % 3, (i+2) % 3
        s = np.sqrt(1 + m[i,i] - m[j,j] - m[k,k]) * 2; q = [0, 0, 0, 0]
        q[i] = s/4; q[j] = (m[j,i]+m[i,j])/s; q[k] = (m[k,i]+m[i,k])/s; q[3] = (m[k,j]-m[j,k])/s
    return qnorm(np.array(q))
def frame_delta(p0, s0, p1, s1): return m2q(basis(p1, s1) @ basis(p0, s0).T)
def slerp(a, b, t):
    d = float(np.dot(a, b))
    if d < 0: b, d = -b, -d
    if d > 0.9995: return qnorm(a + t * (b - a))
    th = np.arccos(d); return (np.sin((1-t)*th) * a + np.sin(t*th) * b) / np.sin(th)
ID = np.array([0, 0, 0, 1.0])

# ---------- skeleton ----------
sk = {b['n']: b for b in json.load(open(skel_path))}
PARENT = {'pelvis': 'root', 'spine_01': 'pelvis', 'spine_02': 'spine_01', 'spine_03': 'spine_02', 'spine_04': 'spine_03', 'spine_05': 'spine_04',
          'neck_01': 'spine_05', 'neck_02': 'neck_01', 'head': 'neck_02'}
for s in 'lr':
    PARENT.update({f'clavicle_{s}': 'spine_05', f'upperarm_{s}': f'clavicle_{s}', f'lowerarm_{s}': f'upperarm_{s}', f'hand_{s}': f'lowerarm_{s}',
                   f'thigh_{s}': 'pelvis', f'calf_{s}': f'thigh_{s}', f'foot_{s}': f'calf_{s}', f'ball_{s}': f'foot_{s}'})
ORDER = list(PARENT.keys())
RP = {n: np.array(sk[n]['w']['t']) for n in sk}   # ref component position
RQ = {n: np.array(sk[n]['w']['q']) for n in sk}   # ref component rotation
# check the assumed hierarchy against the ref local transforms
for n, p in PARENT.items():
    lt = qrot(qinv(RQ[p]), RP[n] - RP[p])
    assert np.allclose(lt, sk[n]['l']['t'], atol=0.05), (n, p, lt, sk[n]['l']['t'])

# ---------- landmarks ----------
data = json.load(open(pose_path)); fps = data['fps']
L = np.array([[p[:3] for p in fr['w']] for fr in data['frames'][:END]])  # (F,33,3) metres
P = np.stack([L[..., 0], -L[..., 2], -L[..., 1]], axis=-1) * 100.0          # -> UE cm
F = P.shape[0]
# temporal smoothing
k = np.exp(-0.5 * (np.arange(-4, 5) / 1.3) ** 2); k /= k.sum()
Pp = np.pad(P, ((4, 4), (0, 0), (0, 0)), mode='edge')
P = sum(k[i] * Pp[i:i + F] for i in range(9))
# camera pitch correction: make the standing pose of the first frames upright (rotate about X)
v = (P[:5, 11] + P[:5, 12]).mean(0) / 2 - (P[:5, 27] + P[:5, 28]).mean(0) / 2
ang = np.arctan2(v[1], v[2]); c, s_ = np.cos(ang), np.sin(ang)
Rx = np.array([[1, 0, 0], [0, c, -s_], [0, s_, c]])
P = P @ Rx.T
print('pitch correction deg', np.degrees(ang))

LM = dict(nose=0, ear_l=7, ear_r=8, sh_l=11, sh_r=12, el_l=13, el_r=14, wr_l=15, wr_r=16, hip_l=23, hip_r=24, kn_l=25, kn_r=26, an_l=27, an_r=28, toe_l=31, toe_r=32)

def solve(p):
    g = lambda n: p[LM[n]]
    D = {}
    hip_mid = (g('hip_l') + g('hip_r')) / 2; sh_mid = (g('sh_l') + g('sh_r')) / 2
    r_hip = RP['thigh_l'] - RP['thigh_r']; r_sh = RP['upperarm_l'] - RP['upperarm_r']
    r_up = (RP['upperarm_l'] + RP['upperarm_r']) / 2 - (RP['thigh_l'] + RP['thigh_r']) / 2
    up = sh_mid - hip_mid
    d_pel = frame_delta(r_up, r_hip, up, g('hip_l') - g('hip_r'))
    d_chest = frame_delta(r_up, r_sh, up, g('sh_l') - g('sh_r'))
    # head: ear line + nose direction; ref head looks along +Y with ears along +X
    ear_mid = (g('ear_l') + g('ear_r')) / 2
    fwd = g('nose') - ear_mid; fwd = fwd - np.array([0, 0, 1.0]) * 0  # keep as is
    d_head = frame_delta(np.array([1.0, 0, 0]), np.array([0, 1.0, -0.25]), g('ear_l') - g('ear_r'), fwd)
    d_head = slerp(d_chest, d_head, 0.8)
    D['pelvis'] = d_pel
    for i, n in enumerate(['spine_01', 'spine_02', 'spine_03', 'spine_04', 'spine_05']): D[n] = slerp(d_pel, d_chest, (i + 1) / 5)
    D['neck_01'] = slerp(d_chest, d_head, 0.35); D['neck_02'] = slerp(d_chest, d_head, 0.7); D['head'] = d_head
    for s in 'lr':
        D[f'clavicle_{s}'] = d_chest
        def aim(parent_d, r0, r1, t0, t1):
            return qmul(arc(qrot(parent_d, RP[r1] - RP[r0]), g(t1) - g(t0)), parent_d)
        D[f'upperarm_{s}'] = aim(d_chest, f'upperarm_{s}', f'lowerarm_{s}', f'sh_{s}', f'el_{s}')
        D[f'lowerarm_{s}'] = aim(D[f'upperarm_{s}'], f'lowerarm_{s}', f'hand_{s}', f'el_{s}', f'wr_{s}')
        D[f'hand_{s}'] = D[f'lowerarm_{s}']
        D[f'thigh_{s}'] = aim(d_pel, f'thigh_{s}', f'calf_{s}', f'hip_{s}', f'kn_{s}')
        D[f'calf_{s}'] = aim(D[f'thigh_{s}'], f'calf_{s}', f'foot_{s}', f'kn_{s}', f'an_{s}')
        D[f'foot_{s}'] = slerp(D[f'calf_{s}'], aim(D[f'calf_{s}'], f'foot_{s}', f'ball_{s}', f'an_{s}', f'toe_{s}'), 0.7)
        D[f'ball_{s}'] = D[f'foot_{s}']
    return D

def fk(D, pelvis_pos):
    WQ = {'root': RQ['root']}; WP = {'root': RP['root']}
    for n in ORDER:
        p = PARENT[n]
        WQ[n] = qnorm(qmul(D[n], RQ[n]))
        WP[n] = pelvis_pos if n == 'pelvis' else WP[p] + qrot(WQ[p], np.array(sk[n]['l']['t']))
    return WQ, WP

# pass 1: rotations
Ds = [solve(P[f]) for f in range(F)]
# ease the last frames back to the first pose so it hands back to idle cleanly
EASE = 12
for f in range(F - EASE, F):
    t = (f - (F - EASE) + 1) / EASE; t = t * t * (3 - 2 * t)
    for n in ORDER: Ds[f][n] = slerp(Ds[f][n], Ds[0][n], t)
# pass 2: pelvis position. Feet stay planted in this clip: keep the ankle midpoint over its ref XY, and the lowest foot on the floor.
pel = np.zeros((F, 3))
ref_mid = (RP['foot_l'] + RP['foot_r']) / 2
for f in range(F):
    WQ, WP = fk(Ds[f], RP['pelvis'])
    mid = (WP['foot_l'] + WP['foot_r']) / 2
    low = min(WP['foot_l'][2] - RP['foot_l'][2], WP['foot_r'][2] - RP['foot_r'][2], WP['ball_l'][2] - RP['ball_l'][2], WP['ball_r'][2] - RP['ball_r'][2])
    pel[f] = RP['pelvis'] + np.array([ref_mid[0] - mid[0], ref_mid[1] - mid[1], -low])
k2 = np.exp(-0.5 * (np.arange(-5, 6) / 2.0) ** 2); k2 /= k2.sum()
pp = np.pad(pel, ((5, 5), (0, 0)), mode='edge'); pel = sum(k2[i] * pp[i:i + F] for i in range(11))
print('pelvis z range', pel[:, 2].min(), pel[:, 2].max(), 'ref', RP['pelvis'][2], 'x range', pel[:, 0].min(), pel[:, 0].max())

# pass 3: locals
out = {n: [] for n in ORDER}; prev = {}
world_frames = []
for f in range(F):
    WQ, WP = fk(Ds[f], pel[f]); world_frames.append(WP)
    for n in ORDER:
        p = PARENT[n]
        lq = qnorm(qmul(qinv(WQ[p]), WQ[n]))
        if n in prev and np.dot(prev[n], lq) < 0: lq = -lq
        prev[n] = lq
        lt = qrot(qinv(WQ[p]), WP[n] - WP[p])
        out[n].append([*map(float, lt), *map(float, lq)])
# sanity: frame identity check -> zero deltas reproduce the ref locals
WQ, WP = fk({n: ID for n in ORDER}, RP['pelvis'])
for n in ORDER:
    lq = qmul(qinv(WQ[PARENT[n]]), WQ[n]); rq = np.array(sk[n]['l']['q'])
    assert abs(abs(np.dot(lq, rq)) - 1) < 1e-3, n
json.dump({'fps': fps, 'frames': F, 'bones': out}, open(out_path, 'w'))
print('wrote', F, 'frames', len(ORDER), 'bones')

# debug: front-view stick figure (as a mirror would show the camera: +X on the right, like the video)
EDGES = [(n, PARENT[n]) for n in ORDER if PARENT[n] != 'root']
tiles = []
for f in range(0, F, 20):
    im = np.full((360, 220, 3), 30, np.uint8); WP = world_frames[f]
    pt = lambda v: (int(110 + v[0] * 1.8), int(340 - v[2] * 1.8))
    for a, b in EDGES:
        col = (80, 200, 255) if a.endswith('_l') else (255, 160, 80) if a.endswith('_r') else (220, 220, 220)
        cv2.line(im, pt(WP[a]), pt(WP[b]), col, 3)
    cv2.line(im, (0, 340), (220, 340), (90, 90, 90), 1); cv2.putText(im, str(f), (5, 25), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 255), 2)
    tiles.append(im)
c = 9; rows = [np.hstack(tiles[i:i + c] + [np.zeros_like(tiles[0])] * (c - len(tiles[i:i + c]))) for i in range(0, len(tiles), c)]
cv2.imwrite(dbg_path, np.vstack(rows))
