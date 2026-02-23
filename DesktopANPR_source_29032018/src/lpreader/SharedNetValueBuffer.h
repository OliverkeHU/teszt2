#ifndef _SHAREDNETVALUEBUFFER_H_INCLUDED_
#define _SHAREDNETVALUEBUFFER_H_INCLUDED_
#include "nettypes.h"

class CSharedNetValueBuffer {
private:
	unsigned int allocationsize_;
	f16_16* buffer_;

public:
	CSharedNetValueBuffer();
	~CSharedNetValueBuffer();

	unsigned int SetAllocationSize(unsigned int _valuecount);
	f16_16* GetBuffer(unsigned int _offset);

	inline void CopyImageWithAVG(uint8_t* _imagedata, unsigned int _imgwidth,
			unsigned int _imgheight, unsigned int _scanline, uint8_t _avg1,
			uint8_t _avg2, uint8_t _avg3) {

		f16_16 *vbuff = buffer_;
		uint8_t* cursl = _imagedata;
		for (unsigned int y = 0; y < _imgheight; y++) {
			uint8_t* curpixel = cursl;
			for (unsigned int x = 0; x < _imgwidth; x++) {
				uint8_t pixdata = *curpixel++;
				*vbuff++ = pixdata;
			}
			cursl += _scanline;
		}
		*vbuff++ = _avg1;
		*vbuff++ = _avg2;
		*vbuff++ = _avg3;
	}

	void CopyImage(uint8_t* _imagedata, unsigned int _imgwidth,
			unsigned int _imgheight, unsigned int _scanline);
	void CopyValues(unsigned int _bufferindex, unsigned char *_data,
			unsigned int _count);

	void Reset();
};

#endif 
