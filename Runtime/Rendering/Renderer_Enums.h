/*
Copyright(c) 2016-2022 Panos Karabelas

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
copies of the Software, and to permit persons to whom the Software is furnished
to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

namespace Spartan
{
    // Constant buffer bindings
    enum class RendererBindings_Cb
    {
        frame    = 0,
        uber     = 1,
        light    = 2,
        material = 3
    };

    // SRV bindings
    enum class RendererBindings_Srv
    {
        // Material
        material_albedo    = 0,
        material_roughness = 1,
        material_metallic  = 2,
        material_normal    = 3,
        material_height    = 4,
        material_occlusion = 5,
        material_emission  = 6,
        material_mask      = 7,

        // G-buffer
        gbuffer_albedo   = 8,
        gbuffer_normal   = 9,
        gbuffer_material = 10,
        gbuffer_velocity = 11,
        gbuffer_depth    = 12,

        // Lighting
        light_diffuse              = 13,
        light_diffuse_transparent  = 14,
        light_specular             = 15,
        light_specular_transparent = 16,
        light_volumetric           = 17,

        // Light depth/color maps
        light_directional_depth = 18,
        light_directional_color = 19,
        light_point_depth       = 20,
        light_point_color       = 21,
        light_spot_depth        = 22,
        light_spot_color        = 23,

        // Noise
        noise_normal = 24,
        noise_blue   = 25,

        // Misc
        lutIbl            = 26,
        environment       = 27,
        ssao              = 28,
        ssao_gi           = 29,
        ssr               = 30,
        frame             = 31,
        tex               = 32,
        tex2              = 33,
        font_atlas        = 34,
        reflection_probe  = 35
    };

    // UAV Bindings
    enum class RendererBindings_Uav
    {
        r         = 0,
        rg        = 1,
        rgb       = 2,
        rgb2      = 3,
        rgb3      = 4,
        rgba      = 5,
        rgba2     = 6,
        rgba_mips = 7
    };

    // Structured buffer bindings
    enum class RendererBindings_Sb
    {
        counter = 19
    };

    // Shaders
    enum class RendererShader : uint8_t
    {
        Gbuffer_V,
        Gbuffer_P,
        Depth_Prepass_V,
        Depth_Prepass_P,
        Depth_Light_V,
        Depth_Light_P,
        Quad_V,
        Copy_Point_C,
        Copy_Bilinear_C,
        Copy_Point_P,
        Copy_Bilinear_P,
        Fxaa_C,
        FilmGrain_C,
        Taa_C,
        MotionBlur_C,
        Dof_DownsampleCoc_C,
        Dof_Bokeh_C,
        Dof_Tent_C,
        Dof_UpscaleBlend_C,
        ChromaticAberration_C,
        BloomLuminance_C,
        BloomDownsample_C,
        BloomBlendFrame_C,
        BloomUpsampleBlendMip_C,
        ToneMapping_C,
        Debanding_C,
        Debug_Texture_C,
        Debug_ReflectionProbe_V,
        Debug_ReflectionProbe_P,
        BrdfSpecularLut_C,
        Light_C,
        Light_Composition_C,
        Light_ImageBased_P,
        Color_V,
        Color_P,
        Font_V,
        Font_P,
        Ssao_C,
        Ssr_C,
        Entity_V,
        Entity_Transform_P,
        BlurGaussian_C,
        BlurGaussianBilateral_C,
        Entity_Outline_P,
        Reflection_Probe_V,
        Reflection_Probe_P,
        AMD_FidelityFX_CAS_C,
        AMD_FidelityFX_SPD_C,
        AMD_FidelityFX_SPD_LuminanceAntiflicker_C,
        AMD_FidelityFX_FSR_Upsample_C,
        AMD_FidelityFX_FSR_Sharpen_C
    };

    // Render targets
    enum class RendererRt : uint8_t
    {
        Undefined,
        Gbuffer_Albedo,
        Gbuffer_Normal,
        Gbuffer_Material,
        Gbuffer_Velocity,
        Gbuffer_Depth,
        Brdf_Specular_Lut,
        Light_Diffuse,
        Light_Diffuse_Transparent,
        Light_Specular,
        Light_Specular_Transparent,
        Light_Volumetric,
        Frame_Render,
        Frame_Render_2,
        Frame_Output,
        Frame_Output_2,
        Dof_Half,
        Dof_Half_2,
        Ssao,
        Ssao_Gi,
        Ssr,
        Taa_History,
        Bloom,
        Blur
    };

    // Renderer/graphics options
    enum Renderer_Option : uint64_t
    {
        Debug_Aabb                                           = 1 << 0,
        Debug_PickingRay                                     = 1 << 1,
        Debug_Grid                                           = 1 << 2,
        Debug_ReflectionProbes                               = 1 << 3,
        Transform_Handle                                     = 1 << 4,
        Debug_SelectionOutline                               = 1 << 5,
        Debug_Lights                                         = 1 << 6,
        Debug_PerformanceMetrics                             = 1 << 7,
        Debug_Physics                                        = 1 << 8,
        Debug_Wireframe                                      = 1 << 9,
        Bloom                                                = 1 << 10,
        VolumetricFog                                        = 1 << 11,
        AntiAliasing_Taa                                     = 1 << 12,
        AntiAliasing_Fxaa                                    = 1 << 13,
        Ssao                                                 = 1 << 14,
        ScreenSpaceShadows                                   = 1 << 15,
        ScreenSpaceReflections                               = 1 << 16,
        MotionBlur                                           = 1 << 17,
        DepthOfField                                         = 1 << 18,
        FilmGrain                                            = 1 << 19,
        Sharpening_AMD_FidelityFX_ContrastAdaptiveSharpening = 1 << 20,
        ChromaticAberration                                  = 1 << 21,
        Debanding                                            = 1 << 22,
        ReverseZ                                             = 1 << 23,
        DepthPrepass                                         = 1 << 24,
        Upsample_TAA                                         = 1 << 25,
        Upsample_AMD_FidelityFX_SuperResolution              = 1 << 26
    };

    // Renderer/graphics options values
    enum class Renderer_Option_Value
    {
        Anisotropy,
        ShadowResolution,
        Tonemapping,
        Gamma,
        Bloom_Intensity,
        Sharpen_Strength,
        Fog,
        Ssao_Gi
    };

    // Tonemapping
    enum Renderer_ToneMapping_Type
    {
        Renderer_ToneMapping_Off,
        Renderer_ToneMapping_ACES,
        Renderer_ToneMapping_Reinhard,
        Renderer_ToneMapping_Uncharted2
    };

    // Renderable object types
    enum class Renderer_ObjectType
    {
        GeometryOpaque,
        GeometryTransparent,
        Light,
        Camera,
        ReflectionProbe,
        Environment
    };
}
