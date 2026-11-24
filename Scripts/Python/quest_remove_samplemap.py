# Clean! Cut! Action! VR - Quest performance: remove the ParisStreet street (SampleMap level instance) from the
# Phase 1 cafe level. 189 actors / 184 meshes outside the windows; the cafe's sun, sky, fog and skylight are in the
# cafe level itself, so lighting is unchanged.
# Run headless (editor closed):
#   UnrealEditor-Cmd.exe "<project>.uproject" -run=pythonscript -script="<this file>" -unattended -nop4 -nosplash -NullRHI
import unreal

LEVEL = "/Game/StylizedNewYorkCity/Level/L_NYC_Cafe_Michael"

world = unreal.EditorLoadingAndSavingUtils.load_map(LEVEL)
if not world:
    raise RuntimeError("could not load " + LEVEL)

actors = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor)
targets = []
for a in actors:
    if a.get_class().get_name() == "LevelInstance":
        asset = str(a.get_editor_property("world_asset"))
        if "ParisStreet" in asset or a.get_actor_label() == "SampleMap":
            targets.append(a)

unreal.log("QUEST: found %d ParisStreet level instance(s)" % len(targets))
for a in targets:
    a.destroy_actor()

if targets:
    ok = unreal.EditorLoadingAndSavingUtils.save_map(world, LEVEL)
    unreal.log("QUEST: saved %s: %s" % (LEVEL, ok))

left = [a.get_actor_label() for a in unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Actor)
        if a.get_class().get_name() == "LevelInstance"]
unreal.log("QUEST: level instances left: %s" % left)
