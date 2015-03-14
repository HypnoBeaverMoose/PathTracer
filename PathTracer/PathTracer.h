#pragma once
#include "d3d11window.h"
#include<wlclient.h>
#include<random>

class PathTracer :
	public D3D11Window
{
public:
	struct CameraData
	{
		DirectX::XMMATRIX  m_projMatrix;
		DirectX::XMMATRIX  m_ViewMatrix;
	};
	struct PerFrameData
	{
		uint frameNumber;
		uint size[2];
		uint mode;
	};
	static int InitializeWindow(int width, int heigth, HINSTANCE instance, char* name);

	PathTracer(int width, int heigth, HINSTANCE instance, char* name);
	
	///Render callback,inheriting classes have to override this function to run rendering code
	virtual int RenderCallback();

	///inheriting classes have to override this function handle keyboard actions
	virtual int KeyboardCallback(unsigned int keydown);

	///inheriting classes have to override this function handle mouse momvement and clicking
	virtual int MouseCallback(short mButton, short mWheel, short x, short y);


	virtual ~PathTracer(void);

private:
	void InitShaders();

	void InitBuffers();

	void InitTextures();

private:
		ID3D11Buffer* m_VertexBuffer;
		ID3D11Buffer* m_CameraBuffer;
		ID3D11Buffer* m_PerFrameBuffer;

		DirectX::XMVECTOR  m_vertexData[4];
		CameraData m_CameraData;
		PerFrameData m_PerFrameData;

		uint m_width;
		uint m_height;

		dx::XMINT2 m_oldMousePos;
		dx::XMVECTOR m_camPosition;
		dx::XMVECTOR m_camRotation;
		bool m_NeedCamUpdate;
		static const int SPEED = 1;

		ID3D11Texture2D* m_RandomTextures[2];	
		ID3D11Texture2D* m_AccumulatorTextures[2];	
		ID3D11ShaderResourceView* m_resourceViews[2];
		ID3D11RenderTargetView* m_randomOutput;
		ID3D11RenderTargetView* m_accumulator;

		std::mt19937 random;

		std::random_device m_random;
};

