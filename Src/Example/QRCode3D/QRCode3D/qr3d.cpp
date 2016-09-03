
#ifndef _CRT_SECURE_NO_WARNINGS
	#define _CRT_SECURE_NO_WARNINGS
#endif


#include "../../../../../Common/Common/Common.h"
#include "../../../../../Common/Network/Network.h"
#include "../../../../../Common/CamCommon/CamCommon.h"
#include "../../../../../Common/Graphic/Graphic.h"
#include "../../../../../Common/Framework/framework.h"

#include "../aruco-1.3.0/src/marker.h"

#include <objidl.h>
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


private:
	cDxCapture m_dxCapture;
	cvproc::cArQRCode m_qrCode;
	graphic::cCube2 m_cube;
	aruco::Marker m_marker;


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


cViewer::cViewer()
{
	m_windowName = L"QRCode 3D";
	//const RECT r = { 0, 0, 1024, 768 };
	const RECT r = { 0, 0, 800, 600};
	m_windowRect = r;

	m_dbgPrint = false;

	m_LButtonDown = false;
	m_RButtonDown = false;
	m_MButtonDown = false;
}

cViewer::~cViewer()
{
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


	if (!m_dxCapture.Init(0, 640, 480, true))
	{
		return false;
	}

	if (!m_qrCode.Init("recognition_edge_config.cfg"))
	{
		return false;
	}
	m_qrCode.m_dbgShow = true;

	m_cube.SetCube(m_renderer, Vector3(-1, -1, -1), Vector3(1, 1, 1));

	return true;
}


void cViewer::OnUpdate(const float elapseT)
{
	Mat src(m_dxCapture.GetCloneBufferToImage(false));
	if (src.empty())
		return;

	if (m_qrCode.Detect(src))
	{
		//calibrateCamera()

// 		Matrix44 tm;
// 
// 		tm._11 = 0.439f;
// 		tm._12 = -0.103f;
// 		tm._13 = 0;
// 		tm._14 = -0.0004f;
// 
// 		tm._21 = -0.5656f;
// 		tm._22 = 0.704f;
// 		tm._23 = 0;
// 		tm._24 = -0.001f;
// 
// 		tm._31 = 0;
// 		tm._32 = 0;
// 		tm._33 = 1;
// 		tm._34 = 0;
// 
// 		tm._41 = 0;
// 		tm._42 = 0;
// 		tm._43 = 0;
// 		tm._44 = 1;
// 
// 		m_cube.SetTransform(tm);

// 		Mat &m = m_qrCode.m_skewDetect.m_transmtx;
// 		float *p = (float*)m.data;
// 		tm._11 = p[0 * m.step1() + 0];
// 		tm._12 = p[0 * m.step1() + 1];
// 		tm._13 = p[0 * m.step1() + 2];
// 
// 		tm._21 = p[1 * m.step1() + 0];
// 		tm._22 = p[1 * m.step1() + 1];
// 		tm._23 = p[1 * m.step1() + 2];
// 
// 		tm._31 = p[2 * m.step1() + 0];
// 		tm._32 = p[2 * m.step1() + 1];
// 		tm._33 = p[2 * m.step1() + 2];
// 		tm = tm.Inverse();

		//tm._11 = m.data[ m.s]

// 		float * data = (float*)m_qrCode.m_skewDetect.m_transmtx.data;
// 		data[0 * m_qrCode.m_skewDetect.m_transmtx.step + 0];
// 		tm._11 = m_qrCode.m_skewDetect.m_transmtx.at<float>(0, 0);
// 		tm._12 = m_qrCode.m_skewDetect.m_transmtx.at<float>(0, 1);
// 		tm._12 = m_qrCode.m_skewDetect.m_transmtx.at<float>(0, 2);
//		m_cube.SetTransform(tm);

	}

	imshow("camera", src);

	const int key = cvWaitKey(1);
	if (key == 'r')
	{
		m_qrCode.UpdateConfig("recognition_edge_config.cfg");
	}
}


void cViewer::OnRender(const float elapseT)
{
	if (m_renderer.ClearScene())
	{
		m_renderer.BeginScene();

		GetMainLight().Bind(m_renderer, 0);

		m_renderer.RenderGrid();
		m_renderer.RenderAxis();

		m_cube.Render(m_renderer, Matrix44::Identity);

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

