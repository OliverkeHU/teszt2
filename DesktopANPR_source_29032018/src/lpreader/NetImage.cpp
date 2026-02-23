#include "NetImage.h"
#include <stdio.h>
#include <algorithm>

using namespace std;

CNetImage::CNetImage() {
	allocatedmemsize_ = 0;
	imgdata_ = nullptr;
	width_ = 0;
	height_ = 0;
	scanline_ = 0;
	ismapped_ = false;
}

CNetImage::~CNetImage() {
	Free();
}

bool CNetImage::CheckAllocation(unsigned int _width, unsigned int _height,
		unsigned int _scanline) {

	unsigned int memneeded = _scanline * _height;
	if (allocatedmemsize_ < memneeded) {
		Free();
		imgdata_ = new unsigned char[memneeded + 1000];
		if (nullptr == imgdata_) {
			return false;
		}
		allocatedmemsize_ = memneeded;
	}

	width_ = _width;
	height_ = _height;
	scanline_ = _scanline;

	return true;
}

bool CNetImage::Create(unsigned int _width, unsigned int _height) {

	return CheckAllocation(_width, _height, _width);
}

bool CNetImage::CreateMap(unsigned int _width, unsigned int _height,
		unsigned int _scanline, const unsigned char *_imgdata) {

	Free();
	width_ = _width;
	height_ = _height;
	scanline_ = _scanline;
	imgdata_ = (unsigned char*) _imgdata;
	ismapped_ = true;

	return true;
}

void CNetImage::Free() {

	if (!ismapped_) {
		delete[] imgdata_;
		imgdata_ = nullptr;
	}
	allocatedmemsize_ = 0;
	imgdata_ = nullptr;
	width_ = 0;
	height_ = 0;
	scanline_ = 0;
	ismapped_ = false;
}

void CNetImage::FillImage(unsigned char _value) {

	fill_n(imgdata_, height_ * scanline_, _value);
}
