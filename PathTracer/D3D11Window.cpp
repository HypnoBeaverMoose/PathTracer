#include"D3D11Window.h"
//#include<d3dcompiler.h>
#include<vector>
#include<dxgi.h>
#include <sstream>
#include<fstream>

D3D11Window* D3D11Window::m_instance = NULL;

D3D11Window::D3D11Window(int width, int heigth, HINSTANCE instance, char* name) : m_FPSCoutner(10)
{

	///Initialize Window Class
	WNDCLASS l_wc;
	ZeroMemory(&l_wc,sizeof(WNDCLASS));

    l_wc.style          = CS_HREDRAW | CS_VREDRAW;
    l_wc.lpfnWndProc    = WndProc;
    l_wc.hInstance      = instance;
    l_wc.hCursor        = LoadCursor(NULL, IDC_ARROW);
    l_wc.hbrBackground  = (HBRUSH)(COLOR_WINDOW + 1);
    l_wc.lpszClassName  = name;
	
	int x = ( GetSystemMetrics(SM_CXSCREEN)  - width ) / 2;
	int y = ( GetSystemMetrics(SM_CYSCREEN) - heigth ) / 2;
	RECT windowRect = {x, y, x + width, y + heigth};	
	AdjustWindowRect(&windowRect,WS_SYSMENU,false);

	///Register Window Class
	if (!RegisterClass(&l_wc))
		Die("Class registration failed");

	///create Window Handle
	m_windowHandle = CreateWindow(	l_wc.lpszClassName,
									l_wc.lpszClassName,
									WS_SYSMENU ,
									windowRect.left,windowRect.top,
									windowRect.right - windowRect.left,
									windowRect.bottom - windowRect.top,
									NULL,NULL,instance,NULL);

	///Initialize Direct3D
	if(InitDirect3D(width,heigth,m_windowHandle)!=0)
		Die("Failed To init Direct3D");
}


int D3D11Window::InitDirect3D(int width, int height, HWND window)
{
	////Init SwapChain
	DXGI_SWAP_CHAIN_DESC l_scDesc;
	ZeroMemory(&l_scDesc,sizeof(DXGI_SWAP_CHAIN_DESC));
	l_scDesc.BufferCount = 2;
	l_scDesc.BufferDesc.Width = width;
	l_scDesc.BufferDesc.Height = height;
	l_scDesc.BufferDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	l_scDesc.BufferDesc.RefreshRate.Numerator = 1;
	l_scDesc.BufferDesc.RefreshRate.Denominator = 60;
	l_scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	l_scDesc.OutputWindow = window;
	l_scDesc.SampleDesc.Quality = 0;
	l_scDesc.SampleDesc.Count = 1;
	l_scDesc.Windowed = TRUE;
	l_scDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	D3D_FEATURE_LEVEL l_FLRequested  = D3D_FEATURE_LEVEL_11_0;

	if(FAILED(  D3D11CreateDeviceAndSwapChain(	NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0,
									&l_FLRequested, 1, D3D11_SDK_VERSION,
									&l_scDesc, &m_SwapChain, &m_Device, &m_FLSupported, &m_DevContext) )) return -1;


	///Get backbuffer, and assign it as a RenderTarget
	m_SwapChain->GetBuffer(0,__uuidof(ID3D11Texture2D), ( LPVOID* )&m_BackBuffer);	
	m_Device->CreateRenderTargetView(m_BackBuffer, NULL,&m_RenderTargetView);
	m_DevContext->OMSetRenderTargets(1, &m_RenderTargetView,NULL);

	///Init Viewport
    D3D11_VIEWPORT vp;
    vp.Width = (float)width;
    vp.Height = (float)height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    m_DevContext->RSSetViewports( 1, &vp );
	return 0;
}


int D3D11Window::MainLoop(int cmdShow)
{	
	ShowWindow(m_windowHandle,cmdShow);
	UpdateWindow(m_windowHandle);

	///used for FPS counting
	LARGE_INTEGER t1, t2, freq;
	QueryPerformanceFrequency(&freq);

	///Main message Loop
	while(true)
	{		
		if(PeekMessage(&m_Message,NULL, 0,0,PM_REMOVE))
		{
			TranslateMessage(&m_Message);			
			DispatchMessage(&m_Message);

			if(m_Message.message == WM_QUIT)  
				break;
		}
		else
		{
			QueryPerformanceCounter(&t1);

			if(m_instance->RenderCallback() != 0) return -1;
			
			QueryPerformanceCounter(&t2);
			m_FPSCoutner.addSample(freq.LowPart / (float)(t2.LowPart - t1.LowPart));	

			float frame = freq.LowPart / (float)(t2.LowPart - t1.LowPart);
			char buf[5];
			itoa(int(m_FPSCoutner.getFPS()),buf,10);
			SetWindowText(m_windowHandle, buf);
		}	
	}
	
	return m_Message.wParam;
}

int D3D11Window::RunMainLoop(int cmdShow) 
{
	if(m_instance!=NULL)
		return m_instance->MainLoop(cmdShow); 

	return -1;
} 

////Window Callback
LRESULT CALLBACK D3D11Window::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	
    switch(message)
    {
		case WM_KEYDOWN:
			m_instance->	KeyboardCallback(wParam);
			break;
		case WM_MOUSEWHEEL:
		case WM_LBUTTONDOWN:
		case WM_MOUSEMOVE:
			m_instance->	MouseCallback(LOWORD(wParam),(short)HIWORD(wParam),
										LOWORD(lParam),(short)HIWORD(lParam));

			break;
        case WM_DESTROY:
                PostQuitMessage(0);
                return 0;			
				
		default:
			return DefWindowProc(hWnd, message, wParam, lParam); 
		break;
    }

    return 0;
}

//ID3DBlob* D3D11Window::CompileShader(LPCWSTR filename, const char* entry, UINT shaderType, UINT flags)
//{
//	//todo: make sure we choose correct version of shadder model
//
//	std::string profile = ( shaderType == VS_SHADER ) ? "vs_4_0" : "ps_4_0";
//	ID3DBlob* shaderBlob;
//	ID3DBlob* errorBlob;
//
//	if(FAILED( D3DCompileFromFile(filename,NULL, NULL, entry,profile.c_str(), flags, 0, &shaderBlob, &errorBlob)))
//		return NULL;
//
//	return shaderBlob;	
//}

bool D3D11Window::CreateAndSetVertexShader(ID3DBlob* binaryCode, D3D11_INPUT_ELEMENT_DESC* input, int numElements)
{
	if(FAILED(m_Device->CreateVertexShader(binaryCode->GetBufferPointer(),binaryCode->GetBufferSize(),NULL,&vShader))) 
		return false;

	m_DevContext->VSSetShader(vShader,NULL,NULL);

	ID3D11InputLayout* pLayout;
	HRESULT hr = m_Device->CreateInputLayout(input, numElements, binaryCode->GetBufferPointer(), binaryCode->GetBufferSize(), &pLayout);
	m_DevContext->IASetInputLayout(pLayout);

	return true;
}

bool D3D11Window::CreateBuffer(D3D11_USAGE usage, uint size, uint bind_flags, uint cpu_access, ID3D11Buffer** buffer, void* data)
{
	D3D11_BUFFER_DESC  buffer_desc = { size, usage, bind_flags, cpu_access, 0, 0, };
	D3D11_SUBRESOURCE_DATA  subrecData;
	subrecData.SysMemSlicePitch = subrecData.SysMemPitch = 0;
	subrecData.pSysMem = data;
	
	HRESULT hr = m_Device->CreateBuffer( &buffer_desc, data == NULL ? NULL : &subrecData, buffer);
	return !FAILED(hr);
}

bool D3D11Window::CreateTexture(D3D11_USAGE usage, uint cpu_access, dx::XMUINT2 size, uint bind_flags,  DXGI_FORMAT format, ID3D11Texture2D** texture)
{
	DXGI_SAMPLE_DESC sample_desc = {1,0};
	D3D11_TEXTURE2D_DESC texture_desctiptor = {size.x, size.y, 1, 1, format, sample_desc, usage, bind_flags,  cpu_access, 0};
	return(!FAILED(  m_Device->CreateTexture2D( &texture_desctiptor, NULL, texture) ));	
}


bool D3D11Window::CreateAndSetPixelShader(ID3DBlob* binaryCode)
{
	if(FAILED(m_Device->CreatePixelShader(binaryCode->GetBufferPointer(),binaryCode->GetBufferSize(),NULL,&pShader))) 
		return false;

	m_DevContext->PSSetShader(pShader,NULL,NULL);
	return true;
}

void D3D11Window::Die(char* message)
{
	MessageBox(m_windowHandle,message,"FAIL!",MB_OK);
	Release();
	exit(-1);
}
bool D3D11Window::LoadShaderFromFile( const char* filename, std::vector<char>& buffer )
{
	
	std::ifstream input(filename,std::ios::in | std::ios::binary);
	input.seekg(0,std::ios::end);
	int size = input.tellg();
	
	if(size < 0) return false;

	buffer.assign(size,0);
	input.seekg(0,std::ios::beg);

	input.read(buffer.data(),size);
	
	return true;
}


bool D3D11Window::UpdateBufferData(ID3D11Buffer* buffer, void* data, unsigned int size)
{
	D3D11_MAPPED_SUBRESOURCE subrec;	
	if(FAILED(m_DevContext->Map(buffer,0,D3D11_MAP_WRITE_DISCARD,0,&subrec)))
		return false;

	memcpy(subrec.pData,data,size);
	m_DevContext->Unmap(buffer,0);
	return true;
}

void D3D11Window::Release()
{
	m_BackBuffer->Release();
	m_DevContext->Release();
	m_Device->Release();
	m_RenderTargetView->Release();
	m_SwapChain->Release();	
	pShader->Release();
	vShader->Release();

}
D3D11Window::~D3D11Window()
{
	Release();
}