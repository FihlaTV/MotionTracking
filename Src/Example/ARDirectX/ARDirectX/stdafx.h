#pragma once


#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#ifndef _WINSOCK_DEPRECATED_NO_WARNINGS 
#define _WINSOCK_DEPRECATED_NO_WARNINGS 
#endif

#include "../Common/Common.h"
#include "../Network/Network.h"
#include "../Graphic/Graphic.h"
#include "../Framework/framework.h"

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv/cvaux.h>

#include <AR/ar.h>
#include <AR/gsub.h>
#include <AR/video.h>
#include <AR/param.h>			// arParamDisp()
#include <AR/gsub_lite.h>

#pragma comment(lib, "ARd.lib")
#pragma comment(lib, "ARICPd.lib")
#pragma comment(lib, "ARgsubd.lib")
#pragma comment(lib, "ARgsub_lited.lib")
#pragma comment(lib, "ARvideod.lib")
#pragma comment(lib, "pthreadVC2.lib")
#pragma comment(lib, "winmm.lib")

#ifdef _DEBUG
#pragma comment(lib, "opencv_core2411d.lib")
#pragma comment(lib, "opencv_highgui2411d.lib")
#pragma comment(lib, "opencv_imgproc2411d.lib")
#pragma comment(lib, "opencv_features2d2411d.lib")
#pragma comment(lib, "opencv_calib3d2411d.lib")
#else
#pragma comment(lib, "opencv_core2411.lib")
#pragma comment(lib, "opencv_highgui2411.lib")
#pragma comment(lib, "opencv_imgproc2411.lib")
#pragma comment(lib, "opencv_features2d2411.lib")
#pragma comment(lib, "opencv_calib3d2411.lib")
#endif

#include "arengine.h"
#include "arutil.h"
#include "qrcode.h"
#include "qrmap.h"

