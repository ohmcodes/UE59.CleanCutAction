"""Editor side of the video-to-animation pipeline. Run inside the Unreal Editor's Python (not the venv).

    import ue_emote
    ue_emote.dump_skeleton(r'C:/work/rookie_skel.json')                     # step 1, before solve.py
    ue_emote.write_anim(r'C:/work/emote_tracks.json', '/Game/_CleanCutAction/Character/Rookie/Anims/A_Emote_Rookie')  # step 4

write_anim overwrites the bone tracks of an existing AnimSequence on the Rookie skeleton
(duplicate any *_Rookie sequence to make a new one first).
"""
import json
import unreal

ROOKIE_MESH = '/Game/_CleanCutAction/Character/Rookie/SKM_Rookie'


def dump_skeleton(out_path, mesh_path=ROOKIE_MESH):
    """Writes every bone's local ('l') and component-space ('w') ref transform to JSON for solve.py."""
    ref = unreal.load_asset(mesh_path).skeleton.get_reference_pose()
    bones = []
    for name in ref.get_bone_names():
        entry = {'n': str(name)}
        for key, space in (('l', unreal.AnimPoseSpaces.LOCAL), ('w', unreal.AnimPoseSpaces.WORLD)):
            t = ref.get_ref_bone_pose(name, space)
            q, l, s = t.rotation, t.translation, t.scale3d
            entry[key] = {'t': [l.x, l.y, l.z], 'q': [q.x, q.y, q.z, q.w], 's': [s.x, s.y, s.z]}
        bones.append(entry)
    with open(out_path, 'w') as f:
        json.dump(bones, f)
    return len(bones)


def write_anim(tracks_path, anim_path, fps=30):
    """Replaces the bone tracks of the AnimSequence at anim_path with the tracks from solve.py, then saves it."""
    with open(tracks_path) as f:
        data = json.load(f)
    seq = unreal.load_asset(anim_path)
    c = seq.controller
    c.open_bracket(unreal.Text('Emote from video'), False)
    c.remove_all_bone_tracks(False)
    c.set_frame_rate(unreal.FrameRate(fps, 1), False)
    c.set_number_of_frames(unreal.FrameNumber(data['frames'] - 1), False)
    failed = []
    for name, keys in data['bones'].items():
        added = c.add_bone_curve(name, False)
        pos = [unreal.Vector(k[0], k[1], k[2]) for k in keys]
        rot = [unreal.Quat(k[3], k[4], k[5], k[6]) for k in keys]
        scl = [unreal.Vector(1, 1, 1)] * len(keys)
        if not (added and c.set_bone_track_keys(name, pos, rot, scl, False)):
            failed.append(name)
    c.close_bracket(False)
    seq.set_editor_property('enable_root_motion', False)
    unreal.EditorAssetLibrary.save_asset(seq.get_path_name())
    return {'length': seq.get_play_length(), 'failed': failed}
