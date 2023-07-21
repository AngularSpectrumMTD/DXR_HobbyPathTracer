#include "DxrPhotonMapper.h"

void DxrPhotonMapper::Denoise()
{
    auto frameIndex = mDevice->GetCurrentFrameIndex();

    std::vector<CD3DX12_RESOURCE_BARRIER> uavBarriers;
    uavBarriers.emplace_back(CD3DX12_RESOURCE_BARRIER::UAV(mDXRMainOutput.Get()));
    uavBarriers.emplace_back(CD3DX12_RESOURCE_BARRIER::UAV(mDenoisedColorBuffer.Get()));
    mCommandList->ResourceBarrier(u32(uavBarriers.size()), uavBarriers.data());

    std::vector<CD3DX12_RESOURCE_BARRIER> uavBarriers2;
    uavBarriers2.emplace_back(CD3DX12_RESOURCE_BARRIER::UAV(mPositionBuffer.Get()));
    uavBarriers2.emplace_back(CD3DX12_RESOURCE_BARRIER::UAV(mNormalBuffer.Get()));
    mCommandList->ResourceBarrier(u32(uavBarriers2.size()), uavBarriers2.data());

    for (s32 i = 0; i < DENOISE_ITE; i++)
    {
        DenoiseCB cb;
        cb.stepScale = 1 << i;
        cb.sigmaScale = (f32)pow(2.0f, i);
        cb.clrSigma = 1.f / DENOISE_ITE;
        cb.nmlSigma = 0.1f / DENOISE_ITE;
        cb.posSigma = 0.5f / DENOISE_ITE;

        auto denoiseCB = mDenoiseCBTbl.at(i).Get();
        mDevice->ImmediateBufferUpdateHostVisible(denoiseCB, &cb, sizeof(cb));

        mCommandList->SetComputeRootSignature(mRsDenoise.Get());
        mCommandList->SetComputeRootConstantBufferView(0, denoiseCB->GetGPUVirtualAddress());
        mCommandList->SetComputeRootDescriptorTable((i % 2 == 0) ? 1 : 2, mMainOutputDescriptorUAV.hGpu);
        mCommandList->SetComputeRootDescriptorTable((i % 2 == 0) ? 2 : 1, mDenoisedColorBufferDescriptorUAV.hGpu);
        mCommandList->SetComputeRootDescriptorTable(3, mPositionBufferDescriptorUAV.hGpu);
        mCommandList->SetComputeRootDescriptorTable(4, mNormalBufferDescriptorUAV.hGpu);
        mCommandList->SetPipelineState(mDenoisePSO.Get());
        mCommandList->Dispatch(GetWidth() / 16, GetHeight() / 16, 1);

        mCommandList->ResourceBarrier(u32(uavBarriers.size()), uavBarriers.data());
        mCommandList->ResourceBarrier(u32(uavBarriers2.size()), uavBarriers2.data());
    }

    if (DENOISE_ITE % 2 == 0)
    {
        D3D12_RESOURCE_BARRIER barriers[] = {
        CD3DX12_RESOURCE_BARRIER::Transition(mDXRMainOutput.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_DEST),
        CD3DX12_RESOURCE_BARRIER::Transition(mDenoisedColorBuffer.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE),
        };

        mCommandList->ResourceBarrier(_countof(barriers), barriers);
        mCommandList->CopyResource(mDXRMainOutput.Get(), mDenoisedColorBuffer.Get());

        CD3DX12_RESOURCE_BARRIER barrier[] = {
        CD3DX12_RESOURCE_BARRIER::Transition(mDXRMainOutput.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
        CD3DX12_RESOURCE_BARRIER::Transition(mDenoisedColorBuffer.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
        };
        mCommandList->ResourceBarrier(_countof(barrier), barrier);
    }
}
