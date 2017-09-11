/*
Copyright(c) 2016-2017 Panos Karabelas

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

//= INCLUDES ============================
#include <memory>
#include "../../Math/Matrix.h"
#include "../D3D11/D3D11GraphicsDevice.h"
//=======================================

namespace Directus
{
	class D3D11ConstantBuffer;
	class D3D11Shader;

	class DepthShader
	{
	public:
		DepthShader();
		~DepthShader();

		void Load(const std::string& filePath, Graphics* graphics);
		void Set();
		void SetBuffer(const Math::Matrix& mWorld, const Math::Matrix& mViewProjection);		
		void Render(unsigned int indexCount);

	private:
		struct DefaultBuffer
		{
			Math::Matrix worldViewProjection;
		};

		std::shared_ptr<D3D11ConstantBuffer> m_defaultBuffer;
		std::shared_ptr<D3D11Shader> m_shader;
		Graphics* m_graphics;
	};
}
