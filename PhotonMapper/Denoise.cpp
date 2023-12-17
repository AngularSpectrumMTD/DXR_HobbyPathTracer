#include "DxrPhotonMapper.h"

void DxrPhotonMapper::SpatiotemporalVarianceGuidedFiltering()
{
    auto frameIndex = mDevice->GetCurrentFrameIndex();
    const u32 src = mRenderFrame % 2;
    const u32 dst = (mRenderFrame + 1) % 2;

    std::vector<CD3DX12_RESOURCE_BARRIER> uavBarriers;
    uavBarriers.emplace_back(CD3DX12_RESOURCE_BARRIER::UAV(mDXRMainOutput.Get()));
    uavBarriers.emplace_back(CD3DX12_RESOURCE_BARRIER::UAV(mDenoisedColorBuffer.Get()));
    mCommandList->ResourceBarrier(u32(uavBarriers.size()), uavBarriers.data());

    std::vector<CD3DX12_RESOURCE_BARRIER> uavBarriers2;
    uavBarriers2.emplace_back(CD3DX12_RESOURCE_BARRIER::UAV(mPositionBuffer.Get()));
    uavBarriers2.emplace_back(CD3DX12_RESOURCE_BARRIER::UAV(mNormalBuffer.Get()));
    mCommandList->ResourceBarrier(u32(uavBarriers2.size()), uavBarriers2.data());

    D3D12_RESOURCE_BARRIER uavToSrvForVariance[] = {
        CD3DX12_RESOURCE_BARRIER::Transition(mDepthBufferTbl[dst].Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
        CD3DX12_RESOURCE_BARRIER::Transition(mNormalBuffer.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
        CD3DX12_RESOURCE_BARRIER::Transition(mLuminanceMomentBufferTbl[dst].Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
    };

    mCommandList->ResourceBarrier(_countof(uavToSrvForVariance), uavToSrvForVariance);

    //variance
    {
        mCommandList->SetComputeRootSignature(mRsComputeVariance.Get());
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapComputeVariance["depthBuffer"], mDepthBufferDescriptorSRVTbl[dst].hGpu);
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapComputeVariance["normalBuffer"], mNormalBufferDescriptorSRV.hGpu);
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapComputeVariance["luminanceMomentBuffer"], mLuminanceMomentBufferDescriptorSRVTbl[dst].hGpu);
        mCommandList->SetComputeRootDescriptorTable(mRegisterMapComputeVariance["varianceBuffer"], mLuminanceVarianceBufferDescriptorUAVTbl[dst].hGpu);
        mCommandList->SetPipelineState(mComputeVariancePSO.Get());
        PIXBeginEvent(mCommandList.Get(), 0, "ComputeVariance");
        mCommandList->Dispatch(GetWidth() / 16, GetHeight() / 16, 1);
        PIXEndEvent(mCommandList.Get());
    }

    //wavelet
    {
        D3D12_RESOURCE_BARRIER initBarrier[] = {
            CD3DX12_RESOURCE_BARRIER::Transition(mDXRMainOutput.Get() ,D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
            CD3DX12_RESOURCE_BARRIER::Transition(mLuminanceVarianceBufferTbl[dst].Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
        };

        mCommandList->ResourceBarrier(_countof(initBarrier), initBarrier);

        for (s32 i = 0; i < DENOISE_ITE; i++)
        {
            DenoiseCB cb;
            cb.stepScale = 1 << i;

            auto denoiseCB = mDenoiseCBTbl.at(i).Get();
            mDevice->ImmediateBufferUpdateHostVisible(denoiseCB, &cb, sizeof(cb));

            mCommandList->SetComputeRootSignature(mRsA_TrousWaveletFilter.Get());
            mCommandList->SetComputeRootConstantBufferView(mRegisterMapA_TrousWaveletFilter["gWaveletParam"], denoiseCB->GetGPUVirtualAddress());
            mCommandList->SetComputeRootDescriptorTable(mRegisterMapA_TrousWaveletFilter["colorBufferSrc"], (i % 2 == 0) ? mMainOutputDescriptorSRV.hGpu : mDenoisedColorBufferDescriptorSRV.hGpu);
            mCommandList->SetComputeRootDescriptorTable(mRegisterMapA_TrousWaveletFilter["depthBuffer"], mDepthBufferDescriptorSRVTbl[dst].hGpu);
            mCommandList->SetComputeRootDescriptorTable(mRegisterMapA_TrousWaveletFilter["normalBuffer"], mNormalBufferDescriptorSRV.hGpu);
            mCommandList->SetComputeRootDescriptorTable(mRegisterMapA_TrousWaveletFilter["varianceBufferSrc"], mLuminanceVarianceBufferDescriptorSRVTbl[(i % 2 == 0) ? dst : src].hGpu);
            mCommandList->SetComputeRootDescriptorTable(mRegisterMapA_TrousWaveletFilter["colorBufferDst"], (i % 2 == 1) ? mMainOutputDescriptorUAV.hGpu : mDenoisedColorBufferDescriptorUAV.hGpu);
            mCommandList->SetComputeRootDescriptorTable(mRegisterMapA_TrousWaveletFilter["varianceBufferDst"], mLuminanceVarianceBufferDescriptorUAVTbl[(i % 2 == 0) ? src : dst].hGpu);
            mCommandList->SetPipelineState(mA_TrousWaveletFilterPSO.Get());
            PIXBeginEvent(mCommandList.Get(), 0, "A_TrousWaveletFiltering");
            mCommandList->Dispatch(GetWidth() / 16, GetHeight() / 16, 1);
            PIXEndEvent(mCommandList.Get());

            D3D12_RESOURCE_BARRIER wavelet_barrier[] = {
                CD3DX12_RESOURCE_BARRIER::Transition(mLuminanceVarianceBufferTbl[(i % 2 == 1) ? dst : src].Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
                CD3DX12_RESOURCE_BARRIER::Transition(mLuminanceVarianceBufferTbl[(i % 2 == 1) ? src : dst].Get(),D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
                CD3DX12_RESOURCE_BARRIER::Transition((i % 2 == 0) ? mDXRMainOutput.Get() : mDenoisedColorBuffer.Get(),D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
                CD3DX12_RESOURCE_BARRIER::Transition((i % 2 == 0) ? mDenoisedColorBuffer.Get() : mDXRMainOutput.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
            };

            mCommandList->ResourceBarrier(_countof(wavelet_barrier), wavelet_barrier);
        }

        D3D12_RESOURCE_BARRIER wavelet_barrier2[] = {
            CD3DX12_RESOURCE_BARRIER::Transition((DENOISE_ITE % 2 == 0) ? mDXRMainOutput.Get() : mDenoisedColorBuffer.Get(),D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
            CD3DX12_RESOURCE_BARRIER::Transition((DENOISE_ITE % 2 == 0) ? mLuminanceVarianceBufferTbl[dst].Get() : mLuminanceVarianceBufferTbl[src].Get(),D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
        };

        mCommandList->ResourceBarrier(_countof(wavelet_barrier2), wavelet_barrier2);
    }

    D3D12_RESOURCE_BARRIER srvToUav[] = {
        CD3DX12_RESOURCE_BARRIER::Transition(mDepthBufferTbl[dst].Get(),D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
        CD3DX12_RESOURCE_BARRIER::Transition(mNormalBuffer.Get(),D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
        CD3DX12_RESOURCE_BARRIER::Transition(mLuminanceMomentBufferTbl[dst].Get(),D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
    };

    mCommandList->ResourceBarrier(_countof(srvToUav), srvToUav);
}
