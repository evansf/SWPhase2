#include "Retraindlg.h"

CRetrainDlg::CRetrainDlg(QWidget *parent)
	: QDialog(parent)
{
	ui.setupUi(this);
	QRect ParentRect = parent->geometry();
	setGeometry(ParentRect.x(),ParentRect.height(),width(),height());
	connect(ui.FlawButton,SIGNAL(clicked()), this, SLOT(On_FlawButton_Clicked()));
	connect(ui.NormalButton,SIGNAL(clicked()), this, SLOT(On_NormalButton_Clicked()));
	connect(ui.AbortButton,SIGNAL(clicked()), this, SLOT(On_AbortButton_Clicked()));
}

CRetrainDlg::~CRetrainDlg()
{
}
void CRetrainDlg::On_NormalButton_Clicked()
{
	done(NORMAL);
}
void CRetrainDlg::On_FlawButton_Clicked()
{
	done(FLAW);
}
void CRetrainDlg::On_AbortButton_Clicked()
{
	done(ABORT);
}

