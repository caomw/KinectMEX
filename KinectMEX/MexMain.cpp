#include "MexMain.h"

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

	mexAtExit(exitCB);
}

// Check if some command is really some givent one
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
	IMultiSourceFrameArrivedEventArgs* pMultiArgs;
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
	sensor->OpenMultiSourceFrameReader(FrameSourceTypes_Color | FrameSourceTypes_Depth, &(context->reader));
	context->sensor = sensor;
}

void exitCB()
{
	SafeRelease(&(context->reader));
	SafeRelease(&(context->sensor));
}
