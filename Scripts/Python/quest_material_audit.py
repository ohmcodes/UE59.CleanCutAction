# Copyright (c) 2026 Ohmcodes. All Rights Reserved.
# Quest overdraw audit: every material on the Phase 1 cafe's meshes, with blend mode, two-sided, how many mesh components
# use it and roughly how many triangles draw with it. Masked / translucent / depth-writing materials switch off the Quest
# GPU's early hidden-surface rejection, so a room full of them is shaded many times per pixel.
# Output: Saved/QuestMaterialAudit.csv and QUESTMAT lines in the log. Run headless:
#   UnrealEditor-Cmd.exe CleanCutAction.uproject -run=pythonscript -script="<path>/quest_material_audit.py" -unattended -nop4 -nosplash -NullRHI
import unreal

LEVEL = '/Game/StylizedNewYorkCity/Level/L_NYC_Cafe_Michael'
unreal.EditorLoadingAndSavingUtils.load_map(LEVEL)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

stats = {}
for actor in eas.get_all_level_actors():
    for comp in actor.get_components_by_class(unreal.StaticMeshComponent):
        mesh = comp.get_editor_property('static_mesh')
        if not mesh:
            continue
        count = comp.get_instance_count() if isinstance(comp, unreal.InstancedStaticMeshComponent) else 1
        try:
            tris = mesh.get_num_triangles(0)
        except Exception:
            tris = 0
        for i in range(comp.get_num_materials()):
            mat = comp.get_material(i)
            if not mat:
                continue
            base = mat.get_base_material() if hasattr(mat, 'get_base_material') else mat
            key = base.get_path_name()
            s = stats.setdefault(key, {'blend': None, 'two': None, 'comps': 0, 'tris': 0, 'mesh': mesh.get_name()})
            if s['blend'] is None:
                try:
                    s['blend'] = str(base.get_editor_property('blend_mode')).split('.')[-1].rstrip('>').split(':')[0]
                    s['two'] = bool(base.get_editor_property('two_sided'))
                except Exception:
                    s['blend'] = '?'
            s['comps'] += count
            s['tris'] += tris * count // max(1, comp.get_num_materials())

rows = sorted(stats.items(), key=lambda kv: kv[1]['tris'], reverse=True)
out = unreal.Paths.project_saved_dir() + 'QuestMaterialAudit.csv'
with open(out, 'w', encoding='utf-8') as f:
    f.write('base_material,blend,two_sided,components,approx_tris,example_mesh\n')
    for k, s in rows:
        f.write('%s,%s,%s,%d,%d,%s\n' % (k, s['blend'], s['two'], s['comps'], s['tris'], s['mesh']))
by_blend = {}
for k, s in rows:
    b = by_blend.setdefault(s['blend'], [0, 0])
    b[0] += s['comps']
    b[1] += s['tris']
unreal.log('QUESTMAT by blend mode: %s' % by_blend)
for k, s in rows[:40]:
    unreal.log('QUESTMAT %-12s two=%-5s comps=%-4d tris=%-8d %s (%s)' % (s['blend'], s['two'], s['comps'], s['tris'], k, s['mesh']))
