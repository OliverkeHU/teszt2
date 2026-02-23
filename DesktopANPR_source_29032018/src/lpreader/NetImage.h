#ifndef _NETIMAGE_H_INCLUDED_
#define _NETIMAGE_H_INCLUDED_

class CNetImage {
private:
	unsigned int allocatedmemsize_;
	bool CheckAllocation(unsigned int _width, unsigned int _height,
			unsigned int _scanline);

public:
	CNetImage();
	~CNetImage();

	bool Create(unsigned int _width, unsigned int _height);
	bool CreateMap(unsigned int _width, unsigned int _height,
			unsigned int _scanline, const unsigned char *_imgdata);

	void FillImage(unsigned char _value);

	void Free();

	unsigned char *imgdata_;
	unsigned int width_;
	unsigned int height_;
	unsigned int scanline_;
	bool ismapped_;

	inline unsigned char *GetPtr(unsigned int _x, unsigned int _y) {
		return imgdata_ + scanline_ * _y + _x;
	}
};

#endif
