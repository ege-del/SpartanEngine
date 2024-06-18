/*
Copyright(c) 2016-2024 Panos Karabelas

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

//= INCLUDES ====================
#include "pch.h"
#include "RHI_OpenImageDenoise.h"
#include "RHI_Device.h"
#include "RHI_Texture.h"
SP_WARNINGS_OFF
#include "oidin/oidin/oidn.h"
SP_WARNINGS_ON
//===============================

namespace Spartan
{
    namespace
    {
        OIDNDevice device = nullptr;
        OIDNFilter filter = nullptr;
    }

    void RHI_OpenImageDenoise::Initialize()
    {
        // create a device
        OIDNDeviceType device_type = RHI_Device::GetPrimaryPhysicalDevice()->IsNvidia() ? OIDN_DEVICE_TYPE_CUDA : OIDN_DEVICE_TYPE_HIP;
        OIDNDevice device = oidnNewDevice(device_type);
        oidnCommitDevice(device);

        // create a generic ray tracing filter
        filter = oidnNewFilter(device, "RT");

        // set denoiser parameters
        oidnSetFilterBool(filter, "hdr", true);
        oidnCommitFilter(filter);
    }

    void RHI_OpenImageDenoise::Shutdown()
    {
        oidnReleaseFilter(filter);
        oidnReleaseDevice(device);
    }

    void RHI_OpenImageDenoise::Denoise(RHI_Texture* texture)
    {
        // create OIDNBuffer from the input Vulkan image
        size_t size_bytes = texture->GetWidth() * texture->GetHeight() * texture->GetBytesPerPixel();
        OIDNBuffer buffer = oidnNewSharedBuffer(device, texture->GetRhiSrv(), size_bytes);

        // set the input and output textures to the same buffer
        oidnSetFilterImage(filter, "texture", buffer, OIDN_FORMAT_FLOAT4, texture->GetWidth(), texture->GetHeight(), 0, 0, 0);

        // execute the denoising filter
        oidnExecuteFilter(filter);

        // release the OIDNBuffer
        oidnReleaseBuffer(buffer);

        // check for errors
        const char* error_message;
        if (oidnGetDeviceError(device, &error_message) != OIDN_ERROR_NONE)
        {
            SP_LOG_ERROR("Open Image Denoise error: %s", error_message);
        }
    }
}
