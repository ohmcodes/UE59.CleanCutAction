# Video to animation (Rookie emotes)

Turns a video of one person into an AnimSequence on the Rookie skeleton. Made for `A_Emote_Rookie` (the G-key emote).
No FBX and no IK retarget: bone rotations are solved against the Rookie ref pose and keyed directly.

## Setup (once)

MediaPipe has no build for the system Python 3.15, so use the engine's Python 3.11 for a venv:

```
"F:\UE5 Engines\UE_5.9\Engine\Binaries\ThirdParty\Python3\Win64\python.exe" -m venv C:\work\venv
C:\work\venv\Scripts\python.exe -m pip install mediapipe opencv-python numpy
```

Download the pose model (30 MB) from Google:
`https://storage.googleapis.com/mediapipe-models/pose_landmarker/pose_landmarker_heavy/float16/latest/pose_landmarker_heavy.task`

## Steps

1. In the editor's Python: `ue_emote.dump_skeleton(r'C:/work/rookie_skel.json')`
2. Track: `venv\Scripts\python.exe track.py <video> <model.task> pose.json track_dbg.jpg`
   Check `track_dbg.jpg`: green dots should sit on the body.
3. Solve: `venv\Scripts\python.exe solve.py pose.json rookie_skel.json emote_tracks.json solve_dbg.jpg <end_frame>`
   `end_frame` trims the clip. Check `solve_dbg.jpg`: stick figures (orange = left side) should match the video seen from the front.
4. In the editor's Python: `ue_emote.write_anim(r'C:/work/emote_tracks.json', '/Game/.../A_Emote_Rookie')`

## What the video needs

One person, whole body in frame, fixed camera, roughly front-on, feet mostly in place.

## Limits

- Fingers, hands and twist bones are not animated; limb twist is approximate.
- The ankle midpoint is pinned and the lowest foot is put on the floor, so stepping or travelling is lost.
- The last 12 frames ease back to the first pose.
- `track.py` crops black side bars using frame 100, so the clip must be longer than that (or edit the script).
- Bone names and hierarchy in `solve.py` are the Manny names the Rookie uses.
