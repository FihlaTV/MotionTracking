
#ifndef _CRT_SECURE_NO_WARNINGS
	#define _CRT_SECURE_NO_WARNINGS
#endif

#include <iostream>
#include "../../../../Common/Common/Common.h"
#include "../../../../Common/Network/Network.h"
#include "../../../../Common/CamCommon/CamCommon.h"

#include "../../../../Common/CamCommon/qrcode.h"
#include "../../../../Common/CamCommon/arqrcode.h"
#include "../../../../Common/CamCommon/fps.h"

using namespace std;

void main()
{
	cout << "init camera..." << endl;

	cDxCapture dxCapture;
	if (!dxCapture.Init(0, 640, 480, true))
	{
		return;
	}

// 	Mat screen(1080, 1920, CV_8UC3);
// 	screen.setTo(Scalar(0, 0, 0));
	// screen window
	// 	const int WIDTH = 1920;
	// 	const int HEIGHT = 1080;
	// 	cvNamedWindow("screen", 0);
	// 	cvResizeWindow("screen", WIDTH, HEIGHT);
	//	cvSetWindowProperty("screen", CV_WND_PROP_FULLSCREEN, CV_WINDOW_FULLSCREEN);

	const int radius = 80;
	Scalar color(255, 255, 255);

	cvproc::cArQRCode qrCode;
	if (!qrCode.Init("recognition_edge_config.cfg"))
	{
		return;
	}
	qrCode.m_dbgShow = true;
	//qrCode.m_detectPoint.m_makeKeypointImage = true;

	cvproc::cFps fps;
	while (1)
	{
		Mat src(dxCapture.GetCloneBufferToImage(false));
		if (src.empty())
			continue;

// 		Mat bin;
// 		if (qrCode.Detect(src))
// 			bin = qrCode.m_detectPoint.m_binWithKeypoints;
// 		else
// 			bin = qrCode.m_detectPoint.m_binMat;

		qrCode.Detect(src);

		fps.Update(src);
		imshow("camera", src);
//		imshow("bin", bin);

		const int key = cvWaitKey(1);
		if (key == VK_ESCAPE)
		{
			break;
		}
		else if (key == 'r')
		{
			qrCode.UpdateConfig("recognition_edge_config.cfg");
		}
	}
}
