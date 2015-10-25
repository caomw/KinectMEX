#pragma once
#include <mex.h>
#include <Kinect.h>


typedef struct kcontext
{
	IKinectSensor* sensor;
	IMultiSourceFrameReader* reader;
	int depthframewidth;
	int depthframeheight;
	int colorframewidth;
	int colorframeheight;
	ColorImageFormat format;
} KMcontext;

KMcontext* context;

template <class T> void SafeRelease(T **ppT)
{
	if (*ppT)
	{
		(*ppT)->Release();
		*ppT = NULL;
	}
}


bool commandIs(const mxArray* mxCommand, const char* command);
void processInit(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]);
void processNextFrame(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]);
void exitCB();