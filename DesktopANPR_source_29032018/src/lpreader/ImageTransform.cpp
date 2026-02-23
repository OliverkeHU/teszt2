#include "ImageTransform.h"
#include "MipMapper.h"
#include "NetImage.h"
#include <cmath>

void CImageTransform::Reset() {

	shiftmatrix_[0].x = 1 << 16;
	shiftmatrix_[0].y = 0;
	shiftmatrix_[1].x = 0;
	shiftmatrix_[1].y = 1 << 16;
}

void CImageTransform::Scale(f16_16 _factor) {

	int64_t f64 = _factor;
	shiftmatrix_[0].x = (f16_16) (((int64_t) shiftmatrix_[0].x * f64) >> 16);
	shiftmatrix_[0].y = (f16_16) (((int64_t) shiftmatrix_[0].y * f64) >> 16);
	shiftmatrix_[1].x = (f16_16) (((int64_t) shiftmatrix_[1].x * f64) >> 16);
	shiftmatrix_[1].y = (f16_16) (((int64_t) shiftmatrix_[1].y * f64) >> 16);
}

CImageTransform::CImageTransform() {
	Reset();
	srcimg_ = nullptr;
	tltransmode_ = ttmNormal;
	tlimg1_ = nullptr;
	tlimg2_ = nullptr;
	mipmapper_ = new CMipMapper;
	ownmipmapper_ = true;
}

CImageTransform::~CImageTransform() {
	if (nullptr != tlimg1_) {
		delete tlimg1_;
	}
	if (nullptr != tlimg2_) {
		delete tlimg2_;
	}
	if (ownmipmapper_) {
		delete mipmapper_;
	}
}

void CImageTransform::ApplyMatrix(CPointf16_16 _matrix[2]) {

	int64_t orig_s_x[2];
	int64_t orig_s_y[2];
	orig_s_x[0] = shiftmatrix_[0].x;
	orig_s_x[1] = shiftmatrix_[0].y;
	orig_s_y[0] = shiftmatrix_[1].x;
	orig_s_y[1] = shiftmatrix_[1].y;

	shiftmatrix_[0].x = (f16_16) ((_matrix[0].x * orig_s_x[0]
			+ _matrix[0].y * orig_s_y[0]) >> 16);
	shiftmatrix_[0].y = (f16_16) ((_matrix[0].x * orig_s_x[1]
			+ _matrix[0].y * orig_s_y[1]) >> 16);
	shiftmatrix_[1].x = (f16_16) ((_matrix[1].x * orig_s_x[0]
			+ _matrix[1].y * orig_s_y[0]) >> 16);
	shiftmatrix_[1].y = (f16_16) ((_matrix[1].x * orig_s_x[1]
			+ _matrix[1].y * orig_s_y[1]) >> 16);
}

void CImageTransform::Shear(f16_16 _mx, f16_16 _my) {

	if (0 != _mx) {
		CPointf16_16 rmatrix[2];
		rmatrix[0].x = 1 << 16;
		rmatrix[0].y = _mx;
		rmatrix[1].x = 0;
		rmatrix[1].y = 1 << 16;
		ApplyMatrix(rmatrix);
	}
	if (0 != _my) {
		CPointf16_16 rmatrix[2];
		rmatrix[0].x = 1 << 16;
		rmatrix[0].y = 0;
		rmatrix[1].x = _my;
		rmatrix[1].y = 1 << 16;
		ApplyMatrix(rmatrix);
	}
}

void CImageTransform::TransformNearestN(CNetImage &_destimg) {

	f16_16 src0x = refsrc_.x;
	f16_16 src0y = refsrc_.y;

	f16_16 refshiftx = (f16_16) ((-(int64_t) refdst_.x * shiftmatrix_[0].x
			- (int64_t) refdst_.y * shiftmatrix_[1].x) >> 16);
	f16_16 refshifty = (f16_16) ((-(int64_t) refdst_.x * shiftmatrix_[0].y
			- (int64_t) refdst_.y * shiftmatrix_[1].y) >> 16);
	src0x += refshiftx;
	src0y += refshifty;

	int srcimg_w = srcimg_->width_;
	int srcimg_h = srcimg_->height_;
	int destimg_w = _destimg.width_;
	int destimg_h = _destimg.height_;

	f16_16 srcx = src0x;
	f16_16 srcy = src0y;

	uint8_t * imgdata_d = _destimg.imgdata_;
	uint8_t * imgdata_s = srcimg_->imgdata_;
	int sssize_s = srcimg_->scanline_;
	int sssize_d = _destimg.scanline_;

	for (int desty = 0; desty < destimg_h; desty++) {
		int32_t linestartx = srcx;
		int32_t linestarty = srcy;
		for (int destx = 0; destx < destimg_w; destx++) {

			int i_srcx = srcx >> 16;
			int i_srcy = srcy >> 16;
			if ((i_srcx >= 0) && (i_srcx < srcimg_w) && (i_srcy >= 0)
					&& (i_srcy < srcimg_h)) {
				imgdata_d[destx + desty * sssize_d] = imgdata_s[(srcx >> 16)
						+ ((srcy >> 16) * sssize_s)];
			} else {
				imgdata_d[destx + desty * sssize_d] = 255;
			}

			srcx += shiftmatrix_[0].x;
			srcy += shiftmatrix_[0].y;
		}

		srcx = linestartx + shiftmatrix_[1].x;
		srcy = linestarty + shiftmatrix_[1].y;
	}
}

void CImageTransform::TransformBilinear(CNetImage &_destimg, int32_t _resvl,
		int32_t *_veclen) {

	int destimg_w = _destimg.width_;
	int destimg_h = _destimg.height_;

	CPointf16_16 min;
	CPointf16_16 max;
	CPointf16_16 cur;
	TransformPoint(min, CPointf16_16(0, 0));
	max = min;
	TransformPoint(cur, CPointf16_16(destimg_w << 16, 0));
	if (max.x < cur.x) {
		max.x = cur.x;
	} else if (min.x > cur.x) {
		min.x = cur.x;
	}
	if (max.y < cur.y) {
		max.y = cur.y;
	} else if (min.y > cur.y) {
		min.y = cur.y;
	}
	TransformPoint(cur, CPointf16_16(0, destimg_h << 16));
	if (max.x < cur.x) {
		max.x = cur.x;
	} else if (min.x > cur.x) {
		min.x = cur.x;
	}
	if (max.y < cur.y) {
		max.y = cur.y;
	} else if (min.y > cur.y) {
		min.y = cur.y;
	}
	TransformPoint(cur, CPointf16_16(destimg_w << 16, destimg_h << 16));
	if (max.x < cur.x) {
		max.x = cur.x;
	} else if (min.x > cur.x) {
		min.x = cur.x;
	}
	if (max.y < cur.y) {
		max.y = cur.y;
	} else if (min.y > cur.y) {
		min.y = cur.y;
	}

	CNetImage *mmsrcimg;
	CPointf16_16 mmshiftmatrix[2];

	mmshiftmatrix[0].x = shiftmatrix_[0].x;
	mmshiftmatrix[0].y = shiftmatrix_[0].y;
	mmshiftmatrix[1].x = shiftmatrix_[1].x;
	mmshiftmatrix[1].y = shiftmatrix_[1].y;
	CPointf16_16 mmrefsrc;
	mmrefsrc = refsrc_;
	mipmapper_->GetMipMap(&mmsrcimg, mmshiftmatrix, &mmrefsrc, min, max, _resvl,
			_veclen);

	f16_16 src0x = mmrefsrc.x;
	f16_16 src0y = mmrefsrc.y;

	f16_16 refshiftx = (f16_16) ((-(int64_t) refdst_.x * mmshiftmatrix[0].x
			- (int64_t) refdst_.y * mmshiftmatrix[1].x) >> 16);
	f16_16 refshifty = (f16_16) ((-(int64_t) refdst_.x * mmshiftmatrix[0].y
			- (int64_t) refdst_.y * mmshiftmatrix[1].y) >> 16);
	src0x += refshiftx;
	src0y += refshifty;

	int srcimg_w = mmsrcimg->width_;
	int srcimg_h = mmsrcimg->height_;

	f16_16 srcx = src0x;
	f16_16 srcy = src0y;

	uint8_t * imgdata_d = _destimg.imgdata_;
	uint8_t * imgdata_s = mmsrcimg->imgdata_;
	int sssize_s = mmsrcimg->scanline_;
	int sssize_d = _destimg.scanline_;

	bool hasoffimg = true;

	int topleftx = srcx >> 16;
	int toplefty = srcy >> 16;
	if ((topleftx > 1) && (toplefty > 1) && (topleftx < (srcimg_w - 1))
			&& (toplefty < (srcimg_h - 1))) {

		int toprightx = (srcx + destimg_w * mmshiftmatrix[0].x) >> 16;
		int toprighty = (srcy + destimg_w * mmshiftmatrix[0].y) >> 16;
		if ((toprightx > 1) && (toprighty > 1) && (toprightx < (srcimg_w - 1))
				&& (toprighty < (srcimg_h - 1))) {

			int bottomleftx = (srcx + destimg_h * mmshiftmatrix[1].x) >> 16;
			int bottomlefty = (srcy + destimg_h * mmshiftmatrix[1].y) >> 16;
			if ((bottomleftx > 1) && (bottomlefty > 1)
					&& (bottomleftx < (srcimg_w - 1))
					&& (bottomlefty < (srcimg_h - 1))) {

				int bottomrightx = (srcx + destimg_h * mmshiftmatrix[1].x
						+ destimg_w * mmshiftmatrix[0].x) >> 16;
				int bottomrighty = (srcy + destimg_h * mmshiftmatrix[1].y
						+ destimg_w * mmshiftmatrix[0].y) >> 16;
				if ((bottomrightx > 1) && (bottomrighty > 1)
						&& (bottomrightx < (srcimg_w - 1))
						&& (bottomrighty < (srcimg_h - 1))) {

					hasoffimg = false;
				}
			}
		}
	}

	if (hasoffimg) {

		for (int desty = 0; desty < destimg_h; desty++) {
			int32_t linestartx = srcx;
			int32_t linestarty = srcy;
			for (int destx = 0; destx < destimg_w; destx++) {

				int i_srcx = srcx >> 16;
				int i_srcy = srcy >> 16;
				if ((i_srcx >= 0) && (i_srcx < (srcimg_w - 1)) && (i_srcy >= 0)
						&& (i_srcy < (srcimg_h - 1))) {

					uint32_t hor_right = (srcx & 0x0000ffff) >> 8;
					uint32_t hor_left = 0x00000100 - hor_right;
					uint32_t ver_low = (srcy & 0x0000ffff) >> 8;
					uint32_t ver_up = 0x00000100 - ver_low;

					int offset = (srcx >> 16) + ((srcy >> 16) * sssize_s);
					uint32_t t00 = imgdata_s[offset];
					uint32_t t01 = imgdata_s[offset + sssize_s];
					uint32_t t10 = imgdata_s[offset + 1];
					uint32_t t11 = imgdata_s[offset + 1 + sssize_s];

					uint32_t col = ((t00 * hor_left + t10 * hor_right) * ver_up
							+ (t01 * hor_left + t11 * hor_right) * ver_low)
							>> 16;

					if (col == 255) {
						col = 254;
					}

					imgdata_d[destx + desty * sssize_d] = col;
				} else {
					imgdata_d[destx + desty * sssize_d] = 255;
					hasoffimg = true;
				}

				srcx += mmshiftmatrix[0].x;
				srcy += mmshiftmatrix[0].y;
			}

			srcx = linestartx + mmshiftmatrix[1].x;
			srcy = linestarty + mmshiftmatrix[1].y;
		}
	} else {

		for (int desty = 0; desty < destimg_h; desty++) {
			int32_t linestartx = srcx;
			int32_t linestarty = srcy;
			for (int destx = 0; destx < destimg_w; destx++) {

				uint32_t hor_right = (srcx & 0x0000ffff) >> 8;
				uint32_t hor_left = 0x00000100 - hor_right;
				uint32_t ver_low = (srcy & 0x0000ffff) >> 8;
				uint32_t ver_up = 0x00000100 - ver_low;

				int offset = (srcx >> 16) + ((srcy >> 16) * sssize_s);
				uint32_t t00 = imgdata_s[offset];
				uint32_t t01 = imgdata_s[offset + sssize_s];
				uint32_t t10 = imgdata_s[offset + 1];
				uint32_t t11 = imgdata_s[offset + 1 + sssize_s];

				uint32_t col = ((t00 * hor_left + t10 * hor_right) * ver_up
						+ (t01 * hor_left + t11 * hor_right) * ver_low) >> 16;

				imgdata_d[destx + desty * sssize_d] = col;

				srcx += mmshiftmatrix[0].x;
				srcy += mmshiftmatrix[0].y;
			}

			srcx = linestartx + mmshiftmatrix[1].x;
			srcy = linestarty + mmshiftmatrix[1].y;
		}
	}

	if (hasoffimg) {

		unsigned char curcolor = 0;

		int curdist = 6250000;
		for (int x = 0; x < destimg_w; x++) {
			for (int y = 0; y < destimg_h; y++) {
				int dx = x - destimg_w;
				int dy = y - destimg_h;
				int dist = dx * dx + dy * dy;
				if (dist < curdist) {
					unsigned char c = *(_destimg.GetPtr(x, y));
					if (c != 255) {
						curcolor = c;
					}
				}
			}
		}

		int centery = destimg_h / 2;
		int centerx = destimg_w / 2;
		for (int x = centerx; x < destimg_w; x++) {
			unsigned char *curptr = _destimg.GetPtr(x, centery);
			unsigned char c = *curptr;
			if (c == 255) {
				*curptr = curcolor;
			} else {
				curcolor = c;
			}
		}
		for (int x = centerx; x >= 0; x--) {
			unsigned char *curptr = _destimg.GetPtr(x, centery);
			unsigned char c = *curptr;
			if (c == 255) {
				*curptr = curcolor;
			} else {
				curcolor = c;
			}
		}

		for (int x = 0; x < destimg_w; x++) {
			for (int y = centery; y >= 0; y--) {
				unsigned char *curptr = _destimg.GetPtr(x, y);
				unsigned char c = *curptr;
				if (c == 255) {
					*curptr = curcolor;
				} else {
					curcolor = c;
				}
			}
			for (int y = centery; y < destimg_h; y++) {
				unsigned char *curptr = _destimg.GetPtr(x, y);
				unsigned char c = *curptr;
				if (c == 255) {
					*curptr = curcolor;
				} else {
					curcolor = c;
				}
			}
		}
	}
}

void CImageTransform::TransformTrilinear(CNetImage &_destimg) {

	int32_t veclen1, veclen2;
	int destimg_w = _destimg.width_;
	int destimg_h = _destimg.height_;

	if (nullptr == tlimg1_) {
		tlimg1_ = new CNetImage();
	}
	if (tltransmode_ == ttmOSMM) {
		if (nullptr == tlimg2_) {
			tlimg2_ = new CNetImage();
		}
	}

	if (tltransmode_ == ttmOSMM) {

		tlimg1_->Create(destimg_w * 4, destimg_h * 2);
		tlimg2_->Create(destimg_w * 4, destimg_h * 2);

		refdst_.x = refdst_.x << 2;
		refdst_.y = refdst_.y << 2;
		Scale(0x4000);
		TransformBilinear(*tlimg1_, 0x00020000, &veclen1);
		TransformBilinear(*tlimg2_, 0x00040000, &veclen2);
		refdst_.x = refdst_.x >> 2;
		refdst_.y = refdst_.y >> 2;
		Scale(0x40000);
	} else {
		tlimg1_->Create(destimg_w, destimg_h);
		TransformBilinear(_destimg, 0x00020000, &veclen1);
		TransformBilinear(*tlimg1_, 0x00040000, &veclen2);
	}

	int32_t e1 = veclen1 - 0x00010000;
	int32_t e2 = 0x00010000 - e1;

	if ((tltransmode_ == ttmNormal) || (tltransmode_ == ttmFilterMinMinMax)) {

		for (int y = 0; y < destimg_h; y++) {
			uint8_t * imgdata_d = _destimg.GetPtr(0, y);
			uint8_t * imgdata_2 = tlimg1_->GetPtr(0, y);
			for (int x = 0; x < destimg_w; x++) {
				imgdata_d[x] = (imgdata_d[x] * e1 + imgdata_2[x] * e2) >> 16;
			}
		}
	} else if (tltransmode_ == ttmAverages) {

		unsigned int avgsum = 0;
		for (int y = 0; y < destimg_h; y++) {
			uint8_t * imgdata_d = _destimg.GetPtr(0, y);
			uint8_t * imgdata_2 = tlimg1_->GetPtr(0, y);
			for (int x = 0; x < destimg_w; x++) {
				uint8_t c = (imgdata_d[x] * e1 + imgdata_2[x] * e2) >> 16;
				imgdata_d[x] = c;
				avgsum += c;
			}
		}
		tltransavg_ = (float) avgsum / (destimg_w * destimg_h);

		int avglsum = 0;
		int avglcount = 0;
		int avghsum = 0;
		int avghcount = 0;
		uint8_t avg8 = (uint8_t) tltransavg_;
		for (int y = 0; y < destimg_h; y++) {
			uint8_t * imgdata_d = _destimg.GetPtr(0, y);
			for (int x = 0; x < destimg_w; x++) {
				uint8_t c = imgdata_d[x];
				if (c < avg8) {
					avglsum += c;
					avglcount++;
				} else if (c > avg8) {
					avghsum += c;
					avghcount++;
				}
			}
		}
		if (avglcount == 0) {
			tltransavgl_ = tltransavg_;
		} else {
			tltransavgl_ = (float) avglsum / avglcount;
		}
		if (avghcount == 0) {
			tltransavgh_ = tltransavg_;
		} else {
			tltransavgh_ = (float) avghsum / avghcount;
		}

	} else if (tltransmode_ == ttmOSMM) {

		unsigned int avgsum = 0;
		for (int paty = 0; paty < destimg_h / 2; paty++) {
			uint8_t * imgdata_d = _destimg.GetPtr(0, paty);
			for (int patx = 0; patx < destimg_w; patx++) {
				unsigned char min = 255;
				unsigned char max = 0;

				for (int osmmy = paty * 4; osmmy < (paty + 1) * 4; osmmy++) {
					uint8_t * imgdata_1 = tlimg1_->GetPtr(0, osmmy);
					uint8_t * imgdata_2 = tlimg2_->GetPtr(0, osmmy);
					for (int osmmx = patx * 4; osmmx < (patx + 1) * 4;
							osmmx++) {

						uint8_t trilinpixel = (imgdata_1[osmmx] * e1
								+ imgdata_2[osmmx] * e2) >> 16;
						imgdata_1[osmmx] = trilinpixel;

						avgsum += trilinpixel;

						if (trilinpixel != 255) {
							if (min > trilinpixel) {
								min = trilinpixel;
							}
							if (max < trilinpixel) {
								max = trilinpixel;
							}
						}
					}
				}

				imgdata_d[patx] = min;
				imgdata_d[patx + (destimg_h / 2) * destimg_w] = max;
			}
		}
		tltransavg_ = ((float) avgsum / (destimg_w * destimg_h)) / 8.0f;

		int avglsum = 0;
		int avglcount = 0;
		int avghsum = 0;
		int avghcount = 0;
		uint8_t avg8 = (uint8_t) tltransavg_;
		for (int y = 0; y < destimg_h * 2; y++) {
			uint8_t * imgdata_1 = tlimg1_->GetPtr(0, y);
			for (int x = 0; x < destimg_w * 4; x++) {
				uint8_t c = imgdata_1[x];
				if (c < avg8) {
					avglsum += c;
					avglcount++;
				} else if (c > avg8) {
					avghsum += c;
					avghcount++;
				}
			}
		}
		if (avglcount == 0) {
			tltransavgl_ = tltransavg_;
		} else {
			tltransavgl_ = (float) avglsum / avglcount;
		}
		if (avghcount == 0) {
			tltransavgh_ = tltransavg_;
		} else {
			tltransavgh_ = (float) avghsum / avghcount;
		}
	}
}

void CImageTransform::TransformPoint(CPointf16_16 &_on_src,
		const CPointf16_16 &_on_dst) const {

	_on_src.Set(refsrc_);
	f16_16 refshiftx = (f16_16) ((-(int64_t) (refdst_.x - _on_dst.x)
			* shiftmatrix_[0].x
			- (int64_t) (refdst_.y - _on_dst.y) * shiftmatrix_[1].x) >> 16);
	f16_16 refshifty = (f16_16) ((-(int64_t) (refdst_.x - _on_dst.x)
			* shiftmatrix_[0].y
			- (int64_t) (refdst_.y - _on_dst.y) * shiftmatrix_[1].y) >> 16);
	_on_src.x += refshiftx;
	_on_src.y += refshifty;
}

void CImageTransform::Set(CNetImage &_destimg, CPointf16_16 &_p0s,
		CPointf16_16 &_p1s, CPointf16_16 &_p2s) {

	refsrc_.Set(_p0s);
	refdst_.Set(0, 0);
	shiftmatrix_[0].x = (_p1s.x - _p0s.x) / (int) _destimg.width_;
	shiftmatrix_[0].y = (_p1s.y - _p0s.y) / (int) _destimg.width_;
	shiftmatrix_[1].x = (_p2s.x - _p0s.x) / (int) _destimg.height_;
	shiftmatrix_[1].y = (_p2s.y - _p0s.y) / (int) _destimg.height_;
}

void CImageTransform::Set(CPointf16_16 &_p0s, CPointf16_16 &_p1s,
		CPointf16_16 &_p0d, CPointf16_16 &_p1d) {

	refsrc_ = _p0s;
	refdst_ = _p0d;
	if (_p1d.x != _p0d.x) {
		shiftmatrix_[0].x = (f16_16) ((((int64_t) (_p1s.x - _p0s.x)) << 16)
				/ (_p1d.x - _p0d.x));
		shiftmatrix_[0].y = (f16_16) ((((int64_t) (_p1s.y - _p0s.y)) << 16)
				/ (_p1d.x - _p0d.x));
		shiftmatrix_[1].x = -shiftmatrix_[0].y;
		shiftmatrix_[1].y = shiftmatrix_[0].x;
	} else {
		shiftmatrix_[1].x = (f16_16) ((((int64_t) (_p1s.x - _p0s.x)) << 16)
				/ (_p1d.y - _p0d.y));
		shiftmatrix_[1].y = (f16_16) ((((int64_t) (_p1s.y - _p0s.y)) << 16)
				/ (_p1d.y - _p0d.y));
		shiftmatrix_[0].x = shiftmatrix_[1].y;
		shiftmatrix_[0].y = -shiftmatrix_[1].x;
	}
}

void CImageTransform::SetMipMapper(CMipMapper *_mipmapper) {

	if (ownmipmapper_) {
		ownmipmapper_ = false;
		delete mipmapper_;
	}
	mipmapper_ = _mipmapper;
}
