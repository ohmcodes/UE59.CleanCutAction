# Copyright (c) 2026 Ohmcodes. All Rights Reserved.
# Quest performance audit: lists every static mesh used in the Phase 1 cafe with its instance count, LOD0 triangles,
# LOD count and Nanite state, heaviest first. Output: Saved/QuestMeshAudit.csv (+ summary in the log).
# Run headless:
#   UnrealEditor-Cmd.exe CleanCutAction.uproject -run=pythonscript -script="<path>/quest_mesh_audit.py" -unattended -nop4 -nosplash -NullRHI
import unreal

LEVEL = '/Game/StylizedNewYorkCity/Level/L_NYC_Cafe_Michael'

world = unreal.EditorLoadingAndSavingUtils.load_map(LEVEL)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
actors = eas.get_all_level_actors()

stats = {}  # mesh path -> dict


def add(mesh, count, actor_label):
    if not mesh:
        return
    p = mesh.get_path_name()
    s = stats.get(p)
    if not s:
        tris, lods = 0, -1
        try:
            lods = mesh.get_num_lods()
            tris = mesh.get_num_triangles(0)
        except Exception as e:
            unreal.log_warning('QUESTAUDIT api %s' % e)
        nanite = False
        try:
            nanite = bool(mesh.get_editor_property('nanite_settings').get_editor_property('enabled'))
        except Exception:
            pass
        s = stats[p] = {'tris': tris, 'lods': lods, 'nanite': nanite, 'count': 0, 'example': actor_label}
    s['count'] += count


for a in actors:
    label = a.get_actor_label()
    for c in a.get_components_by_class(unreal.StaticMeshComponent):
        if not c.is_visible() if hasattr(c, 'is_visible') else False:
            continue
        mesh = c.get_editor_property('static_mesh')
        n = 1
        if isinstance(c, unreal.InstancedStaticMeshComponent):
            n = c.get_instance_count()
        add(mesh, n, label)

rows = sorted(stats.items(), key=lambda kv: kv[1]['tris'] * kv[1]['count'], reverse=True)
total = sum(s['tris'] * s['count'] for _, s in rows)
out = unreal.Paths.project_saved_dir() + 'QuestMeshAudit.csv'
with open(out, 'w', encoding='utf-8') as f:
    f.write('mesh,instances,lod0_tris,total_tris,lods,nanite,example_actor\n')
    for p, s in rows:
        f.write('%s,%d,%d,%d,%d,%s,%s\n' % (p, s['count'], s['tris'], s['tris'] * s['count'], s['lods'], s['nanite'], s['example']))
unreal.log('QUESTAUDIT actors=%d meshes=%d total_lod0_tris=%d csv=%s' % (len(actors), len(rows), total, out))
for p, s in rows[:40]:
    unreal.log('QUESTAUDIT %8d x%-4d = %9d lods=%d nanite=%s %s' % (s['tris'], s['count'], s['tris'] * s['count'], s['lods'], s['nanite'], p))
