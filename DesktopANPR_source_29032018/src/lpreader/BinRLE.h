#ifndef _BINRLE_H_INCLUDED_
#define _BINRLE_H_INCLUDED_

#include <vector>

class CNetImage;

class CBinRLE {
private:
	struct RLERunS {
		unsigned int startx_;
		unsigned int starty_;
		unsigned int runlength_;

		RLERunS(unsigned int _startx, unsigned int _starty,
				unsigned int _runlength) :
				startx_(_startx), starty_(_starty), runlength_(_runlength) {
		}
		RLERunS() {
		}
	};

	std::vector<RLERunS> *runs_;
	std::vector<RLERunS> *tempruns_;

	unsigned int nextrunindex_;

public:
	CBinRLE();
	~CBinRLE();

	void CreateFromImage(CNetImage *_image, unsigned char _threshold);
	void CreateFullImage(unsigned int _width, unsigned int _height);

	void Clear();
	void CopyFrom(CBinRLE *_t);
	void AddRun(unsigned int _startx, unsigned int _starty,
			unsigned int _runlength);

	void ExpandHorizontal(unsigned int _imagewidth);
	void ExpandVertical(unsigned int _imageheight);

	void StartRun();
	bool GetNextRun(unsigned int *_startx, unsigned int *_starty,
			unsigned int *_runlength);
};

#endif 
