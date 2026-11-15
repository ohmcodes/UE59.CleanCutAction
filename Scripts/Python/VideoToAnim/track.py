"""Track body pose per frame with MediaPipe PoseLandmarker. Saves world landmarks (metres, hip-centred) + image landmarks."""
import sys, json, cv2, numpy as np, mediapipe as mp
from mediapipe.tasks import python as mpt
from mediapipe.tasks.python import vision

src, model, out, dbg = sys.argv[1:5]
cap = cv2.VideoCapture(src)
fps = cap.get(cv2.CAP_PROP_FPS)
W = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH)); H = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))
# find the non-black column range (pillarboxed portrait clip) from a mid frame
cap.set(cv2.CAP_PROP_POS_FRAMES, 100); ok, f = cap.read()
cols = np.where(f.max(axis=(0, 2)) > 30)[0]
x0, x1 = int(cols.min()), int(cols.max()) + 1
print('crop x', x0, x1)
cap.set(cv2.CAP_PROP_POS_FRAMES, 0)

opts = vision.PoseLandmarkerOptions(
    base_options=mpt.BaseOptions(model_asset_path=model),
    running_mode=vision.RunningMode.VIDEO, num_poses=1,
    min_pose_detection_confidence=0.5, min_pose_presence_confidence=0.5, min_tracking_confidence=0.5)
frames = []
dbg_tiles = []
with vision.PoseLandmarker.create_from_options(opts) as lm:
    i = 0
    while True:
        ok, f = cap.read()
        if not ok: break
        crop = np.ascontiguousarray(f[:, x0:x1])
        img = mp.Image(image_format=mp.ImageFormat.SRGB, data=cv2.cvtColor(crop, cv2.COLOR_BGR2RGB))
        r = lm.detect_for_video(img, int(i * 1000 / fps))
        if r.pose_world_landmarks:
            wl = [[p.x, p.y, p.z, p.visibility] for p in r.pose_world_landmarks[0]]
            il = [[p.x, p.y, p.z, p.visibility] for p in r.pose_landmarks[0]]
            frames.append({'i': i, 'w': wl, 'img': il})
            if i % 20 == 0:
                d = crop.copy()
                for p in il:
                    cv2.circle(d, (int(p[0] * d.shape[1]), int(p[1] * d.shape[0])), 5, (0, 255, 0) if p[3] > 0.5 else (0, 0, 255), -1)
                cv2.putText(d, str(i), (5, 60), cv2.FONT_HERSHEY_SIMPLEX, 1.5, (0, 255, 255), 3)
                dbg_tiles.append(cv2.resize(d, (int(d.shape[1] * 360 / d.shape[0]), 360)))
        else:
            frames.append({'i': i, 'w': None, 'img': None})
        i += 1
json.dump({'fps': fps, 'frames': frames}, open(out, 'w'))
print('frames', len(frames), 'missing', sum(1 for fr in frames if fr['w'] is None))
n = len(dbg_tiles); c = 9
rows = [np.hstack(dbg_tiles[k:k + c] + [np.zeros_like(dbg_tiles[0])] * (c - len(dbg_tiles[k:k + c]))) for k in range(0, n, c)]
cv2.imwrite(dbg, np.vstack(rows))
