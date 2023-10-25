/*
Copyright(c) 2016-2023 Panos Karabelas

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

// thse functions are shared between depth_prepass.hlsl and g_buffer.hlsl, this is because the calculations have to be exactly the same

struct vertex_simulation
{
    struct wind
    {
        static float hash(float n)
        {
            return frac(sin(n) * 43758.5453f);
        }

        static float perlin_noise(float x)
        {
            float i = floor(x);
            float f = frac(x);
            f       = f * f * (3.0 - 2.0 * f);

            return lerp(hash(i), hash(i + 1.0), f);
        }

        static float4 apply(uint instance_id, float4 position_vertex, float3 position_transform, float time)
        {
            static const float3 wind_direction          = float3(1, 0, 0);
            static const float  wind_vertex_sway_extent = 0.1f; // oscillation amplitude
            static const float  wind_vertex_sway_speed  = 4.0f; // oscillation frequency
            
            // base wave
            float phase_offset = float(instance_id) * PI_HALF;
            float base_wave1   = sin((time * wind_vertex_sway_speed) + position_vertex.x + phase_offset);
            float base_wave2   = sin((time * wind_vertex_sway_speed * 0.8f) + position_vertex.x + phase_offset + PI_HALF); // out of phase

            // perlin noise
            float noise_factor = perlin_noise(position_vertex.x * 0.1f + time) - 0.5f;

            // combine base waves with noise
            float combined_wave = (base_wave1 + base_wave2) * 0.5f + noise_factor;

            // don't sway at the bottom of the mesh, sway the most at the top
            float sway_factor = saturate((position_vertex.y - position_transform.y) / buffer_material.world_space_height);
            
            // calculate final offset
            float3 offset = wind_direction * (combined_wave + noise_factor) * wind_vertex_sway_extent * sway_factor;

            position_vertex.xyz += offset;
            return position_vertex;
        }
    };

    struct water_wave
    {
        static float4 apply(float4 position_vertex, float time)
        {
            static const float  base_wave_height    = 0.1f;
            static const float  base_wave_frequency = 20.0f;
            static const float  base_wave_speed     = 0.5f;
            static const float3 base_wave_direction = float3(1.0f, 0.0f, 0.0f);

            // interleave 4 waves to have a more complex wave pattern
            float3 offset = float3(0.0f, 0.0f, 0.0f);
            for (int i = 0; i < 4; i++)
            {
                // Modulate base wave parameters based on index
                float wave_height    = base_wave_height * (0.75f + i * 0.1f);
                float wave_frequency = base_wave_frequency * (0.9f + i * 0.05f);
                float wave_speed     = base_wave_speed * (0.9f + i * 0.05f);
    
                // dynamically calculate wave direction based on index
                float angle = 2.0f * 3.14159f * i / 4.0f;
                float2 wave_direction = float2(cos(angle), sin(angle));
    
                // gerstner wave equation
                float k = 2 * 3.14159 / wave_frequency;
                float w = sqrt(9.8f / k) * wave_speed;
    
                // phase and amplitude
                float phase = dot(wave_direction, position_vertex.xz) * k + time * w;
                float c     = cos(phase);
                float s     = sin(phase);
    
                // calculate new position for this wave and add to the offset
                offset.x += wave_height * wave_direction.x * c;
                offset.z += wave_height * wave_direction.y * c;
                offset.y += wave_height * s;
            }
    
            position_vertex.xz += offset.xz;
            position_vertex.y  += offset.y;
    
            return position_vertex;
        }
    };
};
