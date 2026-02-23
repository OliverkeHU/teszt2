#include "SharedNetValueBuffer.h"

CSharedNetValueBuffer::CSharedNetValueBuffer() {
	allocationsize_ = 0;
	buffer_ = nullptr;
}

CSharedNetValueBuffer::~CSharedNetValueBuffer() {
	Reset();
}

void CSharedNetValueBuffer::Reset() {

	if (nullptr != buffer_) {
		delete[] buffer_;
		buffer_ = nullptr;
		allocationsize_ = 0;
	}
}

unsigned int CSharedNetValueBuffer::SetAllocationSize(
		unsigned int _valuecount) {

	allocationsize_ += _valuecount;
	return (allocationsize_ - _valuecount);
}

f16_16* CSharedNetValueBuffer::GetBuffer(unsigned int _offset) {

	if (nullptr == buffer_) {

		buffer_ = new f16_16[allocationsize_];
		for (unsigned int b_i = 0; b_i < allocationsize_; b_i++) {
			buffer_[b_i] = 0;
		}
	}
	return buffer_ + _offset;
}

void CSharedNetValueBuffer::CopyImage(uint8_t* _imagedata,
		unsigned int _imgwidth, unsigned int _imgheight,
		unsigned int _scanline) {

	f16_16 *vbuff = buffer_;
	uint8_t *cursl = _imagedata;
	for (unsigned int y = 0; y < _imgheight; y++) {
		uint8_t *curpixel = cursl;
		for (unsigned int x = 0; x < _imgwidth; x++) {
			uint8_t pixdata = *curpixel++;
			*vbuff++ = pixdata;
		}
		cursl += _scanline;
	}
}

void CSharedNetValueBuffer::CopyValues(unsigned int _bufferindex,
		unsigned char *_data, unsigned int _count) {

	f16_16 *vbuff = buffer_ + _bufferindex;
	for (unsigned int data_i = 0; data_i < _count; data_i++) {
		unsigned char curdata = *_data++;
		*vbuff++ = curdata;
	}
}
