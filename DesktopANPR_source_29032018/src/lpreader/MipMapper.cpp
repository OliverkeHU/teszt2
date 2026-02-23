#include "MipMapper.h"
#include "NetImage.h"

CMipMapper::CMipMapper() :
		mipmaps_(), srcimg_(nullptr), srcdx_(0), srcdy_(0), validmipmapcount_(0) {
}

CMipMapper::~CMipMapper() {
	FreeMipMaps();
}

bool CMipMapper::SetSrcImage(CNetImage *_srcimg) {

	bool reschanged;
	if (srcimg_ != nullptr) {
		if ((_srcimg->width_ != srcdx_) || (_srcimg->height_ != srcdy_)) {
			FreeMipMaps();
			srcdx_ = _srcimg->width_;
			srcdy_ = _srcimg->height_;
			reschanged = true;
		} else {
			Invalidate();
			reschanged = false;
		}
	} else {
		srcdx_ = _srcimg->width_;
		srcdy_ = _srcimg->height_;
		reschanged = true;
	}

	srcimg_ = _srcimg;
	return reschanged;
}

void CMipMapper::Invalidate() {

	validmipmapcount_ = 0;
}

void CMipMapper::GetMipMap(CNetImage **_mipmapimg, CPointf16_16 _shiftmatrix[2],
		CPointf16_16 *_refsrc, CPointf16_16 _min, CPointf16_16 _max,
		f16_16 _resvl, f16_16 *_veclen) {

	int32_t veclen = 0;
	if (_shiftmatrix[0].x & 0xf0000000) {
		veclen = -_shiftmatrix[0].x;
	} else {
		veclen = _shiftmatrix[0].x;
	}
	if (_shiftmatrix[0].y & 0xf0000000) {
		if (veclen < -_shiftmatrix[0].y) {
			veclen = -_shiftmatrix[0].y;
		}
	} else {
		if (veclen < _shiftmatrix[0].y) {
			veclen = _shiftmatrix[0].y;
		}
	}
	if (_shiftmatrix[1].x & 0xf0000000) {
		if (veclen < -_shiftmatrix[1].x) {
			veclen = -_shiftmatrix[1].x;
		}
	} else {
		if (veclen < _shiftmatrix[1].x) {
			veclen = _shiftmatrix[1].x;
		}
	}
	if (_shiftmatrix[1].y & 0xf0000000) {
		if (veclen < -_shiftmatrix[1].y) {
			veclen = -_shiftmatrix[1].y;
		}
	} else {
		if (veclen < _shiftmatrix[1].y) {
			veclen = _shiftmatrix[1].y;
		}
	}

	int curmap_i = -1;
	while (veclen > _resvl) {

		_shiftmatrix[0].x >>= 1;
		_shiftmatrix[0].y >>= 1;
		_shiftmatrix[1].x >>= 1;
		_shiftmatrix[1].y >>= 1;
		_refsrc->x >>= 1;
		_refsrc->y >>= 1;
		_min.x >>= 1;
		_max.x >>= 1;
		_min.y >>= 1;
		_max.y >>= 1;

		curmap_i++;

		veclen >>= 1;

	}

	if (curmap_i == -1) {
		*_mipmapimg = srcimg_;
	} else {
		PrepareMipMap(curmap_i);
		*_mipmapimg = mipmaps_[curmap_i];
	}

	if (nullptr != _veclen) {
		*_veclen = veclen;
	}
}

CNetImage * CMipMapper::GetFullMipMap(int _level) {

	if (srcimg_ == nullptr) {
		return nullptr;
	}
	if (_level == 0) {
		return srcimg_;
	}

	PrepareMipMap(_level - 1);
	return mipmaps_[_level - 1];
}

void CMipMapper::FreeMipMaps() {
	for (int map_i = 0; map_i < (int) mipmaps_.size(); map_i++) {
		delete mipmaps_[map_i];
	}
	mipmaps_.clear();
	validmipmapcount_ = 0;
}

void CMipMapper::PrepareMipMap(unsigned int _mapindex) {

	while (_mapindex >= validmipmapcount_) {

		CNetImage *curimage;
		if (mipmaps_.size() == validmipmapcount_) {
			curimage = new CNetImage();
			curimage->Create(srcdx_ >> (validmipmapcount_ + 1),
					srcdy_ >> (validmipmapcount_ + 1));
			mipmaps_.push_back(curimage);
		} else {
			curimage = mipmaps_[validmipmapcount_];
		}

		CNetImage *parimage;
		if (validmipmapcount_ == 0) {
			parimage = srcimg_;
		} else {
			parimage = mipmaps_[validmipmapcount_ - 1];
		}

		for (unsigned int y = 0; y < curimage->height_; y++) {
			unsigned char * parptr0 = parimage->GetPtr(0, y * 2);
			unsigned char * parptr1 = parimage->GetPtr(0, y * 2 + 1);
			unsigned char * curimgptr = curimage->GetPtr(0, y);

			for (unsigned int x = 0; x < curimage->width_; x++) {
				unsigned int c = (parptr0[0] + parptr0[1] + parptr1[0]
						+ parptr1[1]) / 4;
				*curimgptr = c;
				parptr0 += 2;
				parptr1 += 2;
				curimgptr++;
			}
		}

		validmipmapcount_++;
	}
}
