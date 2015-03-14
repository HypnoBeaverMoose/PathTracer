#pragma once
#include<windows.h>
#include<d3d11.h>
#include<vector>
#include<DirectXMath.h>

#define VS_SHADER 0
#define PS_SHADER 1

typedef unsigned int uint;
namespace dx = DirectX; 

struct FPS
{
public:
	FPS(int maxSamples) :m_maxSamples(maxSamples),m_FPSaccum(60),m_samples(1),m_FPS(-1) { }
	void addSample(float time)
	{
		
		if(m_samples++>=m_maxSamples)
		{
			m_FPS =  (m_FPSaccum + time) / m_samples;
			m_FPSaccum = 0.0f;
			m_samples = 0;
		}
		else
			m_FPSaccum+=time;
	}
	float getFPS()
	{
		if(m_FPS < 0) return m_FPSaccum / m_samples;
		return m_FPS;
	}
private:
	float m_FPS;
	int m_maxSamples;
	int m_samples;
	float m_FPSaccum;
};

class D3D11Window
{
public:


	///This function shows the window and runs the main event loop
	static int RunMainLoop(int cmdShow);

	///Render callback,inheriting classes have to override this function to run rendering code
	virtual int RenderCallback() = 0;

	///inheriting classes have to override this function handle keyboard actions
	virtual int KeyboardCallback(unsigned int keydown)  = 0;

	///inheriting classes have to override this function handle mouse momvement and clicking
	virtual int MouseCallback(short mButton, short mWheel, short x, short y)  = 0;

	virtual ~D3D11Window();


protected:
	////Event handler
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

protected:

	///Constructor
	D3D11Window(int width, int heigth, HINSTANCE instance, char* name);	

	int InitDirect3D(int c_width, int c_heigth, HWND window);

	///Main Loop of the program - event loop and rendering happens here
	int MainLoop(int cmdShow);

	///Read a shader from file and comile it
	//ID3DBlob* CompileShader(LPCWSTR filename, const char* entry, UINT shaderType, UINT flags);

	bool CreateAndSetVertexShader(ID3DBlob* binaryCode, D3D11_INPUT_ELEMENT_DESC* input, int numElements);

	bool CreateAndSetPixelShader(ID3DBlob* binaryCode);

	bool CreateBuffer(D3D11_USAGE usage, uint size, uint bind_flags, uint cpu_access, ID3D11Buffer** buffer, void* data);

	bool CreateTexture(D3D11_USAGE usage, uint cpu_access, dx::XMUINT2 size, uint bind_flags,  DXGI_FORMAT format, ID3D11Texture2D** texture);


	/// Display an error message and quit the program
	void Die(char* message);

	///Release COM objects
	virtual void Release();

	bool D3D11Window::LoadShaderFromFile( const char* filename, std::vector<char>& buffer );

	bool UpdateBufferData(ID3D11Buffer* buffer, void* data, unsigned int size);

protected:
	HWND m_windowHandle;
	MSG m_Message;

	ID3D11RenderTargetView* m_RenderTargetView;
	ID3D11Texture2D* m_BackBuffer;
	ID3D11DeviceContext* m_DevContext;
	ID3D11Device* m_Device;	
	IDXGISwapChain* m_SwapChain;	
	ID3D11PixelShader* pShader;
	ID3D11VertexShader* vShader;
	D3D_FEATURE_LEVEL m_FLSupported;
	FPS m_FPSCoutner;


	static D3D11Window* m_instance;
};
