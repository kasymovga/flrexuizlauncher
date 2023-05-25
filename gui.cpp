#include "gui.h"
#include "rexuiz_logo.h"
#include "rexuiz_icon.h"

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Image.H>
#include <FL/Fl_PNG_Image.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Progress.H>
#include <FL/fl_ask.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/Fl_Native_File_Chooser.H>

class GUI::GUIPrivate {
public:
	Fl_Window *window;
	Fl_Box *logo;
	Fl_Box *info;
	Fl_Box *infoSecondary;
	Fl_Progress *progress;
	Fl_Progress *progressSecondary;
	Fl_Image *logoImage;
	Fl_RGB_Image *icon;
	Fl_Native_File_Chooser *directoryChooser;
};

static void gui_close_callback(Fl_Widget *widget, void *p) {
	Launcher *launcher = (Launcher *)p;
	if (fl_choice("%s", "No", "Yes", 0, "Quit now?") == 1) {
		launcher->abort();
		widget->hide();
	}
}

GUI::GUI(Launcher *launcher) {
	priv = new GUIPrivate;
	priv->window = new Fl_Window(340, 140, "RexuizLauncher");
	priv->window->position((Fl::w() - priv->window->decorated_w()) / 2, (Fl::h() - priv->window->decorated_h()) / 2);
	priv->icon = new Fl_PNG_Image("icon", rexuiz_icon, sizeof(rexuiz_icon));
	priv->window->icon(priv->icon);
	priv->logo = new Fl_Box(10, 10, 320, 30, "");
	priv->logo->align(FL_ALIGN_IMAGE_BACKDROP);
	priv->logo->box(FL_NO_BOX);
	priv->logoImage = new Fl_PNG_Image("logo", rexuiz_logo, sizeof(rexuiz_logo));
	priv->logo->image(priv->logoImage);
	priv->info = new Fl_Box(10, 40, 320, 20, "");
	priv->info->box(FL_NO_BOX);
	priv->progress = new Fl_Progress(10, 60, 320, 20);
	priv->progress->maximum(100);
	priv->progress->minimum(0);
	priv->infoSecondary = new Fl_Box(10, 80, 320, 20, "");
	priv->infoSecondary->box(FL_NO_BOX);
	priv->progressSecondary = new Fl_Progress(10, 100, 320, 20);
	priv->progressSecondary->maximum(100);
	priv->progressSecondary->minimum(0);
	priv->window->end();
	priv->window->callback(gui_close_callback, launcher);
	priv->directoryChooser = new Fl_Native_File_Chooser();
	priv->directoryChooser->type(Fl_Native_File_Chooser::BROWSE_DIRECTORY);
}

bool GUI::show() {
	if (!priv->window)
		return false;

	priv->window->show();
	return true;
}

void GUI::hide() {
	if (priv->window)
		priv->window->hide();
}

bool GUI::wait() {
	return Fl::wait();
}

bool GUI::frame() {
	Fl::flush();
	return Fl::wait(0);
}

bool GUI::askYesNo(const char *question) {
	return fl_choice("%s", "No", "Yes", 0, question) == 1;
}

void GUI::error(const char *error) {
	fl_alert("%s", error);
}

void GUI::setInfo(const char *text) {
	priv->info->label(text);
	frame();
}

void GUI::setInfoSecondary(const char *text) {
	priv->infoSecondary->label(text);
	frame();
}

FSChar *GUI::selectDirectory(const char *question, const FSChar *start) {
	const char *startDirectory = FS::toUTF8(start);
	priv->directoryChooser->title(question);
	priv->directoryChooser->directory(startDirectory);
	switch (priv->directoryChooser->show()) {
	case -1: error("Error with directory selection"); return NULL; //Error
	case 1: return NULL; //Cancel
	}
	delete [] startDirectory;
	return FS::fromUTF8(priv->directoryChooser->filename());
}

GUI::~GUI() {
	delete priv->logoImage;
	delete priv->logo;
	delete priv->info;
	delete priv->infoSecondary;
	delete priv->progress;
	delete priv->progressSecondary;
	delete priv->icon;
	delete priv->window;
	delete priv->directoryChooser;
	delete priv;
}

void GUI::setProgress(int p) {
	priv->progress->value(p);
	frame();
}

void GUI::setProgressSecondary(int p) {
	priv->progressSecondary->value(p);
	frame();
}

void GUI::watchfd(int fd, void callback(int fd, void *callback_data), void *callback_data) {
	Fl::add_fd(fd, FL_READ | FL_EXCEPT, callback, callback_data);
}

void GUI::unwatchfd(int fd) {
	Fl::remove_fd(fd);
}
