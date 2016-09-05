//
//
#pragma once


namespace ar
{
	class cQRCode;

	class cQRmap
	{
	public:
		cQRmap();
		virtual ~cQRmap();

		void Init(graphic::cRenderer &renderer);
		void Update(graphic::cRenderer &renderer, ARUint8 *dataPtr);
		void GetCameraPos();
		void Render(graphic::cRenderer &renderer);


	public:
		vector<ar::cQRCode*> m_qrMap;
		sQRPos m_codes[64];
		vector<Vector3> m_qrTablePos; // 64°³
		int m_nearQRCodeId;
		int m_detectCubeCount;
		double m_patt_width;

		graphic::cCube2 m_cube;
		graphic::cModel m_model;
		graphic::cText *m_text;
		graphic::cLine2d m_line;
	};

}
