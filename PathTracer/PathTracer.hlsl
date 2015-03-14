#include "Structures.hlsl"

#define MODE_COLOR 1
#define MODE_DIRECT 2
#define MODE_SSHADOWS 3
#define MODE_PT 4
#define MODE_ORIGINAL_SCENE 0

uint4 seed;

uint LCGStep(uint z, uint A, uint C)  
{  
  return (A*z+C);  
}  
uint TausStep(uint z, int S1, int S2, int S3, uint M)  
{  
  uint b=(((z << S1) ^ z) >> S2);  
  return (((z & M) << S3) ^ b);  
}  
float random01()
{  
	seed = uint4( TausStep(seed.x, 13, 19, 12, 4294967294UL),
				  TausStep(seed.y, 2, 25, 4, 4294967288UL),
				  TausStep(seed.z, 3, 11, 17, 4294967280UL),
				  LCGStep(seed.w, 1664525, 1013904223UL));

   return 2.3283064365387e-10 * (seed.x ^ seed.y ^ seed.z ^ seed.w );  
}


float Intersect(Sphere shpr, float3 ray, float3 start)
{
	float3 center = start - shpr.Center;
	float A = dot(ray, ray);
	float B  = 2 * dot(ray, center);
	float C = dot(center, center) - shpr.Radius * shpr.Radius;

	float det = B * B - 4 * A * C;
	float s_det = sqrt(abs(det));
	
	float t1 = ( -B + s_det) *0.5f *A;
	float t2 = ( -B - s_det) * 0.5f *A;
	
	if(det < 0 || max(t1, t2) < 0) return Wall;
	return ( (t1 < 0) ? t2 : (t2 < 0) ? t1 : min(t1,t2));
}

TraceResult TraceRay(float3 dir, float3 start)
{
	TraceResult tr;
	float t = Wall;
	int index = 0;
	for(int i = 0; i < NumSpheres; i++)
	{
		float newt = Intersect(Cornell[i],dir,start);
		index = (newt <= t) ? i : index;
		t = min(newt,t);
	}
	tr.pos = start + dir * (t-0.01f);
	tr.normal = normalize(tr.pos - Cornell[index].Center);
	tr.emission = Cornell[index].Emission;
	tr.color = Cornell[index].Albedo;
	tr.BRDF = Cornell[index].BRDF;
	return tr;
}

float3 TransfromVectorToNormal(float3 dir, float3 normal)
{
	float3 tangent = normalize(cross(normal,float3(0,-1,0)));
	float3 binormal = normalize(cross(normal,tangent));
	
	return normalize(mul(dir, float3x3 (binormal,tangent,normal)));
}

float3 SamplePhongLobe(float3 normal, float exponent)
{
	float rand = random01();
	float r = sqrt(1.0f - pow(rand, 2.0f / (exponent + 1.0f)));
	float phi = 2*pi *random01();

	return TransfromVectorToNormal(float3(r*cos(phi), r * sin(phi), pow(rand, 1.0f / (exponent + 1.0f))),normal);
}

float3 SampleHemisphere(float3 normal)
{
	float rand = random01();
	float r = sqrt(rand);
	float phi = 2 * pi*random01();
	return TransfromVectorToNormal( float3(r*cos(phi), r * sin(phi),sqrt(1.0f - rand)),normal);
}

float3 BRDF(float3 color, float3 normal, float3 inDir, int brdf,out float3 outDir)
{
	static float exponent = 50.0f;
	if(brdf == BRDF_DIFF)
	{
		outDir = SampleHemisphere(normal);
		return color;
	}
	else if (brdf == BRDF_REFL)
	{
		
		outDir = reflect(inDir,normal);
		return color;
	}
	else if(brdf == BRDF_REFR)
	{
		outDir = inDir;//refract(inDir,normal,1.0f);
		return float3(1,1,1);
	}
	else if(brdf == BRDF_SPEC)
	{
		float chance = random01();
		if(chance > 0.5f)
		{
			outDir = SampleHemisphere(normal);
			return color;
		}
		else
		{
			outDir = SamplePhongLobe(reflect(inDir,normal),exponent);
			return float3(1,1,1);
		}
	}
	else if(brdf == BRDF_GLOSSY)
	{
		outDir = SamplePhongLobe(reflect(inDir,normal),exponent);
		return color;
	}
	return 0;
}

float2 SampleDisk(float2 coords)
{
	return sqrt(coords.x) * (cos(2 * pi * coords.y) * float2(1,0), + sin(2 * pi * coords.y) * float2(0,1));
}

renderOutput main(transfer output)
{

	///initialize random seed before sampling
	seed = randomSeed.Load(int3(output.tex.x * ScreenSize.x, output.tex.y * ScreenSize.y,0));
	
	///Initial stage: get ray direction, trace initial ray
	///get a random offset for AA
	float4 pos = output.ray + float4((2 * random01() - 1.0f)/ScreenSize.x,(2 * random01() - 1.0f)/ScreenSize.y,0,0);	
	float4 projected = mul(projMatrix,pos);
	float3 ray = normalize(mul(viewMatrix, float4(projected.xyz,0)));
	float4 position = mul(viewMatrix,projected);

	TraceResult finalResult = TraceRay(ray, position.xyz / position.w);	
	TraceResult result = finalResult;
	
	if(mode == MODE_COLOR)
	{
		finalResult.color = result.color;
	}
	else if(mode == MODE_DIRECT)
	{		

		ray = normalize(LightSource  - result.pos);
		result = TraceRay(ray,result.pos);
		finalResult.color*= result.emission  * dot(finalResult.normal,ray) / 10;

	}
	else if(mode == MODE_SSHADOWS)
	{		
		float2 sample = LightRadius * SampleDisk(float2(random01(), random01()));
		float3 samplePoint = LightSource + float3(sample.x, 0, sample.y);
		float3 dir = samplePoint - finalResult.pos;
		ray = normalize(dir);
		result = TraceRay(ray,finalResult.pos);
		float area = pi*LightRadius*LightRadius;
		finalResult.color*= (area* 0.12f * result.emission * dot(finalResult.normal, ray) * dot(float3(0,-1,0), -ray))/ dot(dir,dir) ;			
	}
	else
	{
		float absorbProb = 0.8f;
		float chance = random01();
		float3 weigth =float3(1,1,1);						
		float3 accum = float3(0,0,0);		
		//do			
		for(int i = 0;  i < 2; i++)
		{				
			//chance = random01();			
			weigth *= BRDF(result.color, result.normal, ray, mode == MODE_ORIGINAL_SCENE ? BRDF_DIFF : result.BRDF, ray);// / absorbProb;
			result = TraceRay(ray, result.pos);
			accum += weigth * result.emission;
		}
	//	while(chance < absorbProb);
		finalResult.color = accum;
	}
	finalResult.color+=finalResult.emission;

	renderOutput ro;
	ro.accum = accumulator.Sample(smplr,output.tex) + float4(finalResult.color,1);

	///apply gamma correction to the image, no gamma for 16 bit output
	ro.color = pow (ro.accum / (float)frameNumber, 2.2);
	ro.seed = seed;
	return ro;
}