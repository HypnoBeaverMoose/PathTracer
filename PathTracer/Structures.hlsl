
struct transfer
{
	float4 position : SV_POSITION;
	float2 tex: TEXCOORD0;
	float4 ray: NORMAL;
	float2 seed:TEXCOOR2;
};

cbuffer PerFrameData : register(b0)
{
	uint frameNumber;
	uint2 ScreenSize;
	uint mode;
};

cbuffer CameraData : register(b1)
{
	float4x4 projMatrix;
	float4x4 viewMatrix;
};

struct Sphere 
{ 
	float3 Center; 
	float Radius; 
	float3 Albedo; 
	float3 Emission; 
	uint BRDF;
}; 

#define BRDF_DIFF 1
#define BRDF_REFL 2
#define BRDF_REFR 3
#define BRDF_SPEC 4
#define BRDF_GLOSSY 5

static const int NumSpheres = 9; 
static const float Wall = 1e4; 
static const Sphere Cornell[NumSpheres] = 
{ 
	{float3(Wall+49.0f, 21.2f,-149.0f),Wall,float3(0.0f,0.8f,0.0f),float3(0,0,0),BRDF_DIFF},//Right 
	{float3(-Wall-49.0f,21.2f, -149.0f),Wall,float3(0.8f,0.0f,0.0f),float3(0,0,0),BRDF_DIFF},//Left 
	{float3(0,21.2f,Wall+10.6f),Wall,float3(0.8f,0.8f,0.8f),float3(0,0,0),BRDF_DIFF},//Front 
	{float3(0,21.2f,-Wall-230.6f),Wall,float3(0.8f,0.8f,0.8f),float3(0,0,0),BRDF_DIFF},//Back 
	{float3(0,Wall+39.6,-149.0f),Wall,float3(0.8f,0.8f,0.8f),float3(0,0,0),BRDF_DIFF},//Top 
	{float3(0,-Wall-42,-149.0f),Wall,float3(0.8f,0.8f,0.8f),float3(0,0,0),BRDF_GLOSSY},//Bottom 
	{float3(-23,-25.5f,-183.6f),16.5f,float3(0.9f,0.9f,0.9f),float3(0,0,0),BRDF_SPEC},//Left Sphere 
	{float3(23,-25.5f,-152.6f),16.5f,float3(0.9f,0.9f,0.9f),float3(0,0,0),BRDF_REFL},//Right Sphere 
	{float3(0,639.6-.21,-149.0f),600.0f,float3(0,0,0),float3(20,20,12),BRDF_DIFF},//Light Source 
};



static const float3 LightSource = Cornell[8].Center - float3(0,Cornell[8].Radius,0);
static const float LightRadius = 50;

struct TraceResult
{
	float3 color;
	float3 pos;
	float3 normal;
	float3 emission;
	uint BRDF;
};
static const float pi = 3.14159265359f;

Texture2D accumulator: register(t0);
Texture2D<uint4> randomSeed: register(t1);

SamplerState smplr
{
    Filter = LINEAR;
    AddressU = Wrap;
    AddressV = Wrap;
};

struct renderOutput
{
	float4 color :SV_TARGET0;
	uint4 seed :SV_TARGET1;
	float4 accum :SV_TARGET2;
};