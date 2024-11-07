#include "Camera.h"
#include <cmath>

using namespace DirectX;

Camera::Camera() : mEye(), mTarget(), mUp(), mButtonType(-1)
{
    mMtxView = XMMatrixIdentity();
    mMtxProj = XMMatrixIdentity();
}

void Camera::SetLookAt(XMFLOAT3 vPos, XMFLOAT3 vTarget, XMFLOAT3 vUp)
{
    mEye = XMLoadFloat3(&vPos);
    mTarget = XMLoadFloat3(&vTarget);
    mUp = XMVector3Normalize(XMLoadFloat3(&vUp));
    mMtxView = XMMatrixLookAtRH(mEye, mTarget, mUp);
}

void Camera::SetPerspective(f32 fovY, f32 aspect, f32 znear, f32 zfar)
{
    mMtxProj = XMMatrixPerspectiveFovRH(fovY, aspect, znear, zfar);
}

void Camera::OnMouseButtonDown(s32 buttonType, f32, f32 )
{
    mButtonType = buttonType;
}

void Camera::OnMouseMove(f32 dx, f32 dy) 
{
    if (mButtonType < 0)
    {
        return;
    }
    if (mButtonType == 0)
    {
        EyeVecRotation(dx, dy);
    }
   /* if (mButtonType == 1)
    {
        ForwardBackward(dy);
    }*/
}
void Camera::OnMouseButtonUp()
{
    mButtonType = -1;
}

void Camera::OnMouseWheel(f32 rotate)
{
    SwingRightLeft(rotate);
}

bool Camera::OnKeyDown(UINT8  wparam)
{
    bool flag = false;
    XMVECTOR tmp = mEye - mTarget;
    const f32 ratio = 1.0f;
    switch (wparam)
    {
        //Foward
    case VK_UP:
        mEye -= ratio * XMVector3Normalize(tmp);
        mTarget -= ratio * XMVector3Normalize(tmp);
        flag =  true;
        break;
        //Backward
    case VK_DOWN:
        mEye += ratio * XMVector3Normalize(tmp);
        mTarget += ratio * XMVector3Normalize(tmp);
        flag = true;
        break;
        //Right
    case VK_RIGHT:
        mEye -= ratio * XMVector3Normalize(XMVector3Cross(tmp, mUp));
        mTarget -= ratio * XMVector3Normalize(XMVector3Cross(tmp, mUp));
        flag = true;
        break;
        //Left
    case VK_LEFT:
        mEye += ratio * XMVector3Normalize(XMVector3Cross(tmp, mUp));
        mTarget += ratio * XMVector3Normalize(XMVector3Cross(tmp, mUp));
        flag = true;
        break;
    }
    mMtxView = XMMatrixLookAtRH(mEye, mTarget, mUp);
    return flag;
}

void Camera::EyeVecRotation(f32 dx, f32 dy)
{
    auto ToEye = mEye - mTarget;
    auto L = XMVectorGetX(XMVector3Length(ToEye));
    ToEye = XMVector3Normalize(ToEye);

    auto phi = std::atan2(XMVectorGetX(ToEye), XMVectorGetZ(ToEye));//Azimuth
    auto theta = std::acos(XMVectorGetY(ToEye));//Elevation Angle
        
    auto x = (XM_PI + phi) / XM_2PI;
    auto y = theta / XM_PI;

    x += dx;
    y = std::fmax(0.01f, std::fmin(y - dy, 0.99f));

    phi = x * XM_2PI;
    theta = y * XM_PI;

    auto NewToEye = XMVector3Normalize(XMVectorSet(-sin(theta) * sin(phi), cos(theta), -sin(theta) * cos(phi), 0.0f));
    NewToEye *= L;
    mTarget = -NewToEye + mEye;
    mMtxView = XMMatrixLookAtRH(mEye, mTarget, mUp);
}

void Camera::ForwardBackward(f32 d)
{
    auto EyeVec = mEye - mTarget;
    auto toTargetLength = XMVectorGetX(XMVector3Length(EyeVec));
    if (toTargetLength < FLT_EPSILON) {
        return;
    }
    EyeVec = XMVector3Normalize(-EyeVec);//reverse

    mEye += EyeVec * (toTargetLength * d);

    mMtxView = XMMatrixLookAtRH(mEye, mTarget, mUp);
}

void Camera::SwingRightLeft(f32 rotate)
{
    //mTarget += rotate * XMVector3Cross(mEye - mTarget, mUp);
    //TrackingRotation(0, 0);
}
