"""Run once in UE's PythonScript commandlet; never overwrites an existing map.

UnrealEditor-Cmd PROJECT -EnablePlugins=PythonScriptPlugin,EditorScriptingUtilities -run=pythonscript
    -script=ABSOLUTE_PATH_TO_THIS_FILE -unattended -nullrhi
"""
import unreal

material_path = "/Game/Demo/M_FlyDemo"
map_path = "/Game/Demo/FlyBrainDemo"
assets = unreal.EditorAssetLibrary
if not assets.does_asset_exist(material_path):
    material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "M_FlyDemo", "/Game/Demo", unreal.Material, unreal.MaterialFactoryNew())
    if material is None:
        raise RuntimeError("Could not create demo material")
    edit = unreal.MaterialEditingLibrary
    tint = edit.create_material_expression(material, unreal.MaterialExpressionVectorParameter, -420, 0)
    tint.set_editor_property("parameter_name", "Tint")
    tint.set_editor_property("default_value", unreal.LinearColor(0.1, 0.4, 0.3, 1.0))
    tint.set_editor_property("use_custom_primitive_data", True)
    tint.set_editor_property("primitive_data_index", 0)
    edit.connect_material_property(tint, "", unreal.MaterialProperty.MP_BASE_COLOR)
    roughness = edit.create_material_expression(material, unreal.MaterialExpressionConstant, -400, 200)
    roughness.set_editor_property("r", 0.6)
    edit.connect_material_property(roughness, "", unreal.MaterialProperty.MP_ROUGHNESS)
    emission = edit.create_material_expression(material, unreal.MaterialExpressionMultiply, -200, -150)
    emission.set_editor_property("const_b", 0.18)
    edit.connect_material_expressions(tint, "", emission, "A")
    edit.connect_material_property(emission, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    edit.recompile_material(material)
    if not assets.save_loaded_asset(material):
        raise RuntimeError("Could not save demo material")

if not assets.does_asset_exist(map_path):
    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if not levels.new_level(map_path):
        raise RuntimeError("Could not create demo map")
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    game_mode = unreal.load_class(None, "/Script/FlyBrainSimulator.FlyBrainDemoGameMode")
    if game_mode is None:
        raise RuntimeError("Build the FlyBrainSimulator module before creating the demo map")
    world.get_world_settings().set_editor_property("default_game_mode", game_mode)
    if not levels.save_current_level():
        raise RuntimeError("Could not save demo map")

unreal.log("FLY_DEMO_ASSETS_READY")
