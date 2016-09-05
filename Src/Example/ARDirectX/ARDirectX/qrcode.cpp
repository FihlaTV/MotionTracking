
#include "stdafx.h"
#include "qrcode.h"


using namespace graphic;
using namespace ar;


cQRCode::cQRCode()
	: m_model(NULL)
	, m_text(NULL)
{

}

cQRCode::~cQRCode()
{
}


void cQRCode::Init(cRenderer &renderer, cModel *model, graphic::cText *text)
{
	m_model = model;
	m_text = text;
}


void cQRCode::Render(cRenderer &renderer)
{
	if (m_model)
		m_model->Render(renderer, Matrix44::Identity);
	if (m_text)
		m_text->Render();
}
