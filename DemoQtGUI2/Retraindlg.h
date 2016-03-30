#ifndef RETRAINDLG_H
#define RETRAINDLG_H

#include <QDialog>

#include "ui_RetrainDlg.h"
using namespace std;

class CRetrainDlg : public QDialog
{
	Q_OBJECT
public:
	typedef enum returnType
	{
		NORMAL,
		FLAW,
		ABORT
	} RETURNVALUE;
public:

public:
	CRetrainDlg(QWidget *parent = 0);
	~CRetrainDlg();
	Ui::RetrainDlg ui;

public slots:
	void On_NormalButton_Clicked();
	void On_FlawButton_Clicked();
	void On_AbortButton_Clicked();

private:

};

#endif // RETRAINDLG_H
