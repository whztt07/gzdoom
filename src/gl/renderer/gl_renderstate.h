#ifndef __GL_RENDERSTATE_H
#define __GL_RENDERSTATE_H

#include <string.h>
#include "c_cvars.h"
#include "m_fixed.h"
#include "r_defs.h"

EXTERN_CVAR(Bool, gl_direct_state_change)

enum
{
	CCTRL_ATTRIB = 0,
	CCTRL_ATTRIBALPHA = 1,
	CCTRL_BUFFER = 2
};

enum
{
	SPECMODE_DEFAULT,
	SPECMODE_INFRARED,
	SPECMODE_FOGLAYER
};


enum EEffect
{
	EFF_NONE=-1,
	EFF_FOGBOUNDARY,
	EFF_SPHEREMAP,
	EFF_BURN,
	EFF_STENCIL,

	MAX_EFFECTS
};

class FRenderState
{
	TArray<unsigned int> mVAOStack;
	float mColor[5];
	int mColorControl;
	bool mTextureEnabled;
	bool mFogEnabled;
	bool mGlowEnabled;
	bool mBrightmapEnabled;
	int mSpecialEffect;
	int mTextureMode;
	float mSoftLightLevel;
	float mDynLight[3];
	int mDynTick;
	float mLightParms[2];
	int mSrcBlend, mDstBlend;
	float mAlphaThreshold;
	bool mAlphaTest;
	int mBlendEquation;
	int mSpecialMode;
	unsigned int mVertexArray, mLastVertexArray;
	int mLightIndex;
	int mTexMatrixIndex;

	float mGlowParms[16];
	PalEntry mFogColor, mObjectColor;
	float mFogDensity;

	int mEffectState;

	int glSrcBlend, glDstBlend;
	int gl_BlendEquation;

	bool ApplyShader();

public:
	FRenderState()
	{
		mDynTick = 0;
		Reset();
	}

	void Reset();

	int SetupShader(int &shaderindex);
	void Apply();
	void PushVertexArray()
	{
		mVAOStack.Push(mVertexArray);
	}

	void SetDynLightIndex(int li)
	{
		mLightIndex = li;
	}

	void SetColorControl(int ctrl)
	{
		mColorControl = ctrl;
	}

	void PopVertexArray()
	{
		if (mVAOStack.Size() > 0)
		{
			mVAOStack.Pop(mVertexArray);
		}
	}

	void SetVertexArray(unsigned int vao)
	{
		mVertexArray = vao;
	}

	void SetColor(float r, float g, float b, float a = 1.f, float desat = 0.f)
	{
		mColor[0] = r;
		mColor[1] = g;
		mColor[2] = b;
		mColor[3] = a;
		mColor[4] = desat;
	}

	void SetColor(PalEntry pe, float desat = 0.f)
	{
		mColor[0] = pe.r/255.f;
		mColor[1] = pe.g/255.f;
		mColor[2] = pe.b/255.f;
		mColor[3] = pe.a/255.f;
		mColor[4] = desat;
	}

	void SetColorAlpha(PalEntry pe, float alpha = 1.f, float desat = 0.f)
	{
		mColor[0] = pe.r / 255.f;
		mColor[1] = pe.g / 255.f;
		mColor[2] = pe.b / 255.f;
		mColor[3] = alpha;
		mColor[4] = desat;
	}

	void ResetColor()
	{
		mColor[0] = mColor[1] = mColor[2] = mColor[3] = 1.f;
		mColor[4] = 0.f;
	}

	const float *GetColor() const
	{
		return mColor;
	}

	void SetTextureMode(int mode)
	{
		mTextureMode = mode;
	}

	void EnableTexture(bool on)
	{
		mTextureEnabled = on;
	}

	void EnableFog(bool on)
	{
		mFogEnabled = on;
	}

	void SetEffect(int eff)
	{
		mSpecialEffect = eff;
	}

	void EnableGlow(bool on)
	{
		mGlowEnabled = on;
	}

	void EnableBrightmap(bool on)
	{
		mBrightmapEnabled = on;
	}

	void SetGlowParams(float *t, float *b, const secplane_t &top, const secplane_t &bottom)
	{
		mGlowEnabled = true;
		memcpy(&mGlowParms[0], t, 4 * sizeof(float));
		memcpy(&mGlowParms[4], b, 4 * sizeof(float));
		mGlowParms[8] = FIXED2FLOAT(top.a);
		mGlowParms[9] = FIXED2FLOAT(top.b);
		mGlowParms[10] = FIXED2FLOAT(top.ic);
		mGlowParms[11] = FIXED2FLOAT(top.d);
		mGlowParms[12] = FIXED2FLOAT(bottom.a);
		mGlowParms[13] = FIXED2FLOAT(bottom.b);
		mGlowParms[14] = FIXED2FLOAT(bottom.ic);
		mGlowParms[15] = FIXED2FLOAT(bottom.d);
	}

	void SetSpecialMode(int mode)
	{
		mSpecialMode = mode;
	}

	void SetSoftLightLevel(float lightlev)
	{
		mSoftLightLevel = lightlev;
	}

	void SetDynLight(float r,float g, float b)
	{
		mDynTick++;
		mDynLight[0] = r;
		mDynLight[1] = g;
		mDynLight[2] = b;
	}

	void SetFog(PalEntry c, float d)
	{
		mFogColor = c;
		if (d >= 0.0f) mFogDensity = d;
	}

	void SetLightParms(float f, float d)
	{
		mLightParms[0] = f;
		mLightParms[1] = d;
	}

	void SetObjectColor(PalEntry c)
	{
		mObjectColor = c;
	}

	PalEntry GetFogColor() const
	{
		return mFogColor;
	}

	void BlendFunc(int src, int dst)
	{
		if (!gl_direct_state_change)
		{
			mSrcBlend = src;
			mDstBlend = dst;
		}
		else
		{
			glBlendFunc(src, dst);
		}
	}

	// note: func may only be GL_GREATER or GL_GEQUAL to keep this simple. The engine won't need any other settings for the alpha test.
	void AlphaFunc(int func, float thresh)
	{
		if (func == GL_GEQUAL) thresh -= 0.001f;	// reduce by 1/1000.
		mAlphaThreshold = thresh;
	}

	void EnableAlphaTest(bool on)
	{
		mAlphaTest = on;
	}

	void BlendEquation(int eq)
	{
		if (!gl_direct_state_change)
		{
			mBlendEquation = eq;
		}
		else
		{
			::glBlendEquation(eq);
		}
	}

};

extern FRenderState gl_RenderState;

#endif
