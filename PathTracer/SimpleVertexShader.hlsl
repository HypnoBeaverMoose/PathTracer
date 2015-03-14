#include "Structures.hlsl"

transfer main( float4 pos : POSITION )
{
	transfer tr;
	tr.position = float4(pos.x,pos.y,0,1);
	tr.ray = tr.position;
	tr.seed = pos.zw;
	tr.tex = float2((pos.x*0.5f + 0.5f),1.0f  - (pos.y*0.5f + 0.5f));
	return tr;
}