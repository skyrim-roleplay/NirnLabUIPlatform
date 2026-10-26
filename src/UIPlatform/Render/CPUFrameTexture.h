#pragma once

#include "PCH.h"
#include "RenderData.h"
#include <mutex>

namespace NL::Render
{
    /// <summary>
    /// CEF frames painted on the CPU (OnPaint), used when shared textures are disabled (Wine/DXMT).
    /// Store() is called on a CEF thread, Upload() on the render thread that owns the immediate context.
    /// </summary>
    class CPUFrameTexture
    {
        std::vector<std::uint8_t> m_frame;
        std::mutex m_frameMutex;
        bool m_frameDirty = false;

        Microsoft::WRL::ComPtr<ID3D11Texture2D> m_texture;
        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_srv;

    public:
        void Store(const RenderData* a_renderData, const void* a_buffer, int a_width, int a_height);

        /// <summary>
        /// Uploads the last stored frame, returns the view to draw (nullptr until the first CPU frame)
        /// </summary>
        ID3D11ShaderResourceView* Upload(RenderData* a_renderData);
    };
}
