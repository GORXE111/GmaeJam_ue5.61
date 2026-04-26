"""
=========================================================
一键创建里世界资源
=========================================================
- MPC_RealmReveal       : 材质参数集合
- M_Realm_Master        : 里世界母材质（圈内显示，圈外裁剪）
- M_Surface_Master      : 表世界母材质（圈外显示，圈内裁剪）

用法：UE 编辑器 → 工具 → 执行 Python 脚本 → 选本文件
跑完后基于母材质创建 材质实例（Material Instance）调色就行。
=========================================================
"""

import unreal

PACKAGE_PATH      = "/Game/Realm"
MPC_NAME          = "MPC_RealmReveal"
REALM_MAT_NAME    = "M_Realm_Master"
SURFACE_MAT_NAME  = "M_Surface_Master"
OVERLAY_MAT_NAME  = "M_RealmSphereOverlay"


# ---------- 工具函数 ----------

def ensure_dir(path):
    if not unreal.EditorAssetLibrary.does_directory_exist(path):
        unreal.EditorAssetLibrary.make_directory(path)


def delete_if_exists(full_path):
    if unreal.EditorAssetLibrary.does_asset_exist(full_path):
        unreal.EditorAssetLibrary.delete_asset(full_path)


# ---------- MPC ----------

def create_mpc():
    full_path = f"{PACKAGE_PATH}/{MPC_NAME}"
    delete_if_exists(full_path)

    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    factory = unreal.MaterialParameterCollectionFactoryNew()
    mpc = asset_tools.create_asset(MPC_NAME, PACKAGE_PATH, unreal.MaterialParameterCollection, factory)

    # Vector: RevealCenter
    vector_params = mpc.get_editor_property("vector_parameters")
    vp = unreal.CollectionVectorParameter()
    vp.set_editor_property("parameter_name", "RevealCenter")
    vp.set_editor_property("default_value", unreal.LinearColor(0, 0, 0, 0))
    vector_params.append(vp)
    mpc.set_editor_property("vector_parameters", vector_params)

    # Scalar
    scalar_params = mpc.get_editor_property("scalar_parameters")
    for name, val in [("RevealRadius", 500.0), ("EdgeSoftness", 50.0), ("RevealEnabled", 1.0)]:
        sp = unreal.CollectionScalarParameter()
        sp.set_editor_property("parameter_name", name)
        sp.set_editor_property("default_value", val)
        scalar_params.append(sp)
    mpc.set_editor_property("scalar_parameters", scalar_params)

    # 关键：Python 直接 set_editor_property 不会触发 PostEditChangeProperty，
    # UniformBufferStruct 不会构建，材质 shader 会报 "MaterialCollection0 undeclared"。
    # 通过 C++ helper 强制重建。
    unreal.RealmEditorHelper.rebuild_mpc(mpc)

    unreal.EditorAssetLibrary.save_loaded_asset(mpc)
    unreal.log(f"[Realm] 已创建 {full_path}")
    return mpc


# ---------- 母材质 ----------

def create_master_material(mat_name, mpc, invert_mask):
    """创建一个母材质
    invert_mask=False -> 里世界（Mask 直连 Opacity Mask）
    invert_mask=True  -> 表世界（1-Mask 接 Opacity Mask）
    """
    full_path = f"{PACKAGE_PATH}/{mat_name}"
    delete_if_exists(full_path)

    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    factory = unreal.MaterialFactoryNew()
    mat = asset_tools.create_asset(mat_name, PACKAGE_PATH, unreal.Material, factory)

    # 必须 Masked，否则 Opacity Mask 引脚不可用
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)
    mat.set_editor_property("two_sided", False)

    lib = unreal.MaterialEditingLibrary

    # ---- 节点：BaseColor 参数（Vector）
    base_color = lib.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -400, -200)
    base_color.set_editor_property("parameter_name", "BaseColor")
    default_color = unreal.LinearColor(0.5, 0.0, 0.8, 1.0) if not invert_mask else unreal.LinearColor(0.7, 0.7, 0.7, 1.0)
    base_color.set_editor_property("default_value", default_color)

    # ---- 节点：Roughness 参数（Scalar）
    roughness = lib.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -400, -50)
    roughness.set_editor_property("parameter_name", "Roughness")
    roughness.set_editor_property("default_value", 0.7)

    # ---- 节点：Metallic 参数（Scalar）
    metallic = lib.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -400, 50)
    metallic.set_editor_property("parameter_name", "Metallic")
    metallic.set_editor_property("default_value", 0.0)

    # ---- 节点：自定义 Realm Reveal Mask
    reveal = lib.create_material_expression(mat, unreal.MaterialExpressionRealmRevealMask, -400, 200)
    reveal.set_editor_property("collection", mpc)

    # ---- 占位节点：标准 Collection Parameter
    # 引擎只认 UMaterialExpressionCollectionParameter 的实例来注册 MPC uniform buffer，
    # 我们的自定义子类不被 Cast 识别，所以放一个标准节点做"挂名引用"。
    # 它的输出不连任何引脚，只是让材质把 MPC 加进 ReferencedParameterCollections。
    mpc_ref = lib.create_material_expression(mat, unreal.MaterialExpressionCollectionParameter, -400, 400)
    mpc_ref.set_editor_property("collection", mpc)
    mpc_ref.set_editor_property("parameter_name", "RevealRadius")

    # 连主节点
    lib.connect_material_property(base_color, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)
    lib.connect_material_property(roughness,  "",    unreal.MaterialProperty.MP_ROUGHNESS)
    lib.connect_material_property(metallic,   "",    unreal.MaterialProperty.MP_METALLIC)

    # 母材质只做"是否裁剪"。切面发光由额外的球壳叠加材质 M_RealmSphereOverlay 负责。
    if invert_mask:
        # 1 - Mask
        one_minus = lib.create_material_expression(mat, unreal.MaterialExpressionOneMinus, -200, 200)
        lib.connect_material_expressions(reveal, "", one_minus, "")
        lib.connect_material_property(one_minus, "", unreal.MaterialProperty.MP_OPACITY_MASK)
    else:
        lib.connect_material_property(reveal, "", unreal.MaterialProperty.MP_OPACITY_MASK)

    lib.recompile_material(mat)
    unreal.EditorAssetLibrary.save_loaded_asset(mat)
    unreal.log(f"[Realm] 已创建 {full_path}")
    return mat


# ---------- 球壳叠加材质 ----------

def create_sphere_overlay_material():
    """玩家身上挂的那个球。半透明、双面、Unlit。
    与场景几何相交的像素发光（intersection highlight），其余部分用 Fresnel 做弱发光。"""
    full_path = f"{PACKAGE_PATH}/{OVERLAY_MAT_NAME}"
    delete_if_exists(full_path)

    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    factory = unreal.MaterialFactoryNew()
    mat = asset_tools.create_asset(OVERLAY_MAT_NAME, PACKAGE_PATH, unreal.Material, factory)

    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    mat.set_editor_property("two_sided", True)
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)

    lib = unreal.MaterialEditingLibrary

    # ---- 参数
    glow_color = lib.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -800, -250)
    glow_color.set_editor_property("parameter_name", "GlowColor")
    glow_color.set_editor_property("default_value", unreal.LinearColor(0.4, 0.8, 1.0, 1.0))

    width = lib.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -800, -100)
    width.set_editor_property("parameter_name", "IntersectionWidth")
    width.set_editor_property("default_value", 20.0)

    intersect_str = lib.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -800, 0)
    intersect_str.set_editor_property("parameter_name", "IntersectionIntensity")
    intersect_str.set_editor_property("default_value", 8.0)

    fresnel_str = lib.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -800, 100)
    fresnel_str.set_editor_property("parameter_name", "FresnelIntensity")
    fresnel_str.set_editor_property("default_value", 1.5)

    base_op = lib.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -800, 200)
    base_op.set_editor_property("parameter_name", "BaseOpacity")
    base_op.set_editor_property("default_value", 0.08)

    # ---- 相交检测：Intersection = 1 - saturate( |SceneDepth - PixelDepth| / Width )
    scene_depth = lib.create_material_expression(mat, unreal.MaterialExpressionSceneDepth, -600, -50)
    pixel_depth = lib.create_material_expression(mat, unreal.MaterialExpressionPixelDepth, -600, 50)

    depth_sub = lib.create_material_expression(mat, unreal.MaterialExpressionSubtract, -400, 0)
    lib.connect_material_expressions(scene_depth, "", depth_sub, "A")
    lib.connect_material_expressions(pixel_depth, "", depth_sub, "B")

    depth_abs = lib.create_material_expression(mat, unreal.MaterialExpressionAbs, -250, 0)
    lib.connect_material_expressions(depth_sub, "", depth_abs, "")

    depth_div = lib.create_material_expression(mat, unreal.MaterialExpressionDivide, -100, 0)
    lib.connect_material_expressions(depth_abs, "", depth_div, "A")
    lib.connect_material_expressions(width,     "", depth_div, "B")

    depth_sat = lib.create_material_expression(mat, unreal.MaterialExpressionSaturate, 50, 0)
    lib.connect_material_expressions(depth_div, "", depth_sat, "")

    intersect_mask = lib.create_material_expression(mat, unreal.MaterialExpressionOneMinus, 200, 0)
    lib.connect_material_expressions(depth_sat, "", intersect_mask, "")

    intersect_emis = lib.create_material_expression(mat, unreal.MaterialExpressionMultiply, 350, 50)
    lib.connect_material_expressions(intersect_mask, "", intersect_emis, "A")
    lib.connect_material_expressions(intersect_str, "", intersect_emis, "B")

    # ---- Fresnel
    fresnel = lib.create_material_expression(mat, unreal.MaterialExpressionFresnel, -100, 250)
    fresnel_emis = lib.create_material_expression(mat, unreal.MaterialExpressionMultiply, 350, 250)
    lib.connect_material_expressions(fresnel,     "", fresnel_emis, "A")
    lib.connect_material_expressions(fresnel_str, "", fresnel_emis, "B")

    # ---- Emissive = (Intersection + Fresnel) * GlowColor
    emis_sum = lib.create_material_expression(mat, unreal.MaterialExpressionAdd, 500, 150)
    lib.connect_material_expressions(intersect_emis, "", emis_sum, "A")
    lib.connect_material_expressions(fresnel_emis,   "", emis_sum, "B")

    emis_final = lib.create_material_expression(mat, unreal.MaterialExpressionMultiply, 700, -50)
    lib.connect_material_expressions(glow_color, "RGB", emis_final, "A")
    lib.connect_material_expressions(emis_sum,   "",    emis_final, "B")
    lib.connect_material_property(emis_final, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

    # ---- Opacity = saturate(BaseOpacity + Intersection)
    op_add = lib.create_material_expression(mat, unreal.MaterialExpressionAdd, 500, 400)
    lib.connect_material_expressions(base_op,        "", op_add, "A")
    lib.connect_material_expressions(intersect_mask, "", op_add, "B")
    op_sat = lib.create_material_expression(mat, unreal.MaterialExpressionSaturate, 700, 400)
    lib.connect_material_expressions(op_add, "", op_sat, "")
    lib.connect_material_property(op_sat, "", unreal.MaterialProperty.MP_OPACITY)

    lib.recompile_material(mat)
    unreal.EditorAssetLibrary.save_loaded_asset(mat)
    unreal.log(f"[Realm] 已创建 {full_path}")
    return mat


# ---------- 入口 ----------

def run():
    ensure_dir(PACKAGE_PATH)

    mpc = create_mpc()
    create_master_material(REALM_MAT_NAME,   mpc, invert_mask=False)
    create_master_material(SURFACE_MAT_NAME, mpc, invert_mask=True)
    create_sphere_overlay_material()

    unreal.log("[Realm] 全部完成。下一步：基于母材质创建材质实例（右键 → 创建材质实例）。")


run()
