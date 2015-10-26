#include "MexMain.h"
#include <comdef.h>
#include <tchar.h>

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
	if (nrhs < 1)
	{
		mexErrMsgTxt("Not enough input arguments."); return;
	}
	if (!mxIsChar(prhs[0]))
	{
		mexErrMsgTxt("First parameter must be a string"); return;
	}

	if (commandIs(prhs[0], "init"))
	{
		processInit(nlhs, plhs, nrhs, prhs);
	}
	if (commandIs(prhs[0], "nextframe"))
	{
		processNextFrame(nlhs, plhs, nrhs, prhs);
	}

	mexPrintf("%d", GetCurrentThreadId());
	mexAtExit(exitCB);
}

// Check if some command is really some given one
bool commandIs(const mxArray* mxCommand, const char* command)
{
	double result;
	mxArray* plhs1[1];
	mxArray* prhs1[1];
	mxArray* plhs2[1];
	mxArray* prhs2[2];

	if (mxCommand == NULL) { mexErrMsgTxt("'mxCommand' is null"); return false; }
	if (command == NULL) { mexErrMsgTxt("'command' is null"); return false; }
	if (!mxIsChar(mxCommand)) { mexErrMsgTxt("'mxCommand' is not a string"); return false; }

	// First trim
	prhs1[0] = (mxArray*)mxCommand;
	mexCallMATLAB(1, plhs1, 1, prhs1, "strtrim");

	// Then compare
	prhs2[0] = mxCreateString(command);
	prhs2[1] = plhs1[0];
	mexCallMATLAB(1, plhs2, 2, prhs2, "strcmpi");

	// Return comparison result
	result = mxGetScalar(plhs2[0]);
	return (result != 0.0);
}

void processInit(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
	HRESULT hr;
	IKinectSensor *sensor;
	IColorFrameSource *cs;
	IDepthFrameSource *ds;
	context = new KMcontext();


	if (!SUCCEEDED(GetDefaultKinectSensor(&sensor)))
	{
		return;
	}
	hr = sensor->Open();
	if (FAILED(hr))
	{
		return;
	}
	sensor->get_ColorFrameSource(&cs);
	sensor->get_DepthFrameSource(&ds);
	cs->OpenReader(&(context->cr));
	ds->OpenReader(&(context->dr));
	context->sensor = sensor;
	mexPrintf("%d", GetCurrentThreadId());
}

void processNextFrame(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
	if (context == NULL || context->cr == NULL)
	{
		mexPrintf("Not inited");
		return;
	}
	HRESULT hr = E_FAIL;
	mexPrintf("%d", GetCurrentThreadId());
	IColorFrame* pColorFrame = NULL;
	IDepthFrame* pDepthFrame = NULL;
	IFrameDescription *pColorFrameDescription, *pDepthFrameDescription;
	UINT depthlength, colorlength;
	UINT16* depthdata;
	BYTE* colordata;

	while (pColorFrame==NULL)
	{
		hr = context->cr->AcquireLatestFrame(&pColorFrame);
	}
	ColorImageFormat imageFormat;

	pColorFrame->get_RawColorImageFormat(&imageFormat);
	context->format = imageFormat;
	pColorFrame->get_FrameDescription(&pColorFrameDescription);
	pColorFrameDescription->get_Width(&(context->colorframewidth));
	pColorFrameDescription->get_Height(&(context->colorframeheight));
	int colorBufferSize = context->colorframeheight*context->colorframewidth*sizeof(RGBQUAD);
	colordata = new BYTE[colorBufferSize];

	if (imageFormat == ColorImageFormat_Bgra)
	{
		pColorFrame->AccessRawUnderlyingBuffer(&colorlength, &colordata);
	}
	else
	{
		hr = pColorFrame->CopyConvertedFrameDataToArray(colorBufferSize, colordata, ColorImageFormat_Bgra);
	}

	mxArray* rData = mxCreateNumericMatrix(context->colorframeheight, context->colorframewidth, mxUINT8_CLASS, mxREAL);
	mxArray* gData = mxCreateNumericMatrix(context->colorframeheight, context->colorframewidth, mxUINT8_CLASS, mxREAL);
	mxArray* bData = mxCreateNumericMatrix(context->colorframeheight, context->colorframewidth, mxUINT8_CLASS, mxREAL);
	int k = 0;
	int colNum = mxGetN(rData);
	int rowNum = mxGetM(rData);
	UINT8* rPt = (UINT8*)mxGetData(rData);
	UINT8* gPt = (UINT8*)mxGetData(gData);
	UINT8* bPt = (UINT8*)mxGetData(bData);
	for (int i = 0; i < rowNum; i++)
	{
		for (int j = 0; j < colNum; j++)
		{
			*(bPt + i + j*rowNum) = colordata[k++];
			*(gPt + i + j*rowNum) = colordata[k++];
			*(rPt + i + j*rowNum) = colordata[k++];
			k++;
		}
	}
	mxArray* tmpArray[4];
	const mwSize dims[] = { 1 };
	mxArray* numDim = mxCreateNumericArray(1, dims, mxINT32_CLASS, mxREAL);
	UINT32* numPtr = (UINT32*)mxGetData(numDim);
	*numPtr = 3;
	tmpArray[0] = numDim; tmpArray[1] = rData; tmpArray[2] = gData; tmpArray[3] = bData;
	mexCallMATLAB(1, &(plhs[1]), 4, tmpArray, "cat");
	SafeRelease(&pColorFrameDescription);
	SafeRelease(&pColorFrame);


	while (pDepthFrame==NULL)
	{
		hr = context->dr->AcquireLatestFrame(&pDepthFrame);
	}
	pDepthFrame->AccessUnderlyingBuffer(&depthlength, &depthdata);

	pDepthFrame->get_FrameDescription(&pDepthFrameDescription);
	pDepthFrameDescription->get_Height(&(context->depthframeheight));
	pDepthFrameDescription->get_Width(&(context->depthframewidth));

	//Next part set data pointer of mxArray

	UINT16* mDepthdata = (UINT16*)mxMalloc(sizeof(*depthdata)*depthlength);
	mxArray* tmpMat;
	memcpy(mDepthdata, depthdata, sizeof(*depthdata)*depthlength);
	tmpMat = mxCreateNumericArray(0, 0, mxUINT16_CLASS, mxREAL);
	mxSetData(tmpMat, mDepthdata);
	mxSetM(tmpMat, context->depthframewidth);
	mxSetN(tmpMat, context->depthframeheight);
	mexCallMATLAB(1, &(plhs[0]), 1, &tmpMat, "transpose");

	nlhs = 2;
	SafeRelease(&pDepthFrameDescription);
	SafeRelease(&pDepthFrame);
}

void exitCB()
{
	SafeRelease(&(context->cr));
	SafeRelease(&(context->dr));
	SafeRelease(&(context->sensor));
}
