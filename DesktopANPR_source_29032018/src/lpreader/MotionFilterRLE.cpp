#include "MotionFilterRLE.h"
#include "NetImage.h"
#include "BinRLE.h"
#include "MipMapper.h"
#include <algorithm>

CMotionFilterRLE::CMotionFilterRLE() {
	baseimage_ = new CNetImage;
	motionarea_ = new CBinRLE;
}

CMotionFilterRLE::~CMotionFilterRLE() {
	delete motionarea_;
	delete baseimage_;
}

void CMotionFilterRLE::Update(class CMipMapper *_mipmapper, unsigned int _level,
		unsigned char _threshold, class CBinRLE *_area) {
	CNetImage *curimage = _mipmapper->GetFullMipMap(_level);

	if ((curimage->width_ != baseimage_->width_)
			|| (curimage->height_ != baseimage_->height_)) {
		baseimage_->Create(curimage->width_, curimage->height_);

		unsigned int startx;
		unsigned int starty;
		unsigned int runlength;
		_area->StartRun();
		while (_area->GetNextRun(&startx, &starty, &runlength)) {
			unsigned char *curptr = curimage->GetPtr(startx, starty);
			unsigned char *baseptr = baseimage_->GetPtr(startx, starty);
			std::copy_n(curptr, runlength, baseptr);
		}

		motionarea_->CopyFrom(_area);
	} else {

		motionarea_->Clear();
		unsigned int startx;
		unsigned int starty;
		unsigned int runlength;
		_area->StartRun();

		while (_area->GetNextRun(&startx, &starty, &runlength)) {
			unsigned char *curptr = curimage->GetPtr(startx, starty);
			unsigned char *baseptr = baseimage_->GetPtr(startx, starty);
			unsigned int rl_i = 0;

			while (rl_i < runlength) {

				while (rl_i < runlength) {
					unsigned char c1 = baseptr[rl_i];
					unsigned char c2 = curptr[rl_i];
					unsigned char diff;
					diff = c1 - c2 + _threshold;
					if (diff >= (2 * _threshold)) {
						break;
					}
					rl_i++;
				}
				if (rl_i == runlength) {

					break;
				}

				unsigned int runstart_i = rl_i;

				while (rl_i < runlength) {
					unsigned char c1 = baseptr[rl_i];
					unsigned char c2 = curptr[rl_i];
					unsigned char diff;
					diff = c1 - c2 + _threshold;
					if (diff < (2 * _threshold)) {
						break;
					}
					rl_i++;
				}

				motionarea_->AddRun(startx + runstart_i, starty,
						rl_i - runstart_i);
			}

		}

		motionarea_->ExpandHorizontal(curimage->width_);
		motionarea_->ExpandVertical(curimage->height_);

		motionarea_->StartRun();
		while (motionarea_->GetNextRun(&startx, &starty, &runlength)) {
			unsigned char *curptr = curimage->GetPtr(startx, starty);
			unsigned char *baseptr = baseimage_->GetPtr(startx, starty);
			std::copy_n(curptr, runlength, baseptr);
		}
	}
}
