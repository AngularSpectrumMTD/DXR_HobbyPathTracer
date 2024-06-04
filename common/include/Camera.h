#ifndef __CAMERA_H__
#define __CAMERA_H__

#include <DirectXMath.h>
#include <wrl.h>

typedef float f32;
typedef int s32;
typedef unsigned int u32;
typedef double f64;

class Camera {
public:
    using XMFLOAT3 = DirectX::XMFLOAT3;
    using XMFLOAT4 = DirectX::XMFLOAT4;
    using XMMATRIX = DirectX::XMMATRIX;
    using XMVECTOR = DirectX::XMVECTOR;

    Camera();
    void SetLookAt(
        XMFLOAT3 vPos, XMFLOAT3 vTarget, XMFLOAT3 vUp = XMFLOAT3(0.0f, 1.0f, 0.0f)
    );
    void SetPerspective(
        f32 fovY, f32 aspect, f32 znear, f32 zfar);

    XMMATRIX GetViewMatrix() const { return mMtxView; }
    XMMATRIX GetProjectionMatrix() const { return mMtxProj; }

    XMVECTOR GetPosition() const { return mEye; }
    XMVECTOR GetTarget() const { return mTarget; }

    void OnMouseButtonDown(s32 buttonType, f32 dx, f32 dy);
    bool OnKeyDown(UINT8 wparam);
    void OnMouseMove(f32 dx, f32 dy);
    void OnMouseButtonUp();
    void OnMouseWheel(f32 rotate);

private:
    void EyeVecRotation(f32 dx, f32 dy);
    void ForwardBackward(f32 d);
    void SwingRightLeft(f32 rotate);

    XMVECTOR mEye;
    XMVECTOR mTarget;
    XMVECTOR mUp;

    XMMATRIX mMtxView;
    XMMATRIX mMtxProj;

    s32 mButtonType;
};

#endif
