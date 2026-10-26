#include "CPUFrameTexture.h"

namespace NL::Render
{
    void CPUFrameTexture::Store(const RenderData* a_renderData, const void* a_buffer, int a_width, int a_height)
    {
        // CEF paints the view rect, which is the render data size
        if (a_renderData == nullptr ||
            a_buffer == nullptr ||
            a_width != static_cast<int>(a_renderData->width) ||
            a_height != static_cast<int>(a_renderData->height))
        {
            return;
        }

        const auto size = static_cast<std::size_t>(a_width) * a_height * 4;
        const auto bytes = static_cast<const std::uint8_t*>(a_buffer);

        std::lock_guard lock(m_frameMutex);
        m_frame.assign(bytes, bytes + size);
        m_frameDirty = true;
    }

    ID3D11ShaderResourceView* CPUFrameTexture::Upload(RenderData* a_renderData)
    {
        std::lock_guard lock(m_frameMutex);
        if (!m_frameDirty)
        {
            return m_srv.Get();
        }
        m_frameDirty = false;

        if (m_texture == nullptr)
        {
            D3D11_TEXTURE2D_DESC textDesc = {};
            textDesc.Width = a_renderData->width;
            textDesc.Height = a_renderData->height;
            textDesc.MipLevels = 1;
            textDesc.ArraySize = 1;
            textDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
            textDesc.SampleDesc.Count = 1;
            textDesc.Usage = D3D11_USAGE_DYNAMIC;
            textDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
            textDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

            auto hr = a_renderData->device->CreateTexture2D(&textDesc, nullptr, m_texture.ReleaseAndGetAddressOf());
            if (FAILED(hr))
            {
                spdlog::error("{}: failed CreateTexture2D(), code {:X}", NameOf(CPUFrameTexture), hr);
                m_texture = nullptr;
                return nullptr;
            }

            hr = a_renderData->device->CreateShaderResourceView(m_texture.Get(), nullptr, m_srv.ReleaseAndGetAddressOf());
            if (FAILED(hr))
            {
                spdlog::error("{}: failed CreateShaderResourceView(), code {:X}", NameOf(CPUFrameTexture), hr);
                m_texture = nullptr;
                return nullptr;
            }

            spdlog::info("{}: CPU frame texture created ({}x{})", NameOf(CPUFrameTexture), textDesc.Width, textDesc.Height);
        }

        D3D11_MAPPED_SUBRESOURCE mapped = {};
        const auto hr = a_renderData->deviceContext->Map(m_texture.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        if (FAILED(hr))
        {
            spdlog::error("{}: failed Map(), code {:X}", NameOf(CPUFrameTexture), hr);
            return m_srv.Get();
        }

        const auto rowSize = static_cast<std::size_t>(a_renderData->width) * 4;
        for (std::uint32_t y = 0; y < a_renderData->height; ++y)
        {
            std::memcpy(static_cast<std::uint8_t*>(mapped.pData) + y * mapped.RowPitch, m_frame.data() + y * rowSize, rowSize);
        }
        a_renderData->deviceContext->Unmap(m_texture.Get(), 0);

        return m_srv.Get();
    }
}
