#ifndef _FLRL_TRANSLATION_H_
#define _FLRL_TRANSLATION_H_
class Translation {
public:
	Translation(const char *translationData, int translationDataLength, const char *lang);
	~Translation();
	const char *translate(const char *text);
private:
	class Link;
	Link **hashTable;
	int hashTableSize;
};
#endif
