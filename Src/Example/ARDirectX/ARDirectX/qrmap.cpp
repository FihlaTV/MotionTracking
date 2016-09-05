
#include "stdafx.h"
#include "qrmap.h"


using namespace ar;
using namespace graphic;


cQRmap::cQRmap()
	: m_patt_width(80.0f)
	, m_model(0)
	, m_text(NULL)
	, m_detectCubeCount(0)
	, m_nearQRCodeId(-1)
	, m_line(4)
{

}

cQRmap::~cQRmap()
{
	for each(auto &qr in m_qrMap)
		delete qr;
	m_qrMap.clear();

	SAFE_DELETE(m_text);
}


void cQRmap::Init(graphic::cRenderer &renderer)
{
	const float size = 20.f;
	m_cube.SetCube(renderer, Vector3(-size, -size, -size), Vector3(size, size, size));
	m_cube.GetMaterial().InitGreen();

	m_model.Create(renderer, "cube.dat");

	Matrix44 t;
	t.SetTranslate(Vector3(0, size, 0));
	Matrix44 m;
	m.SetScale(Vector3(5, 5, 5));
	m_model.SetTransform(m*t);

	m_text = new graphic::cText(renderer, "", 0, 0);
}


void cQRmap::Update(graphic::cRenderer &renderer, ARUint8 *dataPtr)
{
	const int contF = 0;
	static int contF2 = 0;
	static ARdouble patt_trans[3][4];
	ARdouble err;


	// detect the markers in the video frame
	if (arDetectMarker(cAREngine::Get()->m_arHandle, dataPtr) < 0)
	{
		//cleanup();
		return;
	}

	const int markerNum = arGetMarkerNum(cAREngine::Get()->m_arHandle);
	if (markerNum == 0)
	{
		argSwapBuffers();
		return;
	}

	// check for object visibility
	ARMarkerInfo *markerInfo = arGetMarker(cAREngine::Get()->m_arHandle);
	int k = -1;

	int detectCount = 0;

	for (int j = 0; j < markerNum; j++)
	{
		//ARLOG("ID=%d, CF = %f\n", markerInfo[j].id, markerInfo[j].cf);
		dbg::Print("ID=%d, CF = %f\n", markerInfo[j].id, markerInfo[j].cf);

		if (markerInfo[j].id >= 0)
		{
			if (k == -1)
			{
				if (markerInfo[j].cf >= 0.7) k = j;
			}
			else if (markerInfo[j].cf > markerInfo[k].cf)
			{
				k = j;
			}

			if (contF && contF2)
			{
				err = arGetTransMatSquareCont(cAREngine::Get()->m_ar3DHandle, &(markerInfo[j]), patt_trans, m_patt_width, patt_trans);
			}
			else
			{
				err = arGetTransMatSquare(cAREngine::Get()->m_ar3DHandle, &(markerInfo[j]), m_patt_width, patt_trans);
			}

			contF2 = 1;

			ARdouble gl_para[16];
			argConvGlpara(patt_trans, gl_para);

			float gl_para2[16];
			for (int i = 0; i < 16; ++i)
				gl_para2[i] = gl_para[i];

			Matrix44 viewM;
			D3DConvMatrixView(gl_para2, (D3DXMATRIXA16*)&viewM);

			if ((int)m_qrMap.size() <= detectCount)
			{
				cQRCode *code = new cQRCode();
				code->Init(renderer, &m_model, m_text);
				code->m_id = markerInfo[j].id;
				code->m_tm = viewM;
				m_qrMap.push_back(code);
			}
			else
			{
				m_qrMap[detectCount]->m_id = markerInfo[j].id;
				m_qrMap[detectCount]->m_tm = viewM;
			}

			++detectCount;
		}
	}

	m_detectCubeCount = detectCount;
}


void cQRmap::GetCameraPos()
{

}


void cQRmap::Render(graphic::cRenderer &renderer)
{

	//----------------------------------------------------------------------------------------------------
	// QRCode 격자무늬 출력
	//sQRPos m_codes[64];
	bool codeEdge[64][64];
	const int startId = 0;

	ZeroMemory(m_codes, sizeof(m_codes));
	ZeroMemory(codeEdge, sizeof(codeEdge));

	// QRCode 스크린좌표 계산
	for (int i = 0; i < m_detectCubeCount; ++i)
	{
		const int id = m_qrMap[i]->m_id;
		m_codes[id].used = true;

		GetMainCamera()->SetViewMatrix(m_qrMap[i]->m_tm);
		renderer.GetDevice()->SetTransform(D3DTS_VIEW, (D3DMATRIX*)&m_qrMap[i]->m_tm);
		Vector3 textPos = GetMainCamera()->GetScreenPos(640, 480, Vector3(0, 0, 0));
		m_codes[id].pos = cv::Point((int)textPos.x, (int)textPos.y);
	}

	//----------------------------------------------------------------------------------------------------
	// QRCode Cube 출력
	for (int i = 0; i < m_detectCubeCount; ++i)
	{
		GetMainCamera()->SetViewMatrix(m_qrMap[i]->m_tm);
		renderer.GetDevice()->SetTransform(D3DTS_VIEW, (D3DMATRIX*)&m_qrMap[i]->m_tm);
		const Vector3 textPos = GetMainCamera()->GetScreenPos(640, 480, Vector3(0, 0, 0));

		const int id = m_qrMap[i]->m_id;
		const DWORD color = (m_nearQRCodeId == id) ? D3DXCOLOR(1, 0, 0, 1) : D3DXCOLOR(1, 1, 0, 1);
		m_qrMap[i]->m_text->SetText((int)textPos.x, (int)textPos.y, common::format("%d", id));
		m_qrMap[i]->m_text->SetColor(color);

		m_qrMap[i]->Render(renderer);
	}	
}
