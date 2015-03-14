#pragma once


#define WIN32_LEAN_AND_MEAN
#include <windows.h>
//#include "Mandelbrot.h"
#include "PathTracer.h"

int APIENTRY WinMain( HINSTANCE hInstance,HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	PathTracer::InitializeWindow(1200,675,hInstance,"PathTracer");
	return PathTracer::RunMainLoop(nCmdShow);
}