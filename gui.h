#ifndef _FLRL_GUI_H_
#define _FLRL_GUI_H_

#include "fs.h"

class GUI {
public:
	GUI();
	~GUI();
	bool show();
	void hide();
	bool frame();
	bool wait();
	void setInfo(const char* text);
	void setInfoSecondary(const char* text);
	void setProgress(int p);
	void setProgressSecondary(int p);
	FSChar *selectDirectory(const char *question, const FSChar *start);
	bool askYesNo(const char *choice);
	void error(const char *error);
private:
	class GUIPrivate;
	GUIPrivate *priv;
};

#endif
