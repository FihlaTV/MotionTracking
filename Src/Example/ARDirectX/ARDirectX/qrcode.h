//
//

#pragma once


namespace ar
{

	class cQRCode
	{
	public:
		cQRCode();
		virtual ~cQRCode();

		void Init(graphic::cRenderer &renderer, graphic::cModel *model, graphic::cText *text);
		void Render(graphic::cRenderer &renderer);


	public:
		int m_id; // QRCode ID 
		Matrix44 m_tm;	// QRCode를 인식한 후, 3차원 상에서의 카메라 행렬

		// display
		graphic::cModel *m_model; // reference
		graphic::cText *m_text; // reference
	};

}
