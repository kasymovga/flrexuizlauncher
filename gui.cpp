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
};

GUI::GUI() {
	priv = new GUIPrivate;
	priv->window = new Fl_Window(340, 140);
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
	return fl_choice(question, "No", "Yes", 0) == 1;
}

void GUI::error(const char *error) {
	fl_alert(error);
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
	char *directory = fl_dir_chooser(question, "", 0);
	if (!directory) return NULL;
	return FS::fromUTF8(directory);
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
