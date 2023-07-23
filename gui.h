#ifndef _FLRL_GUI_H_
#define _FLRL_GUI_H_

#include "fs.h"
#include "launcher.h"

class Launcher;
class GUI {
public:
	GUI(Launcher *launcher);
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
	void watchfd(long int fd, void callback(long int fd, void *callback_data), void *callback_data);
	void unwatchfd(long int fd);
private:
	class GUIPrivate;
	GUIPrivate *priv;
	Launcher *launcher;
};

#endif
