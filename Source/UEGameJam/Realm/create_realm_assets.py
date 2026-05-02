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
OVERLAY_MAT_NAME  = "M_RealmSphereOverlay"   # 旧版（带 Fresnel/接触环），保留不动
DOME_MAT_NAME     = "M_RealmDome"             # 新版穹面：陷入物体内部的整片球面发光


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

def create_dome_material():
    """玩家身上挂的那个球。半透明 + 双面 + Unlit + 禁用深度测试。

    视觉分两层叠加：
      1) 球壳本体：半透明发光（BaseOpacity 基础透明 + Fresnel 边缘亮）—— 沿用旧 Overlay 的质感
      2) 陷入物体的那部分球面：不透明 + 加强发光 —— 用 PixelDepth > SceneDepth 检测

    Two Sided 必须开：相机位于球内（球挂玩家身上），看到的是球的内表面，不开就剔光了。
    Disable Depth Test 必须开：陷入物体的球面像素本来会被深度遮挡，关掉测试才能画出来。

    注意：新母材质 M_RealmDome，与旧的 M_RealmSphereOverlay 共存、互不影响。"""
    full_path = f"{PACKAGE_PATH}/{DOME_MAT_NAME}"
    delete_if_exists(full_path)

    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    factory = unreal.MaterialFactoryNew()
    mat = asset_tools.create_asset(DOME_MAT_NAME, PACKAGE_PATH, unreal.Material, factory)

    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    mat.set_editor_property("two_sided", True)
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property("disable_depth_test", True)

    lib = unreal.MaterialEditingLibrary

    # ---- 参数
    glow_color = lib.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -1000, -300)
    glow_color.set_editor_property("parameter_name", "GlowColor")
    glow_color.set_editor_property("default_value", unreal.LinearColor(0.4, 0.8, 1.0, 1.0))

    base_op = lib.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -1000, -150)
    base_op.set_editor_property("parameter_name", "BaseOpacity")
    base_op.set_editor_property("default_value", 0.08)

    fresnel_int = lib.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -1000, -50)
    fresnel_int.set_editor_property("parameter_name", "FresnelIntensity")
    fresnel_int.set_editor_property("default_value", 1.5)

    fresnel_op_w = lib.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -1000, 50)
    fresnel_op_w.set_editor_property("parameter_name", "FresnelOpacityWeight")
    fresnel_op_w.set_editor_property("default_value", 0.3)

    soft_edge = lib.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -1000, 150)
    soft_edge.set_editor_property("parameter_name", "SoftEdge")
    soft_edge.set_editor_property("default_value", 5.0)  # cm

    inside_int = lib.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -1000, 250)
    inside_int.set_editor_property("parameter_name", "InsideIntensity")
    inside_int.set_editor_property("default_value", 6.0)

    # ---- Fresnel：球壳本体的边缘弱发光
    fresnel = lib.create_material_expression(mat, unreal.MaterialExpressionFresnel, -700, -100)

    # ---- 穹面遮罩：inside = saturate( (PixelDepth - SceneDepth) / SoftEdge )
    #   PixelDepth > SceneDepth → 球壳被物体挡住 → 陷入物体内部 → 显示为不透明
    #   PixelDepth ≤ SceneDepth → 球壳在物体前面（暴露空气）→ 退化为半透明球壳
    scene_depth = lib.create_material_expression(mat, unreal.MaterialExpressionSceneDepth, -700, 100)
    pixel_depth = lib.create_material_expression(mat, unreal.MaterialExpressionPixelDepth, -700, 200)

    depth_diff = lib.create_material_expression(mat, unreal.MaterialExpressionSubtract, -500, 150)
    lib.connect_material_expressions(pixel_depth, "", depth_diff, "A")
    lib.connect_material_expressions(scene_depth, "", depth_diff, "B")

    depth_norm = lib.create_material_expression(mat, unreal.MaterialExpressionDivide, -350, 150)
    lib.connect_material_expressions(depth_diff, "", depth_norm, "A")
    lib.connect_material_expressions(soft_edge,  "", depth_norm, "B")

    inside_mask = lib.create_material_expression(mat, unreal.MaterialExpressionSaturate, -200, 150)
    lib.connect_material_expressions(depth_norm, "", inside_mask, "")

    # ==================== Emissive ====================
    # emis_total = Fresnel * FresnelIntensity + inside * InsideIntensity
    fres_emis = lib.create_material_expression(mat, unreal.MaterialExpressionMultiply, -400, -75)
    lib.connect_material_expressions(fresnel,     "", fres_emis, "A")
    lib.connect_material_expressions(fresnel_int, "", fres_emis, "B")

    inside_emis = lib.create_material_expression(mat, unreal.MaterialExpressionMultiply, -400, 250)
    lib.connect_material_expressions(inside_mask, "", inside_emis, "A")
    lib.connect_material_expressions(inside_int,  "", inside_emis, "B")

    emis_sum = lib.create_material_expression(mat, unreal.MaterialExpressionAdd, -150, 75)
    lib.connect_material_expressions(fres_emis,   "", emis_sum, "A")
    lib.connect_material_expressions(inside_emis, "", emis_sum, "B")

    emis_final = lib.create_material_expression(mat, unreal.MaterialExpressionMultiply, 100, 0)
    lib.connect_material_expressions(glow_color, "RGB", emis_final, "A")
    lib.connect_material_expressions(emis_sum,   "",    emis_final, "B")
    lib.connect_material_property(emis_final, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

    # ==================== Opacity ====================
    # opacity = saturate( BaseOpacity + Fresnel * FresnelOpacityWeight + inside )
    fres_op = lib.create_material_expression(mat, unreal.MaterialExpressionMultiply, -400, 350)
    lib.connect_material_expressions(fresnel,      "", fres_op, "A")
    lib.connect_material_expressions(fresnel_op_w, "", fres_op, "B")

    op_sum1 = lib.create_material_expression(mat, unreal.MaterialExpressionAdd, -200, 350)
    lib.connect_material_expressions(base_op, "", op_sum1, "A")
    lib.connect_material_expressions(fres_op, "", op_sum1, "B")

    op_sum2 = lib.create_material_expression(mat, unreal.MaterialExpressionAdd, 0, 400)
    lib.connect_material_expressions(op_sum1,     "", op_sum2, "A")
    lib.connect_material_expressions(inside_mask, "", op_sum2, "B")

    op_sat = lib.create_material_expression(mat, unreal.MaterialExpressionSaturate, 200, 400)
    lib.connect_material_expressions(op_sum2, "", op_sat, "")
    lib.connect_material_property(op_sat, "", unreal.MaterialProperty.MP_OPACITY)

    lib.recompile_material(mat)
    unreal.EditorAssetLibrary.save_loaded_asset(mat)
    unreal.log(f"[Realm] 已创建 {full_path}")
    return mat


# ---------- 入口 ----------

def run():
    """默认行为：只创建/更新新版穹面母材质 M_RealmDome。
    其他既有资产（MPC_RealmReveal、M_Realm_Master、M_Surface_Master、M_RealmSphereOverlay
    以及基于它们的所有材质实例）一律不动，避免影响他人正在使用的引用。
    需要从零重建全部资产时，调用 run_full_setup()。"""
    ensure_dir(PACKAGE_PATH)
    create_dome_material()
    unreal.log(
        "[Realm] M_RealmDome 已创建/更新。下一步：基于它创建材质实例 "
        "（内容浏览器里右键 → 创建材质实例），再把材质实例拖到玩家球壳的 元素0 上。"
    )


def run_full_setup():
    """从零创建/覆盖全部里世界资产（MPC + 两个母材质 + 旧 Overlay + 新 Dome）。
    会覆盖已有的同名资源 —— 仅在确认没有材质实例还在使用旧母材质时才能跑。"""
    ensure_dir(PACKAGE_PATH)
    mpc = create_mpc()
    create_master_material(REALM_MAT_NAME,   mpc, invert_mask=False)
    create_master_material(SURFACE_MAT_NAME, mpc, invert_mask=True)
    create_dome_material()
    unreal.log("[Realm] 全量重建完成。")


run()
