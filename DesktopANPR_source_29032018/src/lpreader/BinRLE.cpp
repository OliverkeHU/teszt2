#include "BinRLE.h"
#include "NetImage.h"

CBinRLE::CBinRLE() {
	runs_ = new std::vector<RLERunS>;
	tempruns_ = nullptr;
}

CBinRLE::~CBinRLE() {
	delete tempruns_;
	delete runs_;
}

void CBinRLE::CreateFromImage(CNetImage *_image, unsigned char _threshold) {

	runs_->clear();

	for (unsigned int y = 0; y < _image->height_; y++) {
		unsigned char* lineptr = _image->GetPtr(0, y);
		int runstartx;

		if (*lineptr < _threshold) {
			runstartx = -1;
		} else {
			runstartx = 0;
		}

		for (unsigned int x = 1; x < _image->width_; x++) {
			if (lineptr[x] < _threshold) {
				if (runstartx != -1) {

					runs_->emplace_back(RLERunS(runstartx, y, x - runstartx));
					runstartx = -1;
				}
			} else {
				if (runstartx == -1) {

					runstartx = x;
				}
			}
		}

		if (runstartx != -1) {
			runs_->emplace_back(
					RLERunS(runstartx, y, _image->width_ - runstartx));
		}
	}
}

void CBinRLE::CreateFullImage(unsigned int _width, unsigned int _height) {

	runs_->clear();
	for (unsigned int y = 0; y < _height; y++) {
		runs_->emplace_back(RLERunS(0, y, _width));
	}
}

void CBinRLE::StartRun() {

	nextrunindex_ = 0;
}

bool CBinRLE::GetNextRun(unsigned int *_startx, unsigned int *_starty,
		unsigned int *_runlength) {

	if (nextrunindex_ == runs_->size()) {
		*_runlength = 0;
		return false;
	}

	RLERunS &currun = (*runs_)[nextrunindex_];

	*_startx = currun.startx_;
	*_starty = currun.starty_;
	*_runlength = currun.runlength_;

	nextrunindex_++;
	return true;
}

void CBinRLE::CopyFrom(CBinRLE *_t) {

	runs_->clear();

	unsigned int startx;
	unsigned int starty;
	unsigned int runlength;
	_t->StartRun();
	while (_t->GetNextRun(&startx, &starty, &runlength)) {
		runs_->emplace_back(RLERunS(startx, starty, runlength));
	}
}

void CBinRLE::Clear() {

	runs_->clear();
}

void CBinRLE::AddRun(unsigned int _startx, unsigned int _starty,
		unsigned int _runlength) {

	bool appendnewrunatend = false;
	if (runs_->size() == 0) {

		appendnewrunatend = true;
	} else {

		RLERunS &lastrun = runs_->back();
		if (lastrun.starty_ < _starty) {

			appendnewrunatend = true;
		} else if (lastrun.starty_ == _starty) {
			if ((lastrun.startx_ + lastrun.runlength_) < _startx) {

				appendnewrunatend = true;
			} else if ((lastrun.startx_ <= _startx)
					&& ((lastrun.startx_ + lastrun.runlength_) >= _startx)) {

				unsigned int lastxorig = lastrun.startx_ + lastrun.runlength_
						- 1;
				unsigned int lastxnew = _startx + _runlength - 1;
				if (lastxorig >= lastxnew) {

					return;
				} else {

					lastrun.runlength_ = lastxnew - lastrun.startx_ + 1;
					return;
				}
			}
		}
	}

	if (appendnewrunatend) {
		runs_->emplace_back(RLERunS(_startx, _starty, _runlength));
		return;
	}

	if (tempruns_ == nullptr) {
		tempruns_ = new std::vector<RLERunS>;
	} else {
		tempruns_->clear();
	}

	unsigned int origrun_i = 0;
	for (origrun_i = 0; origrun_i < runs_->size(); origrun_i++) {
		RLERunS &origrun = (*runs_)[origrun_i];
		if ((origrun.starty_ > _starty)
				|| ((origrun.starty_ == _starty)
						&& ((origrun.startx_ + origrun.runlength_) >= _startx))) {
			break;
		}

		tempruns_->emplace_back(
				RLERunS(origrun.startx_, origrun.starty_, origrun.runlength_));
	}

	{
		RLERunS &origrun = (*runs_)[origrun_i];
		if ((origrun.starty_ == _starty) && (origrun.startx_ <= _startx)
				&& ((origrun.startx_ + origrun.runlength_)
						>= (_startx + _runlength))) {
			tempruns_->clear();
			return;
		}
	}

	unsigned int newstartx = _startx;
	unsigned int newendx = _startx + _runlength - 1;

	for (; origrun_i < runs_->size(); origrun_i++) {
		RLERunS &origrun = (*runs_)[origrun_i];
		if (origrun.starty_ != _starty) {
			break;
		}
		if (origrun.startx_ > (newendx + 1)) {
			break;
		}
		unsigned int origendx = origrun.startx_ + origrun.runlength_ - 1;
		if (newendx < origendx) {
			newendx = origendx;
		}
	}

	{
		tempruns_->emplace_back(
				RLERunS(newstartx, _starty, newendx - newstartx + 1));
	}

	while (runs_->size() != origrun_i) {
		RLERunS &origrun = (*runs_)[origrun_i];
		tempruns_->emplace_back(origrun);
		origrun_i++;
	}

	std::vector<RLERunS> *t = tempruns_;
	tempruns_ = runs_;
	runs_ = t;
	tempruns_->clear();
}

void CBinRLE::ExpandHorizontal(unsigned int _imagewidth) {

	if (tempruns_ == nullptr) {
		tempruns_ = new std::vector<RLERunS>;
	} else {
		tempruns_->clear();
	}

	unsigned int prevy = -1;
	unsigned int prevendx = 0;
	for (unsigned int r_i = 0; r_i < runs_->size(); r_i++) {
		RLERunS &currun = (*runs_)[r_i];
		if ((currun.starty_ == prevy) && ((currun.startx_ - 1) <= prevendx)) {

			unsigned int newrl = currun.runlength_;
			if ((currun.startx_ + newrl) < _imagewidth) {
				newrl += 1;
			}

			RLERunS &lastrun = (*tempruns_)[tempruns_->size() - 1];
			prevendx = currun.startx_ + newrl;
			lastrun.runlength_ = prevendx - lastrun.startx_;

		} else {

			tempruns_->emplace_back();
			RLERunS &newrunp1 = tempruns_->back();
			newrunp1.starty_ = currun.starty_;
			unsigned int newrl = currun.runlength_;

			if (currun.startx_ > 0) {
				newrunp1.startx_ = currun.startx_ - 1;
				newrl++;
			} else {
				newrunp1.startx_ = currun.startx_;
			}

			if ((currun.startx_ + newrl) < _imagewidth) {
				newrunp1.runlength_ = newrl + 1;
			} else {
				newrunp1.runlength_ = newrl;
			}

			prevendx = newrunp1.startx_ + newrunp1.runlength_;
		}

		prevy = currun.starty_;
	}

	std::vector<RLERunS> *t = tempruns_;
	tempruns_ = runs_;
	runs_ = t;
	tempruns_->clear();
}

void CBinRLE::ExpandVertical(unsigned int _imageheight) {

	if (tempruns_ == nullptr) {
		tempruns_ = new std::vector<RLERunS>;
	} else {
		tempruns_->clear();
	}

	for (unsigned int y = 0; y < _imageheight; y++) {

		int cury_i = -1;
		int cury1_i = -1;
		for (unsigned int r_i = 0; r_i < runs_->size(); r_i++) {
			RLERunS &currun = (*runs_)[r_i];
			if ((cury_i == -1) && (currun.starty_ == y)) {
				cury_i = r_i;
			} else {
				if (currun.starty_ == (y + 1)) {
					cury1_i = r_i;
					break;
				} else if (currun.starty_ > (y + 1)) {
					break;
				}
			}
		}

		while ((cury_i != -1) || (cury1_i != -1)) {
			if (cury_i == -1) {

				for (unsigned int cr_i = cury1_i; cr_i < runs_->size();
						cr_i++) {
					RLERunS &currun = (*runs_)[cr_i];
					if (currun.starty_ != (y + 1)) {
						break;
					}
					tempruns_->emplace_back(
							RLERunS(currun.startx_, y, currun.runlength_));
				}
				break;
			}
			if (cury1_i == -1) {

				for (unsigned int cr_i = cury_i; cr_i < runs_->size(); cr_i++) {
					RLERunS &currun = (*runs_)[cr_i];
					if (currun.starty_ != y) {
						break;
					}
					tempruns_->emplace_back(
							RLERunS(currun.startx_, y, currun.runlength_));
				}
				break;
			}

			RLERunS &curruny = (*runs_)[cury_i];
			RLERunS &curruny1 = (*runs_)[cury1_i];

			if ((curruny.startx_ + curruny.runlength_) < curruny1.startx_) {

				tempruns_->emplace_back(
						RLERunS(curruny.startx_, y, curruny.runlength_));
				cury_i++;
				if ((cury_i == (int) runs_->size())
						|| ((*runs_)[cury_i].starty_ != y)) {
					cury_i = -1;
				}
				continue;
			}
			if ((curruny1.startx_ + curruny1.runlength_) < curruny.startx_) {

				tempruns_->emplace_back(
						RLERunS(curruny1.startx_, y, curruny1.runlength_));
				cury1_i++;
				if ((cury1_i == (int) runs_->size())
						|| ((*runs_)[cury1_i].starty_ != (y + 1))) {
					cury1_i = -1;
				}
				continue;
			}

			unsigned int startx;
			if (curruny.startx_ < curruny1.startx_) {
				startx = curruny.startx_;
			} else {
				startx = curruny1.startx_;
			}

			unsigned int endx;
			while (true) {
				RLERunS &currunyw = (*runs_)[cury_i];
				RLERunS &currunyw1 = (*runs_)[cury1_i];

				if ((currunyw.startx_ + currunyw.runlength_)
						== (currunyw1.startx_ + currunyw1.runlength_)) {

					endx = currunyw.startx_ + currunyw.runlength_;

					cury_i++;
					if ((cury_i == (int) runs_->size())
							|| ((*runs_)[cury_i].starty_ != y)) {
						cury_i = -1;
					}

					cury1_i++;
					if ((cury1_i == (int) runs_->size())
							|| ((*runs_)[cury1_i].starty_ != (y + 1))) {
						cury1_i = -1;
					}

					break;
				}

				if ((currunyw.startx_ + currunyw.runlength_)
						< (currunyw1.startx_ + currunyw1.runlength_)) {

					cury_i++;
					if ((cury_i == (int) runs_->size())
							|| ((*runs_)[cury_i].starty_ != y)) {
						cury_i = -1;
						endx = currunyw1.startx_ + currunyw1.runlength_;

						cury1_i++;
						if ((cury1_i == (int) runs_->size())
								|| ((*runs_)[cury1_i].starty_ != (y + 1))) {
							cury1_i = -1;
						}

						break;
					} else {

						if ((*runs_)[cury_i].startx_
								> (currunyw1.startx_ + currunyw1.runlength_)) {
							endx = currunyw1.startx_ + currunyw1.runlength_;
							cury1_i++;
							if ((cury1_i == (int) runs_->size())
									|| ((*runs_)[cury1_i].starty_ != (y + 1))) {
								cury1_i = -1;
							}
							break;
						}
					}

					continue;
				}

				cury1_i++;
				if ((cury1_i == (int) runs_->size())
						|| ((*runs_)[cury1_i].starty_ != (y + 1))) {
					cury1_i = -1;
					endx = currunyw.startx_ + currunyw.runlength_;

					cury_i++;
					if ((cury_i == (int) runs_->size())
							|| ((*runs_)[cury_i].starty_ != y)) {
						cury_i = -1;
					}

					break;
				} else {

					if ((*runs_)[cury1_i].startx_
							> (currunyw.startx_ + currunyw.runlength_)) {
						endx = currunyw.startx_ + currunyw.runlength_;
						cury_i++;
						if ((cury_i == (int) runs_->size())
								|| ((*runs_)[cury_i].starty_ != y)) {
							cury_i = -1;
						}
						break;
					}
				}

			}

			tempruns_->emplace_back(RLERunS(startx, y, endx - startx));
		}
	}

	runs_->clear();

	for (unsigned int y = 0; y < _imageheight; y++) {

		int cury_i = -1;
		int cury1_i = -1;
		for (unsigned int r_i = 0; r_i < tempruns_->size(); r_i++) {
			RLERunS &currun = (*tempruns_)[r_i];

			if ((cury1_i == -1) && (currun.starty_ == y - 1)) {
				cury1_i = r_i;
			} else {
				if (currun.starty_ == y) {
					cury_i = r_i;
					break;
				} else if (currun.starty_ > y) {
					break;
				}
			}
		}

		while ((cury_i != -1) || (cury1_i != -1)) {
			if (cury_i == -1) {

				for (unsigned int cr_i = cury1_i; cr_i < tempruns_->size();
						cr_i++) {
					RLERunS &currun = (*tempruns_)[cr_i];
					if (currun.starty_ != (y - 1)) {
						break;
					}
					runs_->emplace_back(
							RLERunS(currun.startx_, y, currun.runlength_));
				}
				break;
			}
			if (cury1_i == -1) {

				for (unsigned int cr_i = cury_i; cr_i < tempruns_->size();
						cr_i++) {
					RLERunS &currun = (*tempruns_)[cr_i];
					if (currun.starty_ != y) {
						break;
					}
					runs_->emplace_back(
							RLERunS(currun.startx_, y, currun.runlength_));
				}
				break;
			}

			RLERunS &curruny = (*tempruns_)[cury_i];
			RLERunS &curruny1 = (*tempruns_)[cury1_i];

			if ((curruny.startx_ + curruny.runlength_) < curruny1.startx_) {

				runs_->emplace_back(
						RLERunS(curruny.startx_, y, curruny.runlength_));
				cury_i++;
				if ((cury_i == (int) tempruns_->size())
						|| ((*tempruns_)[cury_i].starty_ != y)) {
					cury_i = -1;
				}
				continue;
			}
			if ((curruny1.startx_ + curruny1.runlength_) < curruny.startx_) {

				runs_->emplace_back(
						RLERunS(curruny1.startx_, y, curruny1.runlength_));
				cury1_i++;
				if ((cury1_i == (int) tempruns_->size())
						|| ((*tempruns_)[cury1_i].starty_ != (y - 1))) {
					cury1_i = -1;
				}
				continue;
			}

			unsigned int startx;
			if (curruny.startx_ < curruny1.startx_) {
				startx = curruny.startx_;
			} else {
				startx = curruny1.startx_;
			}

			unsigned int endx;
			while (true) {
				RLERunS &currunyw = (*tempruns_)[cury_i];
				RLERunS &currunyw1 = (*tempruns_)[cury1_i];

				if ((currunyw.startx_ + currunyw.runlength_)
						== (currunyw1.startx_ + currunyw1.runlength_)) {

					endx = currunyw.startx_ + currunyw.runlength_;

					cury_i++;
					if ((cury_i == (int) tempruns_->size())
							|| ((*tempruns_)[cury_i].starty_ != y)) {
						cury_i = -1;
					}

					cury1_i++;
					if ((cury1_i == (int) tempruns_->size())
							|| ((*tempruns_)[cury1_i].starty_ != (y - 1))) {
						cury1_i = -1;
					}

					break;
				}

				if ((currunyw.startx_ + currunyw.runlength_)
						< (currunyw1.startx_ + currunyw1.runlength_)) {

					cury_i++;
					if ((cury_i == (int) tempruns_->size())
							|| ((*tempruns_)[cury_i].starty_ != y)) {
						cury_i = -1;
						endx = currunyw1.startx_ + currunyw1.runlength_;

						cury1_i++;
						if ((cury1_i == (int) tempruns_->size())
								|| ((*tempruns_)[cury1_i].starty_ != (y - 1))) {
							cury1_i = -1;
						}

						break;
					} else {

						if ((*tempruns_)[cury_i].startx_
								> (currunyw1.startx_ + currunyw1.runlength_)) {
							endx = currunyw1.startx_ + currunyw1.runlength_;
							cury1_i++;
							if ((cury1_i == (int) tempruns_->size())
									|| ((*tempruns_)[cury1_i].starty_ != (y - 1))) {
								cury1_i = -1;
							}
							break;
						}
					}

					continue;
				}

				cury1_i++;
				if ((cury1_i == (int) tempruns_->size())
						|| ((*tempruns_)[cury1_i].starty_ != (y - 1))) {
					cury1_i = -1;
					endx = currunyw.startx_ + currunyw.runlength_;

					cury_i++;
					if ((cury_i == (int) tempruns_->size())
							|| ((*tempruns_)[cury_i].starty_ != y)) {
						cury_i = -1;
					}

					break;
				} else {

					if ((*tempruns_)[cury1_i].startx_
							> (currunyw.startx_ + currunyw.runlength_)) {
						endx = currunyw.startx_ + currunyw.runlength_;
						cury_i++;
						if ((cury_i == (int) tempruns_->size())
								|| ((*tempruns_)[cury_i].starty_ != y)) {
							cury_i = -1;
						}
						break;
					}
				}

			}

			runs_->emplace_back(RLERunS(startx, y, endx - startx));
		}
	}

	tempruns_->clear();
}
