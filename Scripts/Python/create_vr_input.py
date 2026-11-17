# Clean! Cut! Action! - creates the VR input assets (Touch controller bindings for OpenXR).
#
# Run once with the editor closed:
#   UnrealEditor-Cmd.exe "<project>/CleanCutAction.uproject" -run=pythonscript -script="<project>/Scripts/Python/create_vr_input.py"
# or from an open editor: Tools > Execute Python Script.
#
# Makes /Game/_CleanCutAction/Input/VR/:
#   IA_VR_Turn (Axis1D)      right stick X -> snap turn (ULTTVRComponent)
#   IA_VR_UIClick (Bool)     right trigger -> clicks the world-space UI panel (works while paused)
#   IA_VR_Pause (Bool)       left menu button -> pause menu (works while paused)
#   IA_VR_Hint (Bool)        X (left controller) -> hint (ALTTMessDirector)
#   IMC_VR                   maps those plus the existing gameplay actions to the Touch controllers.
# IMC_VR is listed in Config/DefaultInput.ini as an Enhanced Input default mapping context; OpenXR only
# exposes bindings from those to the runtime.
# Safe to run again: existing assets are reused and IMC_VR's mappings are rebuilt.

import unreal

VR_DIR = "/Game/_CleanCutAction/Input/VR"
tools = unreal.AssetToolsHelpers.get_asset_tools()


def load_or_create(name, cls, factory):
    path = f"{VR_DIR}/{name}"
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        return unreal.load_asset(path)
    asset = tools.create_asset(name, VR_DIR, cls, factory)
    if not asset:
        raise RuntimeError(f"Could not create {path}")
    unreal.log(f"VR input: created {path}")
    return asset


def make_action(name, value_type, when_paused=False):
    action = load_or_create(name, unreal.InputAction, unreal.InputAction_Factory())
    action.set_editor_property("value_type", value_type)
    action.set_editor_property("trigger_when_paused", when_paused)
    return action


def key(name):
    k = unreal.Key()
    k.import_text(name)  # FKey has no Python constructor args; its text form is the key name
    return k


turn = make_action("IA_VR_Turn", unreal.InputActionValueType.AXIS1D)
ui_click = make_action("IA_VR_UIClick", unreal.InputActionValueType.BOOLEAN, when_paused=True)
pause = make_action("IA_VR_Pause", unreal.InputActionValueType.BOOLEAN, when_paused=True)
hint = make_action("IA_VR_Hint", unreal.InputActionValueType.BOOLEAN)

move = unreal.load_asset("/Game/Input/Actions/IA_Move")
jump = unreal.load_asset("/Game/Input/Actions/IA_Jump")
interact = unreal.load_asset("/Game/_CleanCutAction/Input/IA_Interact")
throw = unreal.load_asset("/Game/_CleanCutAction/Input/IA_Throw")
emote = unreal.load_asset("/Game/_CleanCutAction/Input/IA_Emote")

imc = load_or_create("IMC_VR", unreal.InputMappingContext, unreal.InputMappingContext_Factory())
imc.unmap_all()
# OpenXR rejects two contexts with the same localized description.
try:
    imc.set_editor_property("context_description", unreal.Text("VR Touch Controllers"))
except Exception as e:  # property name differs between versions; the FName fallback still works
    unreal.log_warning(f"VR input: could not set context description: {e}")

bindings = [
    (move, "OculusTouch_Left_Thumbstick_2D", True),
    (turn, "OculusTouch_Right_Thumbstick_X", False),
    (jump, "OculusTouch_Right_A_Click", False),
    (interact, "OculusTouch_Right_Trigger_Click", False),
    (interact, "OculusTouch_Left_Trigger_Click", False),
    (ui_click, "OculusTouch_Right_Trigger_Click", False),
    # No controller (keyboard and mouse with the XR Simulator): look at a button and click / press Enter.
    (ui_click, "LeftMouseButton", False),
    (ui_click, "Enter", False),
    (throw, "OculusTouch_Right_B_Click", False),
    (emote, "OculusTouch_Left_Y_Click", False),
    (pause, "OculusTouch_Left_Menu_Click", False),
    (hint, "OculusTouch_Left_X_Click", False),
]
dead_zone_keys = set()
for action, key_name, dead_zone in bindings:
    if not action:
        unreal.log_error(f"VR input: missing action for {key_name}, skipped")
        continue
    imc.map_key(action, key(key_name))
    if dead_zone:
        dead_zone_keys.add(key_name)

# Dead zone on the move stick so a drifting stick doesn't creep the player along.
try:
    data = imc.get_editor_property("default_key_mappings")
    mappings = list(data.get_editor_property("mappings"))
    for i, m in enumerate(mappings):
        if str(m.get_editor_property("key").get_editor_property("key_name")) in dead_zone_keys:
            dz = unreal.new_object(unreal.InputModifierDeadZone, outer=imc)
            dz.set_editor_property("lower_threshold", 0.2)
            m.set_editor_property("modifiers", [dz])
            mappings[i] = m
    data.set_editor_property("mappings", mappings)
    imc.set_editor_property("default_key_mappings", data)
except Exception as e:
    unreal.log_warning(f"VR input: dead zone not added ({e}); the stick still works")

for asset in (turn, ui_click, pause, hint, imc):
    if hasattr(unreal, "EditorAssetLibrary"):
        unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False)
    else:
        unreal.EditorLoadingAndSavingUtils.save_packages([asset.get_outermost()], only_dirty=False)

unreal.log(f"VR input: IMC_VR has {len(imc.get_editor_property('default_key_mappings').get_editor_property('mappings'))} mappings")
