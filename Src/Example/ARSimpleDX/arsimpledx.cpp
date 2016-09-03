
#include "../../../../../Common/Common/Common.h"
#include "../../../../../Common/Network/Network.h"
#include "../../../../../Common/CamCommon/CamCommon.h"
#include "../../../../../Common/Graphic/Graphic.h"
#include "../../../../../Common/Framework/framework.h"

#include <AR/ar.h>
#include <AR/gsub.h>
#include <AR/video.h>

#include "opencvgdiplus.h"


#pragma comment(lib, "ARd.lib")
#pragma comment(lib, "ARICPd.lib")
#pragma comment(lib, "ARgsubd.lib")
#pragma comment(lib, "ARvideod.lib")
#pragma comment(lib, "pthreadVC2.lib")


#include <gdiplus.h> 
#pragma comment( lib, "gdiplus.lib" ) 
using namespace Gdiplus;
using namespace graphic;




class cViewer : public framework::cGameMain
{
public:
	cViewer();
	virtual ~cViewer();

	virtual bool OnInit() override;
	virtual void OnUpdate(const float elapseT) override;
	virtual void OnRender(const float elapseT) override;
	virtual void OnShutdown() override;
	virtual void MessageProc(UINT message, WPARAM wParam, LPARAM lParam) override;

	void RenderMat(const cv::Mat &image);


private:
	//cDxCapture m_dxCapture;
	graphic::cCube2 m_cube;
	graphic::cModel m_model;
	graphic::cBillboard m_camBillboard;
	cv::Mat m_camImage;
	cv::Mat m_cvtImage; // 4 channel


	bool m_dbgPrint;
	string m_filePath;
	POINT m_curPos;
	bool m_LButtonDown;
	bool m_RButtonDown;
	bool m_MButtonDown;
	Matrix44 m_rotateTm;

	// GDI plus
	ULONG_PTR m_gdiplusToken;
	GdiplusStartupInput m_gdiplusStartupInput;
};

INIT_FRAMEWORK(cViewer);



#define             CPARA_NAME       "Data/camera_para.dat"
#define             VPARA_NAME       "Data/cameraSetting-%08x%08x.dat"
#define             PATT_NAME        "Data/patt.hiro"

ARHandle           *arHandle;
ARPattHandle       *arPattHandle;
AR3DHandle         *ar3DHandle;
ARGViewportHandle  *vp;
int                 xsize, ysize;
int                 flipMode = 0;
int                 patt_id;
double              patt_width = 80.0;
int                 count = 0;
char                fps[256];
char                errValue[256];
int                 distF = 0;
int                 contF = 0;
ARParamLT          *gCparamLT = NULL;



cViewer::cViewer() 
	: m_model(0)
{
	m_windowName = L"QRCode 3D";
	//const RECT r = { 0, 0, 1024, 768 };
	const RECT r = { 0, 0, 800, 600 };
	m_windowRect = r;

	m_dbgPrint = false;

	m_LButtonDown = false;
	m_RButtonDown = false;
	m_MButtonDown = false;
}

cViewer::~cViewer()
{
	arVideoCapStop();
	argCleanup();
	arPattDetach(arHandle);
	arPattDeleteHandle(arPattHandle);
	ar3DDeleteHandle(&ar3DHandle);
	arDeleteHandle(arHandle);
	arParamLTFree(&gCparamLT);
	arVideoClose();

	Gdiplus::GdiplusShutdown(m_gdiplusToken);
	graphic::ReleaseRenderer();
}


bool cViewer::OnInit()
{

	DragAcceptFiles(m_hWnd, TRUE);

	Gdiplus::GdiplusStartup(&m_gdiplusToken, &m_gdiplusStartupInput, NULL);

	graphic::cResourceManager::Get()->SetMediaDirectory("./");

	m_renderer.GetDevice()->SetRenderState(D3DRS_NORMALIZENORMALS, TRUE);
	m_renderer.GetDevice()->LightEnable(0, true);

	const int WINSIZE_X = 1024;		//초기 윈도우 가로 크기
	const int WINSIZE_Y = 768;	//초기 윈도우 세로 크기
	GetMainCamera()->Init(&m_renderer);
	GetMainCamera()->SetCamera(Vector3(30, 30, -30), Vector3(0, 0, 0), Vector3(0, 1, 0));
	GetMainCamera()->SetProjection(D3DX_PI / 4.f, (float)WINSIZE_X / (float)WINSIZE_Y, 1.f, 10000.0f);

	const Vector3 lightPos(300, 300, -300);
	GetMainLight().SetPosition(lightPos);
	GetMainLight().SetDirection(lightPos.Normal());
	GetMainLight().Bind(m_renderer, 0);
	GetMainLight().Init(cLight::LIGHT_DIRECTIONAL,
		Vector4(0.7f, 0.7f, 0.7f, 0), Vector4(0.2f, 0.2f, 0.2f, 0));


	m_camBillboard.Create(m_renderer, graphic::BILLBOARD_TYPE::ALL_AXIS, 64, 48, 
		Vector3(0, 0, 0), "kim.jpg", false);


// 	if (!m_dxCapture.Init(0, 640, 480, true))
// 	{
// 		return false;
// 	}
// 
//	if (!m_qrCode.Init("recognition_edge_config.cfg"))
// 	{
// 		return false;
// 	}
// 	m_qrCode.m_dbgShow = true;

	const float size = 40.f;
	m_cube.SetCube(m_renderer, Vector3(-size, -size, -size), Vector3(size, size, size));
	Matrix44 t;
	t.SetTranslate(Vector3(0, 0, -40));
	m_cube.SetTransform(t);

	m_model.Create(m_renderer, "cube.dat");


	CGdiPlus::Init();
	m_camImage.create(cvSize(640, 480), CV_8UC3);
	m_cvtImage.create(m_cvtImage.size(), CV_8UC4);


	int argc = 1;
	char *argv[] = { "arsimpledx" };

	glutInit(&argc, argv);

	ARParam         cparam;
	ARGViewport     viewport;
	char            vconf[512];
	AR_PIXEL_FORMAT pixFormat;
	ARUint32        id0, id1;
	int             i;

	if (argc == 1)
	{
		vconf[0] = '\0';
	}
	else 
	{
		strcpy_s(vconf, argv[1]);
		for (i = 2; i < argc; i++)
		{
			strcat_s(vconf, " ");
			strcat_s(vconf, argv[i]);
		}
	}

	// open the video path
	ARLOGi("Using video configuration '%s'.\n", vconf);
	if (arVideoOpen(vconf) < 0) exit(0);
	if (arVideoGetSize(&xsize, &ysize) < 0) exit(0);
	ARLOGi("Image size (x,y) = (%d,%d)\n", xsize, ysize);
	if ((pixFormat = arVideoGetPixelFormat()) < 0) exit(0);
	if (arVideoGetId(&id0, &id1) == 0) {
		ARLOGi("Camera ID = (%08x, %08x)\n", id1, id0);
		sprintf_s(vconf, VPARA_NAME, id1, id0);
		if (arVideoLoadParam(vconf) < 0) {
			ARLOGe("No camera setting data!!\n");
		}
	}

	// set the initial camera parameters
	if (arParamLoad(CPARA_NAME, 1, &cparam) < 0) {
		ARLOGe("Camera parameter load error !!\n");
		exit(0);
	}
	arParamChangeSize(&cparam, xsize, ysize, &cparam);
	ARLOG("*** Camera Parameter ***\n");
	arParamDisp(&cparam);
	if ((gCparamLT = arParamLTCreate(&cparam, AR_PARAM_LT_DEFAULT_OFFSET)) == NULL) {
		ARLOGe("Error: arParamLTCreate.\n");
		exit(-1);
	}

	if ((arHandle = arCreateHandle(gCparamLT)) == NULL) {
		ARLOGe("Error: arCreateHandle.\n");
		exit(0);
	}
	if (arSetPixelFormat(arHandle, pixFormat) < 0) {
		ARLOGe("Error: arSetPixelFormat.\n");
		exit(0);
	}

	if ((ar3DHandle = ar3DCreateHandle(&cparam)) == NULL) {
		ARLOGe("Error: ar3DCreateHandle.\n");
		exit(0);
	}

	if ((arPattHandle = arPattCreateHandle()) == NULL) {
		ARLOGe("Error: arPattCreateHandle.\n");
		exit(0);
	}
	if ((patt_id = arPattLoad(arPattHandle, PATT_NAME)) < 0) {
		ARLOGe("pattern load error !!\n");
		exit(0);
	}
	arPattAttach(arHandle, arPattHandle);

	// open the graphics window 
	// 	int winSizeX, winSizeY;
	// 	argCreateFullWindow();
	// 	argGetScreenSize( &winSizeX, &winSizeY );
	// 	viewport.sx = 0;
	// 	viewport.sy = 0;
	// 	viewport.xsize = winSizeX;
	// 	viewport.ysize = winSizeY;
	viewport.sx = 0;
	viewport.sy = 0;
	viewport.xsize = xsize;
	viewport.ysize = ysize;
	if ((vp = argCreateViewport(&viewport)) == NULL) exit(0);
	argViewportSetCparam(vp, &cparam);
	argViewportSetPixFormat(vp, pixFormat);
	//argViewportSetDispMethod( vp, AR_GL_DISP_METHOD_GL_DRAW_PIXELS );
	argViewportSetDistortionMode(vp, AR_GL_DISTORTION_COMPENSATE_DISABLE);

	if (arVideoCapStart() != 0) {
		ARLOGe("video capture start error !!\n");
		exit(0);
	}

	return true;
}


// 모델뷰 행렬(OpenGL) -> 뷰 행렬(Direct3D)
// http://www.cg-ya.net/imedia/ar/artoolkit_directx/
void D3DConvMatrixView(float* src, D3DXMATRIXA16* dest)
{
	// src의 값을 dest로 1:1 복사를 수행한다.
	dest->_11 = src[0];
	dest->_12 = -src[1];
	dest->_13 = src[2];
	dest->_14 = src[3];

	dest->_21 = src[8];
	dest->_22 = -src[9];
	dest->_23 = src[10];
	dest->_24 = src[7];

	dest->_31 = src[4];
	dest->_32 = -src[5];
	dest->_33 = src[6];
	dest->_34 = src[11];

	dest->_41 = src[12];
	dest->_42 = -src[13];
	dest->_43 = src[14];
	dest->_44 = src[15];
}

// 투영 행렬(OpenGL) -> 투영 행렬(Direct3D)
// http://www.cg-ya.net/imedia/ar/artoolkit_directx/
void D3DConvMatrixProjection(float* src, D3DXMATRIXA16* dest)
{
	dest->_11 = src[0];
	dest->_12 = src[1];
	dest->_13 = src[2];
	dest->_14 = src[3];
	dest->_21 = src[4];
	dest->_22 = src[5];
	dest->_23 = src[6];
	dest->_24 = src[7];
	dest->_31 = -src[8];
	dest->_32 = -src[9];
	dest->_33 = -src[10];
	dest->_34 = -src[11];
	dest->_41 = src[12];
	dest->_42 = src[13];
	dest->_43 = src[14];
	dest->_44 = src[15];
}



void cViewer::OnUpdate(const float elapseT)
{
// 	Mat src(m_dxCapture.GetCloneBufferToImage(false));
// 	if (src.empty())
// 		return;
// 
// 	if (m_qrCode.Detect(src))
// 	{
// 	}
// 
// 	imshow("camera", src);
// 
// 	const int key = cvWaitKey(1);
// 	if (key == 'r')
// 	{
// 		m_qrCode.UpdateConfig("recognition_edge_config.cfg");
//	}

	static int      contF2 = 0;
	static ARdouble patt_trans[3][4];
	static ARUint8 *dataPtr = NULL;
	ARMarkerInfo   *markerInfo;
	int             markerNum;
	ARdouble        err;
	int             imageProcMode;
	int             debugMode;
	int             j, k;

	// grab a video frame
	if ((dataPtr = (ARUint8 *)arVideoGetImage()) == NULL)
	{
		arUtilSleep(2);
		return;
	}

	const size_t sizeInBytes = m_camImage.step[0] * m_camImage.rows;
	memcpy(m_camImage.data, dataPtr, sizeInBytes);
	imshow("window", m_camImage);
	cvWaitKey(1);


// 	if (m_camBillboard.GetTexture())
// 	{
// 		D3DLOCKED_RECT lockR;
// 		m_camBillboard.GetTexture()->Lock(lockR);
// 		if (lockR.pBits)
// 		{
// 			cv::cvtColor(m_camImage, m_cvtImage, CV_BGR2BGRA, 4);
// 			const size_t size = m_cvtImage.step[0] * m_cvtImage.rows;
// 			memcpy(lockR.pBits, m_cvtImage.data, size);
// 			m_camBillboard.GetTexture()->Unlock();
// 		}
// 	}


	argDrawMode2D(vp);
	arGetDebugMode(arHandle, &debugMode);
	if (debugMode == 0)
	{
		argDrawImage(dataPtr);
	}
	else
	{
		arGetImageProcMode(arHandle, &imageProcMode);
		if (imageProcMode == AR_IMAGE_PROC_FRAME_IMAGE)
		{
			argDrawImage(arHandle->labelInfo.bwImage);
		}
		else
		{
			argDrawImageHalf(arHandle->labelInfo.bwImage);
		}
	}

	// detect the markers in the video frame
	if (arDetectMarker(arHandle, dataPtr) < 0)
	{
		//cleanup();
		exit(0);
	}

	if (count % 60 == 0) 
	{
		sprintf_s(fps, "%f[fps]", 60.0 / arUtilTimer());
		arUtilTimerReset();
	}
	count++;
	glColor3f(0.0f, 1.0f, 0.0f);
	argDrawStringsByIdealPos(fps, 10, ysize - 30);

	markerNum = arGetMarkerNum(arHandle);
	if (markerNum == 0) 
	{
		argSwapBuffers();
		return;
	}


	// check for object visibility
	markerInfo = arGetMarker(arHandle);
	k = -1;

	for (j = 0; j < markerNum; j++) 
	{
		ARLOG("ID=%d, CF = %f\n", markerInfo[j].id, markerInfo[j].cf);
		if (patt_id == markerInfo[j].id) 
		{
			if (k == -1) 
			{
				if (markerInfo[j].cf >= 0.7) k = j;
			}
			else if (markerInfo[j].cf > markerInfo[k].cf) 
				k = j;

			if (contF && contF2) 
			{
				err = arGetTransMatSquareCont(ar3DHandle, &(markerInfo[j]), patt_trans, patt_width, patt_trans);
			}
			else 
			{
				err = arGetTransMatSquare(ar3DHandle, &(markerInfo[j]), patt_width, patt_trans);
			}

			contF2 = 1;
			//draw(patt_trans);


			Matrix44 mat;
			for (int i = 0; i < 3; ++i)
			{
				for (int k = 0; k < 3; ++k)
				{
					mat.m[i][k] = patt_trans[i][k];
				}
			}
			mat.Transpose();


			ARdouble gl_para[16];
			argConvGlpara(patt_trans, gl_para);

			float gl_para2[16];
			for (int i = 0; i < 16; ++i)
				gl_para2[i] = gl_para[i];

			Matrix44 viewM;
			D3DConvMatrixView(gl_para2, (D3DXMATRIXA16*)&viewM);

			Matrix44 invView = viewM.Inverse();
			Vector3 lookV = Vector3(1, 0, 0).MultiplyNormal(invView);
			Vector3 upV = Vector3(0, 1, 0).MultiplyNormal(invView);
			upV.Normalize();
			lookV.Normalize();
			const Vector3 eyePos = invView.GetPosition();
			const Vector3 lookAt = lookV * 100.f + eyePos;
// 			GetMainCamera()->SetEyePos(eyePos);
// 			GetMainCamera()->SetUpVector(upV);
// 			GetMainCamera()->SetLookAt(lookAt);

			m_renderer.GetDevice()->SetTransform(D3DTS_VIEW, (D3DMATRIX*)&viewM);

			ARdouble camTrans[3][4];
			arUtilMatInv(patt_trans, camTrans);
			common::dbg::Print(common::format("%f, %f, %f", camTrans[0][3], camTrans[1][3], camTrans[2][3]));

			break;
		}
	}
	if (k == -1) 
	{
		contF2 = 0;
		argSwapBuffers();
		return;
	}


	sprintf_s(errValue, "err = %f", err);
	glColor3f(0.0f, 1.0f, 0.0f);
	argDrawStringsByIdealPos(fps, 10, ysize - 30);
	argDrawStringsByIdealPos(errValue, 10, ysize - 60);
	//ARLOG("err = %f\n", err);

	argSwapBuffers();
}


void cViewer::RenderMat(const cv::Mat &image)
{
	using namespace Gdiplus;

	RET(image.empty());

// 	//------------------------------------------------
// 	const int t = timeGetTime();
// 	if (m_lastRenderTime == 0)
// 		m_lastRenderTime = t;
// 	if (t - m_lastRenderTime < 33) // 30 frame
// 		return;
// 	m_lastRenderTime = t;
// 	//------------------------------------------------
	static cv::Size m_imageSize;
	static Gdiplus::Bitmap *m_bitmap = NULL;

	cv::Size size = image.size();
	if (m_imageSize == size)
	{
		CGdiPlus::CopyMatToBmp(image, m_bitmap);
	}
	else
	{
		SAFE_DELETE(m_bitmap);
		m_bitmap = CGdiPlus::CopyMatToBmp(image);
	}

	HWND hWnd = m_renderer.GetHwnd();
	RECT r;
	GetClientRect(hWnd, &r);
	HDC hdc = GetDC(hWnd);
	Gdiplus::Graphics g(hdc);
	g.DrawImage(m_bitmap, Gdiplus::Rect(0, 0, r.right, r.bottom));
	ReleaseDC(hWnd, hdc);
}


void cViewer::OnRender(const float elapseT)
{
	if (m_renderer.ClearScene())
	{
		m_renderer.BeginScene();

		GetMainLight().Bind(m_renderer, 0);

		m_renderer.RenderGrid();
		m_renderer.RenderAxis();
		//m_camBillboard.Render(m_renderer);

		m_cube.Render(m_renderer, Matrix44::Identity);
		//m_model.Render(m_renderer, Matrix44::Identity);

		m_renderer.RenderFPS();

		m_renderer.EndScene();
		m_renderer.Present();
	}
}


void cViewer::OnShutdown()
{
}


void cViewer::MessageProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_DROPFILES:
	{
		HDROP hdrop = (HDROP)wParam;
		char filePath[MAX_PATH];
		const UINT size = DragQueryFileA(hdrop, 0, filePath, MAX_PATH);
		if (size == 0)
			return;// handle error...

		m_filePath = filePath;

		const graphic::RESOURCE_TYPE::TYPE type = graphic::cResourceManager::Get()->GetFileKind(filePath);
		switch (type)
		{
		case graphic::RESOURCE_TYPE::MESH:
			break;

		case graphic::RESOURCE_TYPE::ANIMATION:
			break;
		}
	}
	break;

	case WM_MOUSEWHEEL:
	{
		int fwKeys = GET_KEYSTATE_WPARAM(wParam);
		int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
		dbg::Print("%d %d", fwKeys, zDelta);

		const float len = graphic::GetMainCamera()->GetDistance();
		float zoomLen = (len > 100) ? 50 : (len / 4.f);
		if (fwKeys & 0x4)
			zoomLen = zoomLen / 10.f;

		graphic::GetMainCamera()->Zoom((zDelta < 0) ? -zoomLen : zoomLen);
	}
	break;

	case WM_KEYDOWN:
		switch (wParam)
		{
		case VK_F5: // Refresh
		{
			if (m_filePath.empty())
				return;
		}
		break;
		case VK_BACK:
			// 회전 행렬 초기화.
			m_dbgPrint = !m_dbgPrint;
			break;
		case VK_TAB:
		{
			static bool flag = false;
			m_renderer.GetDevice()->SetRenderState(D3DRS_CULLMODE, flag ? D3DCULL_CCW : D3DCULL_NONE);
			m_renderer.GetDevice()->SetRenderState(D3DRS_FILLMODE, flag ? D3DFILL_SOLID : D3DFILL_WIREFRAME);
			flag = !flag;
		}
		break;
		}
		break;

	case WM_LBUTTONDOWN:
	{
		m_LButtonDown = true;
		m_curPos.x = LOWORD(lParam);
		m_curPos.y = HIWORD(lParam);
	}
	break;

	case WM_LBUTTONUP:
		m_LButtonDown = false;
		break;

	case WM_RBUTTONDOWN:
	{
		m_RButtonDown = true;
		m_curPos.x = LOWORD(lParam);
		m_curPos.y = HIWORD(lParam);
	}
	break;

	case WM_RBUTTONUP:
		m_RButtonDown = false;
		break;

	case WM_MBUTTONDOWN:
		m_MButtonDown = true;
		m_curPos.x = LOWORD(lParam);
		m_curPos.y = HIWORD(lParam);
		break;

	case WM_MBUTTONUP:
		m_MButtonDown = false;
		break;

	case WM_MOUSEMOVE:
	{
		if (m_LButtonDown)
		{
			POINT pos = { LOWORD(lParam), HIWORD(lParam) };
			const int x = pos.x - m_curPos.x;
			const int y = pos.y - m_curPos.y;
			m_curPos = pos;

			Quaternion q1(graphic::GetMainCamera()->GetRight(), -y * 0.01f);
			Quaternion q2(graphic::GetMainCamera()->GetUpVector(), -x * 0.01f);

			m_rotateTm *= (q2.GetMatrix() * q1.GetMatrix());
		}
		else if (m_RButtonDown)
		{
			POINT pos = { LOWORD(lParam), HIWORD(lParam) };
			const int x = pos.x - m_curPos.x;
			const int y = pos.y - m_curPos.y;
			m_curPos = pos;

			//if (GetAsyncKeyState('C'))
			{
				graphic::GetMainCamera()->Yaw2(x * 0.005f);
				graphic::GetMainCamera()->Pitch2(y * 0.005f);
			}
		}
		else if (m_MButtonDown)
		{
			const POINT point = { LOWORD(lParam), HIWORD(lParam) };
			const POINT pos = { point.x - m_curPos.x, point.y - m_curPos.y };
			m_curPos = point;

			const float len = graphic::GetMainCamera()->GetDistance();
			graphic::GetMainCamera()->MoveRight(-pos.x * len * 0.001f);
			graphic::GetMainCamera()->MoveUp(pos.y * len * 0.001f);
		}
		else
		{
		}

	}
	break;
	}
}

