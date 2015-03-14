#include "PathTracer.h"
#include<fstream>
#include<vector>
#include<sstream>
#include<random>
#include<ctime>

int PathTracer::InitializeWindow(int width, int heigth, HINSTANCE instance, char* name)
{

	if(m_instance == NULL)
	{
		m_instance = new PathTracer(width,heigth,instance,name);
		return 0;
	}

	return -1;
}

PathTracer::PathTracer(int width, int heigth, HINSTANCE instance, char* name) 
	: D3D11Window(width,heigth,instance,name), m_width(width), m_height(heigth), m_NeedCamUpdate(false)
{
	InitShaders();
	InitBuffers();
	InitTextures();
	m_camPosition = dx::XMVectorSet(0,0,0,1);
	m_camRotation = dx::XMVectorSet(0,0,0,0);
	m_oldMousePos.x = m_oldMousePos.y = -1;
}


void PathTracer::InitTextures()
{
	DXGI_FORMAT accum_format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	DXGI_FORMAT random_format = DXGI_FORMAT_R32G32B32A32_UINT;
	///0 is the texture input, 1 is render target
	bool created  = CreateTexture(D3D11_USAGE_DYNAMIC,D3D11_CPU_ACCESS_WRITE,dx::XMUINT2(m_width, m_height), 
		D3D11_BIND_SHADER_RESOURCE,accum_format, &m_AccumulatorTextures[0]);

	created &= CreateTexture(D3D11_USAGE_DEFAULT, 0, dx::XMUINT2(m_width, m_height), 
		D3D11_BIND_RENDER_TARGET,accum_format, &m_AccumulatorTextures[1]);

	created &= CreateTexture(D3D11_USAGE_DYNAMIC,D3D11_CPU_ACCESS_WRITE,dx::XMUINT2(m_width, m_height), 
		D3D11_BIND_SHADER_RESOURCE,random_format, &m_RandomTextures[0]);

	created &= CreateTexture(D3D11_USAGE_DEFAULT, 0, dx::XMUINT2(m_width, m_height), 
		D3D11_BIND_RENDER_TARGET,random_format, &m_RandomTextures[1]);

	if(!created)
		Die("Texture Creation Failed");

 	D3D11_SHADER_RESOURCE_VIEW_DESC rv_desc;
	ZeroMemory (&rv_desc, sizeof (rv_desc));
	rv_desc.Format = accum_format;
	rv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	rv_desc.Texture2D.MipLevels = 1;
	rv_desc.Texture2D.MostDetailedMip = 0;
	if(FAILED(m_Device->CreateShaderResourceView(m_AccumulatorTextures[0],&rv_desc,&m_resourceViews[0])))
		Die("Texture Genration Failed");
	rv_desc.Format  = random_format;
	if(FAILED(m_Device->CreateShaderResourceView(m_RandomTextures[0],&rv_desc,&m_resourceViews[1])))
		Die("Random Texture Genration Failed");

	m_DevContext->PSSetShaderResources(0,1,&m_resourceViews[0]);
	m_DevContext->PSSetShaderResources(1,1,&m_resourceViews[1]);

	D3D11_RENDER_TARGET_VIEW_DESC rt_desc;
	ZeroMemory (&rt_desc, sizeof (rt_desc));
	rt_desc.Format = random_format;
	rt_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	m_Device->CreateRenderTargetView(m_RandomTextures[1], &rt_desc, &m_randomOutput);
	rt_desc.Format = accum_format;
	m_Device->CreateRenderTargetView(m_AccumulatorTextures[1], &rt_desc, &m_accumulator);

	ID3D11RenderTargetView* rts[3] = {m_RenderTargetView, m_randomOutput, m_accumulator};
	m_DevContext->OMSetRenderTargets(3,rts,NULL);



	D3D11_MAPPED_SUBRESOURCE subrec;
	if(FAILED(m_DevContext->Map(m_RandomTextures[0],0,D3D11_MAP_WRITE_DISCARD,0,&subrec)))
		Die("Failed to Map Random Texture");

	UINT* pTexels = (UINT*)subrec.pData;
	for( UINT row = 0; row < m_height; row++ )
	{
		UINT rowStart = row * (subrec.RowPitch / sizeof(UINT));
		for( UINT col = 0; col < m_width; col++ )
		{
			UINT colStart = col * 4;
			pTexels[rowStart + colStart + 0] = random();
			pTexels[rowStart + colStart + 1] = random();
			pTexels[rowStart + colStart + 2] = random();
			pTexels[rowStart + colStart + 3] = random();
		}
	}
	m_DevContext->Unmap(m_RandomTextures[0],0);
}


void PathTracer::InitBuffers()
{
	m_vertexData[0] = dx::XMVectorSet(-1.0f, 1.0f, 0.0f, 1.0f);
	m_vertexData[1] = dx::XMVectorSet(1.0f, 1.0f, 0.0f, 1.0f);
	m_vertexData[2] = dx::XMVectorSet(-1.0f,-1.0f, 0.0f, 1.0f);
	m_vertexData[3] = dx::XMVectorSet(1.0f,-1.0f, 0.0f, 1.0f);
	if(!CreateBuffer(D3D11_USAGE_DEFAULT,sizeof(m_vertexData),D3D11_BIND_VERTEX_BUFFER,0,&m_VertexBuffer,m_vertexData))
		Die("Vertex Buffer Creation Failed");
	
	unsigned int stride = sizeof(dx::XMVECTOR);
	unsigned int offset = 0;	

	m_DevContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	m_DevContext->IASetVertexBuffers(0,1,&m_VertexBuffer,&stride,&offset);
	
	
	m_PerFrameData.frameNumber = 0;
	m_PerFrameData.mode  = 4;
	m_PerFrameData.size[0] = m_width;
	m_PerFrameData.size[1] = m_height;
	if(!CreateBuffer(D3D11_USAGE_DYNAMIC, sizeof(m_PerFrameData), D3D11_BIND_CONSTANT_BUFFER,	
									D3D11_CPU_ACCESS_WRITE, &m_PerFrameBuffer, &m_PerFrameData))
		Die("PerFrame Buffer Creation Failed");

	m_DevContext->PSSetConstantBuffers(0,1,&m_PerFrameBuffer);
	
	
	float aspect = m_width/(float)m_height;
	dx::XMMATRIX  proj = dx::XMMatrixPerspectiveFovRH(dx::XM_PIDIV4,aspect,1,100);
	dx::XMVECTOR det= dx::XMMatrixDeterminant(proj);
	m_CameraData.m_projMatrix = dx::XMMatrixInverse(&det,proj);
	m_CameraData.m_ViewMatrix = dx::XMMatrixIdentity();
	if(!CreateBuffer(D3D11_USAGE_DYNAMIC, sizeof(m_CameraData), D3D11_BIND_CONSTANT_BUFFER,	
										 D3D11_CPU_ACCESS_WRITE, &m_CameraBuffer, &m_CameraData))
	Die("Camera Buffer Creation Failed");
	m_DevContext->PSSetConstantBuffers(1,1,&m_CameraBuffer);
	
}

void PathTracer::InitShaders()
{
	D3D11_INPUT_ELEMENT_DESC ied[] = { {"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0} };

	ID3D11InputLayout* pLayout;
	std::vector<char> vsByteCode;
	std::vector<char> psByteCode;

	bool loaded = LoadShaderFromFile("SimpleVertexShader.cso",vsByteCode);	
	loaded |= LoadShaderFromFile("PathTracer.cso",psByteCode);	
	if(!loaded) Die("Shaders Not Loaded");

	HRESULT hr_vs = m_Device->CreateVertexShader(vsByteCode.data(),vsByteCode.size(),NULL,&vShader);	
	HRESULT hr_ps = m_Device->CreatePixelShader(psByteCode.data(),psByteCode.size(),NULL,&pShader);
	if(FAILED(hr_ps) || FAILED(hr_ps))
		Die("Shaders Not Created");

	if(FAILED(m_Device->CreateInputLayout(ied, 1, vsByteCode.data(), vsByteCode.size(), &pLayout)))
		Die("Input Layout Craeation Failed");

	m_DevContext->VSSetShader(vShader, NULL, NULL);	
	m_DevContext->PSSetShader(pShader, NULL, NULL);
	m_DevContext->IASetInputLayout(pLayout);
}

int PathTracer::RenderCallback()
{

	dx::XMVECTOR clearColor = dx::XMVectorSet(0,0,0,0); 	 
	m_DevContext->ClearRenderTargetView(m_RenderTargetView,clearColor.m128_f32);		
	
	if(m_NeedCamUpdate)
	{
		m_DevContext->ClearRenderTargetView(m_accumulator,clearColor.m128_f32);		
		m_CameraData.m_ViewMatrix = dx::XMMatrixRotationRollPitchYawFromVector(m_camRotation) * dx::XMMatrixTranslationFromVector(m_camPosition);
		UpdateBufferData(m_CameraBuffer,&m_CameraData, sizeof(m_CameraData));
		m_NeedCamUpdate = false;
	}
	m_DevContext->CopyResource(m_AccumulatorTextures[0],m_AccumulatorTextures[1]);		

	m_PerFrameData.frameNumber++;
	UpdateBufferData(m_PerFrameBuffer,&m_PerFrameData, sizeof(m_PerFrameData));

	UpdateBufferData(m_VertexBuffer,&m_vertexData, sizeof(m_vertexData));
	m_DevContext->Draw(4,0);	
	m_SwapChain->Present(0,0);
	m_DevContext->CopyResource(m_RandomTextures[0],m_RandomTextures[1]);
	
	return 0;
}

int PathTracer::KeyboardCallback(unsigned int keydown)
{	

	switch (keydown)
	{
	case VK_SPACE:
		m_camPosition = dx::XMVectorZero();
		m_camRotation = dx::XMVectorZero();
		m_PerFrameData.mode = 0; break;
	case 'A' : 
		m_camPosition = dx::XMVectorAdd(m_camPosition,dx::XMVector4Transform(dx::XMVectorSet(-SPEED,0,0,1), 
																	dx::XMMatrixRotationRollPitchYawFromVector(m_camRotation)));break;
	case 'D' : 
		m_camPosition = dx::XMVectorAdd(m_camPosition,dx::XMVector4Transform(dx::XMVectorSet(SPEED,0,0,1), 
																	dx::XMMatrixRotationRollPitchYawFromVector(m_camRotation)));break;
	case 'W' : 
		m_camPosition = dx::XMVectorAdd(m_camPosition,dx::XMVector4Transform(dx::XMVectorSet(0,0,-SPEED,1), 
																	dx::XMMatrixRotationRollPitchYawFromVector(m_camRotation)));break;
	case 'S' : 
		m_camPosition = dx::XMVectorAdd(m_camPosition,dx::XMVector4Transform(dx::XMVectorSet(0,0,+SPEED,1), 
																	dx::XMMatrixRotationRollPitchYawFromVector(m_camRotation)));break;
	case '0':
		m_PerFrameData.mode = 0; break;
	case '1':
		m_PerFrameData.mode = 1; break;
	case '2':
		m_PerFrameData.mode = 2; break;
	case '3':
		m_PerFrameData.mode = 3; break;
	case '4':
		m_PerFrameData.mode = 4; break;
	default:
		return 0;
		break;
	}
	m_NeedCamUpdate = true;
	m_PerFrameData.frameNumber = 0;

	return 0;
}

int PathTracer::MouseCallback(short mButton, short mWheel, short x, short y)
{
	if(mButton != MK_LBUTTON)
	{
		m_oldMousePos.x = m_oldMousePos.y = -1;
		return 0;
	}
	
	if(	m_oldMousePos.x == -1)
	{
		m_oldMousePos.y  = y;
		m_oldMousePos.x = x;
	}
	dx::XMFLOAT2 delta;
	delta.y = (m_oldMousePos.x - x)/(float)m_width;
	delta.x = (m_oldMousePos.y - y)/(float)m_height;

	m_camRotation  = dx::XMVectorAdd(m_camRotation, dx::XMLoadFloat2(&delta));
	m_NeedCamUpdate = true;

	m_oldMousePos.x = x;
	m_oldMousePos.y = y;
	m_PerFrameData.frameNumber = 0;
	return 0;
}

PathTracer::~PathTracer(void)
{
}
