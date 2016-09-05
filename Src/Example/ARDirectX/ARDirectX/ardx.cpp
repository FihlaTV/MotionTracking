
#include "stdafx.h"

using namespace cv;


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
	graphic::cCube2 m_cube;
	graphic::cModel m_model;
	graphic::cLine2d m_line;
	LPD3DXSPRITE m_sprite;
	graphic::cSprite *m_videoSprite;
	cv::Mat m_camImage;
	cv::Mat m_cvtImage; // 4 channel

	ar::cQRmap m_qrmap;

	bool m_skipCamera;

	bool m_dbgPrint;
	string m_filePath;
	POINT m_curPos;
	bool m_LButtonDown;
	bool m_RButtonDown;
	bool m_MButtonDown;
	Matrix44 m_rotateTm;
};

INIT_FRAMEWORK(cViewer);




cViewer::cViewer()
	: m_model(0)
	, m_line(4)
{
	m_windowName = L"ARToolkit With DirectX Test";
	const RECT r = { 0, 0, 640, 480 };
	m_windowRect = r;

	m_dbgPrint = false;

	m_LButtonDown = false;
	m_RButtonDown = false;
	m_MButtonDown = false;
}


cViewer::~cViewer()
{
	ar::cAREngine::Release();

	SAFE_DELETE(m_videoSprite);
	m_sprite->Release();

	graphic::ReleaseRenderer();
}


bool cViewer::OnInit()
{
	graphic::cResourceManager::Get()->SetMediaDirectory("./");

	m_renderer.GetDevice()->SetRenderState(D3DRS_NORMALIZENORMALS, TRUE);
	m_renderer.GetDevice()->LightEnable(0, true);

	const int WINSIZE_X = 1024;		//초기 윈도우 가로 크기
	const int WINSIZE_Y = 768;	//초기 윈도우 세로 크기
	graphic::GetMainCamera()->Init(&m_renderer);
	graphic::GetMainCamera()->SetCamera(Vector3(30, 30, -30), Vector3(0, 0, 0), Vector3(0, 1, 0));
	graphic::GetMainCamera()->SetProjection(D3DX_PI / 4.f, (float)WINSIZE_X / (float)WINSIZE_Y, 1.f, 10000.0f);

	const Vector3 lightPos(300, 300, -300);
	graphic::GetMainLight().SetPosition(lightPos);
	graphic::GetMainLight().SetDirection(lightPos.Normal());
	graphic::GetMainLight().Bind(m_renderer, 0);
	graphic::GetMainLight().Init(graphic::cLight::LIGHT_DIRECTIONAL,
		Vector4(0.7f, 0.7f, 0.7f, 0), Vector4(0.2f, 0.2f, 0.2f, 0));

	D3DXCreateSprite(m_renderer.GetDevice(), &m_sprite);
	m_videoSprite = new graphic::cSprite(m_sprite, 0);
	m_videoSprite->SetTexture(m_renderer, "kim.jpg");
	m_videoSprite->SetPos(Vector3(0, 0, 0));

	m_skipCamera = false;

	const float size = 20.f;
	m_cube.SetCube(m_renderer, Vector3(-size, -size, -size), Vector3(size, size, size));
	m_cube.GetMaterial().InitGreen();

	Matrix44 t;
	t.SetTranslate(Vector3(0, size, 0));
	m_cube.SetTransform(t);

	m_model.Create(m_renderer, "cube.dat");
	Matrix44 m;
	m.SetScale(Vector3(5, 5, 5));
	m_model.SetTransform(m*t);

	m_camImage.create(cvSize(640, 480), CV_8UC3);
	m_cvtImage.create(m_cvtImage.size(), CV_8UC4);


	if (!ar::cAREngine::Get()->Init(__argc, __argv))
		return false;

	m_qrmap.Init(m_renderer);


	//--------------------------------------------------------------------------------------------------------------
	// DirectX Perspective Setting
#define VIEW_DISTANCE_MIN		40.0        // Objects closer to the camera than this will not be displayed. OpenGL units.
#define VIEW_DISTANCE_MAX		10000.0     // Objects further away from the camera than this will not be displayed. OpenGL units.
	ARdouble p[16];
	arglCameraFrustumRH(&(ar::cAREngine::Get()->m_CparamLT->param), VIEW_DISTANCE_MIN, VIEW_DISTANCE_MAX, p);
	float gl_para3[16];
	for (int i = 0; i < 16; ++i)
		gl_para3[i] = p[i];

	Matrix44 projM;
	ar::D3DConvMatrixProjection(gl_para3, (D3DXMATRIXA16*)&projM);
	graphic::GetMainCamera()->SetProjectionMatrix(projM);
	m_renderer.GetDevice()->SetTransform(D3DTS_PROJECTION, (D3DMATRIX*)&projM);

	return true;
}


void cViewer::OnUpdate(const float elapseT)
{
	if (m_skipCamera)
		return;

	static ARUint8 *dataPtr = NULL;

	// grab a video frame
	if ((dataPtr = (ARUint8 *)arVideoGetImage()) == NULL)
	{
		arUtilSleep(2);
		return;
	}


	// display camera image to DirectX Texture
	D3DLOCKED_RECT lockRect;
	m_videoSprite->GetTexture()->Lock(lockRect);
	if (lockRect.pBits)
	{
		const size_t sizeInBytes = m_camImage.step[0] * m_camImage.rows;
		memcpy(m_camImage.data, dataPtr, sizeInBytes);

		cv::Mat RGBA(m_camImage.size(), CV_8UC4);
		cv::cvtColor(m_camImage, RGBA, CV_BGR2BGRA, 4);
		const size_t sizeInBytes2 = RGBA.step[0] * RGBA.rows;
		memcpy(lockRect.pBits, RGBA.data, sizeInBytes2);
		m_videoSprite->GetTexture()->Unlock();
	}

	// Update QR Map
	m_qrmap.Update(m_renderer, dataPtr);
}


void cViewer::OnRender(const float elapseT)
{
	if (m_renderer.ClearScene())
	{
		m_renderer.BeginScene();

		graphic::GetMainLight().Bind(m_renderer, 0);

		m_renderer.GetDevice()->SetRenderState(D3DRS_ZENABLE, 0);
		m_videoSprite->Render(m_renderer, Matrix44::Identity);
		m_renderer.GetDevice()->SetRenderState(D3DRS_ZENABLE, 1);

//		m_renderer.RenderGrid();
//		m_renderer.RenderAxis();
		if (!m_skipCamera)
			m_qrmap.Render(m_renderer);

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
		case VK_SPACE:
		{
			m_skipCamera = !m_skipCamera;
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


