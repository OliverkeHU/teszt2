#include "LPReader.h"
#include "BinRLE.h"
#include "MipMapper.h"
#include "NetImage.h"
#include "PoligonDraw.h"
#include "MotionFilterRLE.h"
#include "NeuralNet_f16_16.h"
#include "SharedNetValueBuffer.h"
#include "HAData.h"
#include "ANPROCR.h"
#include "ImageTransform.h"

#include <string.h>

CLPReader::CLPReader() {
	imgtran_ = nullptr;

	cm3image_ = nullptr;
	lsplineimg_ = nullptr;
	msimage_ = nullptr;

	detectionareapoints_.reset();

	cm3topfilternet_valuebufferarray_ = nullptr;

	ocrcandidates_ = nullptr;

	msnet_valuebufferarray_ = nullptr;
	for (unsigned int ms_i = 0; ms_i < 4; ms_i++) {
		msnetarray_[ms_i] = nullptr;
	}

	for (unsigned int tf_i = 0; tf_i < 5; tf_i++) {
		cm3topfilternetarray_[tf_i] = nullptr;
	}

	for (unsigned int da_i = 0; da_i < ( LPREADER_MAXSEARCHLEVEL + 1); da_i++) {

		chartopfilterRLE_[da_i] = nullptr;

		detareaRLE_[da_i] = nullptr;
		motionfilter_[da_i] = nullptr;

		avgA_[da_i] = nullptr;
		avgB_[da_i] = nullptr;
	}

	charsizemin_ = 0;
	charsizemax_ = 0;
	SetCharSizes(16, 128);

	ocr_ = nullptr;

	linesearchposvect_ = nullptr;

	for (unsigned int ms_i = 0; ms_i < 5; ms_i++) {
		msnetruncount_[ms_i] = 0;
		msnetpasscount_[ms_i] = 0;
	}
	msrunposcount_ = 0;
	msconthrcount1_ = 0;
	msconthrcount2_ = 0;
	secondaryOCRcount_ = 0;
	secondaryOCRfilteredcount_ = 0;

	fclimage_ = nullptr;
	fclimagestats_ = nullptr;
	fclimagemap_ = nullptr;

	lstopnet_valuebufferarray_ = nullptr;
	lstopnetarray_[0] = nullptr;
	lstopnetarray_[1] = nullptr;
	lsbottomnet_valuebufferarray_ = nullptr;
	lsbottomnetarray_[0] = nullptr;
	lsbottomnetarray_[1] = nullptr;

	lstopvalues_ = nullptr;
	lsbottomvalues_ = nullptr;
	lsvaluescount_ = 0;
}

bool CLPReader::Init(CHAData *_lprdata) {
	CHAData *lprparams = _lprdata->GetHAData("LPRparams");

	mipmapper_ = nullptr;
	detectionareapoints_.reset();

	detareachanged_ = true;

	lastimagewidth_ = 0;
	lastimageheight_ = 0;
	motionfilterthreshold_ = 12;

	ocr_ = new CANPROCR;
	ocr_->Init(_lprdata);

	for (unsigned int da_i = 0; da_i < ( LPREADER_MAXSEARCHLEVEL + 1); da_i++) {
		detareaRLE_[da_i] = new CBinRLE;
		motionfilter_[da_i] = new CMotionFilterRLE;

		avgA_[da_i] = new CNetImage;
		avgB_[da_i] = new CNetImage;

		chartopfilterRLE_[da_i] = new CBinRLE;
	}

	cm3image_ = new CNetImage;
	cm3image_->Create(18, 16);

	msimage_ = new CNetImage;
	msimage_->Create(18, 20);

	lsplineimg_ = new CNetImage;

	ocrcandidates_ = new std::vector<OCRCandidateS>;

	InitTopFilterNets(_lprdata);

	InitMiddleSearchNets(_lprdata);

	InitLineSearchNets(_lprdata);

	mscontthr1_ = (unsigned char) (100
			* lprparams->GetFloat("LPRmscontthr1", 0.15f));
	mscontthr2_ = (unsigned char) (100
			* lprparams->GetFloat("LPRmscontthr2", 0.15f));
	secondarystrengththr_ = lprparams->GetFloat("LPRsecondarystrengththr",
			0.0f);
	lsptopadjust_ = lprparams->GetFloat("LPRlsptopadjust", 0.5f);
	lspbottomadjust_ = lprparams->GetFloat("LPRlspbottomadjust", 0.5f);
	fclsamplingedgedist_ = lprparams->GetFloat("FCLsamplingedgedist", 0.0f);

	imgtran_ = new CImageTransform();

	linesearchposvect_ = new std::vector<LineSearchPosS>;

	fclimage_ = new CNetImage;
	fclimagestats_ = new CNetImage;
	fclimagemap_ = new CNetImage;

	return true;
}

bool CLPReader::InitTopFilterNets(CHAData *_lprdata) {

	cm3topfilternet_valuebufferarray_ = new CSharedNetValueBuffer;
	cm3topfilternet_valuebufferarray_->SetAllocationSize(18 * 16 + 9);

	for (unsigned int tf_i = 0; tf_i < 5; tf_i++) {
		cm3topfilternetarray_[tf_i] = new CNeuralNet_f16_16;
		cm3topfilternetarray_[tf_i]->SetValueBuffer(
				cm3topfilternet_valuebufferarray_);

		char tfnetfn[250];
		sprintf(tfnetfn, "cm3tf%c.net", tf_i + 'a');
		if (!LoadNet(cm3topfilternetarray_[tf_i], tfnetfn, _lprdata)) {
			return false;
		}
	}
	for (unsigned int tf_i = 0; tf_i < 5; tf_i++) {
		cm3topfilternetarray_[tf_i]->CheckValuesBuffer();
	}

	topfilterthreshold_[0] = 0.90f;
	topfilterthreshold_[1] = 0.90f;
	topfilterthreshold_[2] = 0.70f;
	topfilterthreshold_[3] = 0.98f;
	topfilterthreshold_[4] = 0.65f;

	return true;
}

bool CLPReader::InitLineSearchNets(CHAData *_lprdata) {

	lstopnet_valuebufferarray_ = new CSharedNetValueBuffer;
	lstopnet_valuebufferarray_->SetAllocationSize(6 * 16);
	lstopnetarray_[0] = new CNeuralNet_f16_16;
	lstopnetarray_[0]->SetValueBuffer(lstopnet_valuebufferarray_);
	if (!LoadNet(lstopnetarray_[0], "LSTop6a.net", _lprdata)) {
		return false;
	}
	lstopnetarray_[1] = new CNeuralNet_f16_16;
	lstopnetarray_[1]->SetValueBuffer(lstopnet_valuebufferarray_);
	if (!LoadNet(lstopnetarray_[1], "LSTop6b.net", _lprdata)) {
		return false;
	}
	lstopnetarray_[0]->CheckValuesBuffer();
	lstopnetarray_[1]->CheckValuesBuffer();

	lsbottomnet_valuebufferarray_ = new CSharedNetValueBuffer;
	lsbottomnet_valuebufferarray_->SetAllocationSize(6 * 16);
	lsbottomnetarray_[0] = new CNeuralNet_f16_16;
	lsbottomnetarray_[0]->SetValueBuffer(lsbottomnet_valuebufferarray_);
	if (!LoadNet(lsbottomnetarray_[0], "LSBottom6a.net", _lprdata)) {
		return false;
	}
	lsbottomnetarray_[1] = new CNeuralNet_f16_16;
	lsbottomnetarray_[1]->SetValueBuffer(lsbottomnet_valuebufferarray_);
	if (!LoadNet(lsbottomnetarray_[1], "LSBottom6b.net", _lprdata)) {
		return false;
	}
	lsbottomnetarray_[0]->CheckValuesBuffer();
	lsbottomnetarray_[1]->CheckValuesBuffer();

	CHAData *lprparams = _lprdata->GetHAData("LPRparams");
	lstopnetthreshold_[0] = lprparams->GetFloat("thr_LSTop6a", 0.31f);
	lstopnetthreshold_[1] = lprparams->GetFloat("thr_LSTop6b", 0.37f);
	lsbottomnetthreshold_[0] = lprparams->GetFloat("thr_LSBottom6a", 0.71f);
	lsbottomnetthreshold_[1] = lprparams->GetFloat("thr_LSBottom6b", 0.38f);

	return true;
}

bool CLPReader::InitMiddleSearchNets(CHAData *_lprdata) {

	msnet_valuebufferarray_ = new CSharedNetValueBuffer;
	msnet_valuebufferarray_->SetAllocationSize(18 * 20 + 9);
	for (unsigned int ms_i = 0; ms_i < 4; ms_i++) {
		msnetarray_[ms_i] = new CNeuralNet_f16_16;
		msnetarray_[ms_i]->SetValueBuffer(msnet_valuebufferarray_);

		char netthrname[50];
		sprintf(netthrname, "LPRms10%c.net", 'a' + ms_i);
		if (!LoadNet(msnetarray_[ms_i], netthrname, _lprdata)) {

			return false;
		}
	}
	for (unsigned int ms_i = 0; ms_i < 4; ms_i++) {
		msnetarray_[ms_i]->CheckValuesBuffer();
	}

	CHAData *lprparams = _lprdata->GetHAData("LPRparams");
	for (unsigned int ms_i = 0; ms_i < 4; ms_i++) {
		char netthrname[50];
		sprintf(netthrname, "thr_LPRms%c", 'a' + ms_i);
		msnetthreshold_[ms_i] = lprparams->GetFloat(netthrname, 0.5f);
	}

	return true;
}

CLPReader::~CLPReader() {
	delete lstopvalues_;
	delete lsbottomvalues_;

	delete lstopnet_valuebufferarray_;
	delete lstopnetarray_[0];
	delete lstopnetarray_[1];
	delete lsbottomnet_valuebufferarray_;
	delete lsbottomnetarray_[0];
	delete lsbottomnetarray_[1];

	delete fclimagemap_;
	delete fclimagestats_;
	delete fclimage_;

	delete linesearchposvect_;

	delete imgtran_;

	delete msimage_;
	delete lsplineimg_;
	delete cm3image_;

	for (unsigned int ms_i = 0; ms_i < 4; ms_i++) {
		delete msnetarray_[ms_i];
	}
	delete msnet_valuebufferarray_;

	delete ocrcandidates_;

	for (unsigned int tf_i = 0; tf_i < 5; tf_i++) {
		delete cm3topfilternetarray_[tf_i];
	}
	delete cm3topfilternet_valuebufferarray_;

	for (unsigned int da_i = 0; da_i < ( LPREADER_MAXSEARCHLEVEL + 1); da_i++) {

		delete chartopfilterRLE_[da_i];

		delete detareaRLE_[da_i];
		delete motionfilter_[da_i];

		delete avgA_[da_i];
		delete avgB_[da_i];
	}

	delete ocr_;
}

bool CLPReader::LoadNet(CNeuralNet_f16_16 *_net, const char*_netfilename,
		CHAData *_initdata) {

	if (_initdata != nullptr) {
		CHAData *netdataroot = _initdata->GetHAData("nets");
		CHAData *netdata = netdataroot->GetHAData(_netfilename);
		if (netdata == nullptr) {
			return false;
		}
		_net->LoadFromStripedHAData(netdata);
	} else {
		return false;
	}
	return true;
}

void CLPReader::SetDetectionArea(const std::vector<int16_t>* const _mask) {
	detectionareapoints_.reset(_mask);

	detareachanged_ = true;
}

void CLPReader::SetSearchLevels(unsigned int _slmin, unsigned int _slmax) {

	if ((slmin_ == _slmin) && (slmax_ == _slmax)) {
		return;
	}

	if (_slmax > LPREADER_MAXSEARCHLEVEL) {
		_slmax = LPREADER_MAXSEARCHLEVEL;
	}

	if (_slmax < _slmin) {
		_slmax = _slmin;
	}

	slmin_ = _slmin;
	slmax_ = _slmax;

	charsizemin_ = 8 * (1 << slmin_);
	charsizemax_ = 8 * (1 << (slmax_ + 1));

	detareachanged_ = true;
}

void CLPReader::GetSearchLevels(unsigned int *_slmin, unsigned int *_slmax) {
	*_slmin = slmin_;
	*_slmax = slmax_;
}

void CLPReader::SetCharSizes(unsigned int _charsizemin,
		unsigned int _charsizemax) {

	if ((charsizemin_ == _charsizemin) && (charsizemax_ == _charsizemax)) {
		return;
	}

	if (_charsizemax > (1 << ( LPREADER_MAXSEARCHLEVEL + 4))) {
		_charsizemax = (1 << ( LPREADER_MAXSEARCHLEVEL + 4));
	}

	if (_charsizemax < _charsizemin) {
		_charsizemax = _charsizemin;
	}

	charsizemin_ = _charsizemin;
	charsizemax_ = _charsizemax;

	slmin_ = 0;
	while (charsizemin_ >= (unsigned int) (8 * (1 << (slmin_ + 1)))) {
		slmin_++;
	}
	slmax_ = slmin_;
	while (charsizemax_ > (unsigned int) (8 * (1 << (slmax_ + 1)))) {
		slmax_++;
	}

	detareachanged_ = true;
}

void CLPReader::GetCharSizes(unsigned int *_charsizemin,
		unsigned int *_charsizemax) const {
	*_charsizemin = charsizemin_;
	*_charsizemax = charsizemax_;
}

void CLPReader::SetMipMapper(CMipMapper *_mipmapper) {
	mipmapper_ = _mipmapper;
	imgtran_->SetMipMapper(mipmapper_);
}

void CLPReader::RunMotionFilters() {
	for (unsigned int sl_i = slmin_; sl_i <= slmax_; sl_i++) {
		motionfilter_[sl_i]->Update(mipmapper_, sl_i + 2,
				motionfilterthreshold_, detareaRLE_[sl_i]);
	}
}

ERRORVAL CLPReader::ProcessImage() {
	if (mipmapper_ == nullptr) {
		return EV_INVALIDARG;
	}

	CNetImage *fullimage = mipmapper_->GetFullMipMap(0);
	bool reschanged = (fullimage->width_ != lastimagewidth_)
			|| (fullimage->height_ != lastimageheight_);

	if (detareachanged_ || (reschanged)) {
		UpdateDetAreaRLE();

		linegroups_.clear();
	}

	RunMotionFilters();

	if (!reschanged) {
		AddLineGroupsToMotionFilter();
	}

	RunAVGABRecount();

	RunTopFilterNet();

	InitLineSearchPosVect();

	RunAngleLineSearch();

	FindCharLines2();

	RunOCROnLSP();

	CreateLineGroups();

	CheckLetterNumberCollisions();

	CreatePlates();

	RunHistogramCalc();

	lastimagewidth_ = fullimage->width_;
	lastimageheight_ = fullimage->height_;

	detareachanged_ = false;
	return EV_SUCCESS;
}

void CLPReader::UpdateDetAreaRLE() {

	if ((detectionareapoints_ != nullptr)
			&& (detectionareapoints_->size() != 0)) {

		CNetImage *fullimage = mipmapper_->GetFullMipMap(0);
		CNetImage poliimg;
		poliimg.Create(fullimage->width_, fullimage->height_);

		int *truepoints = new int[detectionareapoints_->size()];
		for (unsigned int point_i = 0; point_i < detectionareapoints_->size();
				point_i++) {
			truepoints[point_i] = (((int) ((*detectionareapoints_)[point_i]))
					* fullimage->width_ + 16384 / 2) / 16384;
		}

		CPoligonDraw::DrawPoligon(&poliimg, truepoints,
				detectionareapoints_->size() / 2);

		delete[] truepoints;

		CMipMapper tmipmapper;
		tmipmapper.SetSrcImage(&poliimg);

		for (unsigned int sl_i = slmin_; sl_i <= slmax_; sl_i++) {

			CNetImage *mfimage = tmipmapper.GetFullMipMap(sl_i + 2);
			detareaRLE_[sl_i]->CreateFromImage(mfimage, 1);
		}
	} else {

		for (unsigned int sl_i = slmin_; sl_i <= slmax_; sl_i++) {

			CNetImage *mfimage = mipmapper_->GetFullMipMap(sl_i + 2);
			detareaRLE_[sl_i]->CreateFullImage(mfimage->width_,
					mfimage->height_);
		}
	}
}

void CLPReader::Createcm3Image12x4AVG3AtBlockCenter(unsigned int _searchlevel,
		int _blockx, int _blocky) {

	CNetImage *curmipmap = mipmapper_->GetFullMipMap(_searchlevel);
	cm3image_->CreateMap(18, 16, curmipmap->scanline_,
			curmipmap->GetPtr(_blockx * 4 - 7, _blocky * 4 - 6));

	CNetImage *avgmap = mipmapper_->GetFullMipMap(_searchlevel + 2);
	unsigned char *avgptr = avgmap->GetPtr(0, _blocky);
	CNetImage *avgAmap = avgA_[_searchlevel];
	unsigned char *avgAptr = avgAmap->GetPtr(0, _blocky);
	CNetImage *avgBmap = avgB_[_searchlevel];
	unsigned char *avgBptr = avgBmap->GetPtr(0, _blocky);
	for (unsigned int blokk_i = 0; blokk_i < 3; blokk_i++) {
		unsigned char avgvalue = avgptr[_blockx - 1 + blokk_i];
		cm3imageextrachar_[blokk_i * 3] = avgvalue;
		cm3imageextra_[blokk_i * 3] = (float) avgvalue;

		unsigned char avgAvalue = avgAptr[_blockx - 1 + blokk_i];
		cm3imageextrachar_[1 + blokk_i * 3] = avgAvalue;
		cm3imageextra_[1 + blokk_i * 3] = (float) avgAvalue;

		unsigned char avgBvalue = avgBptr[_blockx - 1 + blokk_i];
		cm3imageextrachar_[2 + blokk_i * 3] = avgBvalue;
		cm3imageextra_[2 + blokk_i * 3] = (float) avgBvalue;
	}
}

void CLPReader::RunAVGABRecount() {

	for (unsigned int sl_i = slmin_; sl_i <= slmax_; sl_i++) {

		CNetImage *curavg = mipmapper_->GetFullMipMap(sl_i + 2);
		CNetImage *curavgA = avgA_[sl_i];
		CNetImage *curavgB = avgB_[sl_i];
		CNetImage *cursrc = mipmapper_->GetFullMipMap(sl_i);

		if ((curavgA->width_ != curavg->width_)
				|| (curavgA->height_ != curavg->height_)) {
			curavgA->Create(curavg->width_, curavg->height_);
			curavgA->FillImage(0);
			curavgB->Create(curavg->width_, curavg->height_);
			curavgB->FillImage(0);
		}

		CMotionFilterRLE * curmf = motionfilter_[sl_i];
		CBinRLE *curRLE = curmf->GetMotionArea();
		unsigned int startx;
		unsigned int starty;
		unsigned int runlength;
		curRLE->StartRun();
		while (curRLE->GetNextRun(&startx, &starty, &runlength)) {

			unsigned char *avgptr = curavg->GetPtr(startx, starty);
			unsigned char *avgAptr = curavgA->GetPtr(startx, starty);
			unsigned char *avgBptr = curavgB->GetPtr(startx, starty);
			unsigned char *srcptr[4];
			srcptr[0] = cursrc->GetPtr(startx * 4, starty * 4);
			srcptr[1] = cursrc->GetPtr(startx * 4, starty * 4 + 1);
			srcptr[2] = cursrc->GetPtr(startx * 4, starty * 4 + 2);
			srcptr[3] = cursrc->GetPtr(startx * 4, starty * 4 + 3);

			for (unsigned int rl_i = 0; rl_i < runlength; rl_i++) {

				unsigned char avgval = avgptr[rl_i];
				unsigned int sumabove = 0;
				unsigned int sumbelove = 0;
				unsigned int countabove = 0;
				unsigned int countbelove = 0;

				for (unsigned int y_i = 0; y_i < 4; y_i++) {
					unsigned char *curline = srcptr[y_i];
					for (unsigned int x_i = 0; x_i < 4; x_i++) {
						unsigned char c = curline[(rl_i * 4) + x_i];
						if (c > avgval) {
							sumabove += c;
							countabove++;
						} else if (c < avgval) {
							sumbelove += c;
							countbelove++;
						}
					}
				}

				unsigned char avgA;
				unsigned char avgB;
				if (countabove == 0) {
					avgA = avgval;
				} else {
					avgA = sumabove / countabove;
				}
				if (countbelove == 0) {
					avgB = avgval;
				} else {
					avgB = sumbelove / countbelove;
				}

				avgAptr[rl_i] = avgA;
				avgBptr[rl_i] = avgB;
			}
		}
	}
}

void CLPReader::CreateLineGroups() {

	linegroups_.clear();

	for (unsigned int ocrr_i = 0; ocrr_i < ocrresults_.size(); ocrr_i++) {
		OCRResultS &curocrresult = ocrresults_[ocrr_i];
		if (curocrresult.charcode_ == 0) {
			curocrresult.conflictremoved_ = true;
		} else {
			curocrresult.conflictremoved_ = false;
		}
	}

	for (unsigned int ocrr_i = 0; ocrr_i < ocrresults_.size(); ocrr_i++) {
		OCRResultS &curocrresult = ocrresults_[ocrr_i];
		float middlexsum = curocrresult.middlex_;
		float topysum = curocrresult.topy_;
		float charheightsum = curocrresult.charheight_;

		if (!curocrresult.conflictremoved_) {

			unsigned int samecode = 0;

			for (unsigned int ocrr_i2 = 0; ocrr_i2 < ocrresults_.size();
					ocrr_i2++) {
				OCRResultS &curocrresult2 = ocrresults_[ocrr_i2];
				if ((!curocrresult2.conflictremoved_) && (ocrr_i != ocrr_i2)) {
					if (curocrresult.IsOverlapping(curocrresult2)) {
						if (curocrresult.charcode_ == curocrresult2.charcode_) {
							samecode++;
							middlexsum += curocrresult2.middlex_;
							topysum += curocrresult2.topy_;
							charheightsum += curocrresult2.charheight_;
						}
					}
				}
			}

			if (samecode > 0) {
				curocrresult.strength_ = (curocrresult.strength_ + samecode)
						/ (samecode + 1);
			}
			curocrresult.middlexavg_ = middlexsum / (samecode + 1);
			curocrresult.topyavg_ = topysum / (samecode + 1);
			curocrresult.charheightavg_ = charheightsum / (samecode + 1);
		}
	}

	for (unsigned int ocrr_i = 0; ocrr_i < ocrresults_.size(); ocrr_i++) {
		OCRResultS &curocrresult = ocrresults_[ocrr_i];
		if (!curocrresult.conflictremoved_) {
			for (unsigned int ocrr_i2 = 0; ocrr_i2 < ocrresults_.size();
					ocrr_i2++) {
				if (ocrr_i != ocrr_i2) {
					OCRResultS &curocrresult2 = ocrresults_[ocrr_i2];
					if (!curocrresult2.conflictremoved_) {
						if (curocrresult.IsOverlapping(curocrresult2)) {
							if (curocrresult.strength_
									> curocrresult2.strength_) {
								curocrresult2.conflictremoved_ = true;
							} else {
								curocrresult.conflictremoved_ = true;
								break;
							}
						}
					}
				}
			}
		}
	}

	for (unsigned int ocrr_i = 0; ocrr_i < ocrresults_.size(); ocrr_i++) {
		OCRResultS &curocrresult = ocrresults_[ocrr_i];
		if (!curocrresult.conflictremoved_) {

			linegroups_.emplace_back();
			struct LineGroupS &newgroup = linegroups_.back();
			newgroup.characters_[0] = ocrr_i;
			newgroup.charcount_ = 1;
			newgroup.charheight_ = curocrresult.charheight_;
			newgroup.posx_ = (unsigned int) curocrresult.middlex_;
			newgroup.posy_ = (unsigned int) curocrresult.topy_;

			for (unsigned int charcheck_i = 0;
					charcheck_i < newgroup.charcount_; charcheck_i++) {
				OCRResultS &curcharcheck =
						ocrresults_[newgroup.characters_[charcheck_i]];

				for (unsigned int ocrr_i2 = 0; ocrr_i2 < ocrresults_.size();
						ocrr_i2++) {
					OCRResultS &curocrresult2 = ocrresults_[ocrr_i2];
					if ((!curocrresult2.conflictremoved_)
							&& (ocrr_i != ocrr_i2)) {

						if (curcharcheck.IsNeighbour(curocrresult2)) {

							if (newgroup.charcount_ < LPREADER_MAX_CHAR_PER_LINE) {
								newgroup.characters_[newgroup.charcount_] =
										ocrr_i2;
								newgroup.charcount_++;
								curocrresult2.conflictremoved_ = true;
							} else {

							}
						}
					}
				}
			}
			curocrresult.conflictremoved_ = true;
		}
	}

	for (unsigned int lg_i = 0; lg_i < linegroups_.size(); lg_i++) {
		LineGroupS &curlinegroup = linegroups_[lg_i];
		for (unsigned int char_i = 0; char_i < (curlinegroup.charcount_ - 1);
				char_i++) {
			for (unsigned int char_i2 = char_i + 1;
					char_i2 < curlinegroup.charcount_; char_i2++) {
				if (ocrresults_[curlinegroup.characters_[char_i]].middlexavg_
						> ocrresults_[curlinegroup.characters_[char_i2]].middlexavg_) {
					unsigned int s = curlinegroup.characters_[char_i];
					curlinegroup.characters_[char_i] =
							curlinegroup.characters_[char_i2];
					curlinegroup.characters_[char_i2] = s;
				}
			}
		}
	}

	for (unsigned int lg_i = 0; lg_i < linegroups_.size(); lg_i++) {
		LineGroupS &curlinegroup = linegroups_[lg_i];
		float chsum = 0.0f;
		for (unsigned int char_i = 0; char_i < curlinegroup.charcount_;
				char_i++) {
			chsum +=
					ocrresults_[curlinegroup.characters_[char_i]].charheightavg_;
		}
		curlinegroup.charheight_ = chsum / curlinegroup.charcount_;
	}

	for (unsigned int lg_i = 0; lg_i < linegroups_.size(); lg_i++) {
		LineGroupS &curlinegroup = linegroups_[lg_i];
		for (unsigned int char_i = 0; char_i < curlinegroup.charcount_;
				char_i++) {
			curlinegroup.text_[char_i] =
					ocrresults_[curlinegroup.characters_[char_i]].charcode_;
		}
		curlinegroup.text_[curlinegroup.charcount_] = 0;
	}

	for (unsigned int lg_i = 0; lg_i < linegroups_.size(); lg_i++) {
		LineGroupS &curlinegroup = linegroups_[lg_i];

		for (unsigned int char_i = 0; char_i < curlinegroup.charcount_;
				char_i++) {
			curlinegroup.characters2_[char_i] = -1;
			curlinegroup.text2_[char_i] = ' ';
		}
		curlinegroup.text2_[curlinegroup.charcount_] = 0;

		for (unsigned int char_i = 0; char_i < curlinegroup.charcount_;
				char_i++) {
			OCRResultS &lgocrresult =
					ocrresults_[curlinegroup.characters_[char_i]];

			float maxconflictstrength_ = 0;
			unsigned int conflictindex = (unsigned int) -1;

			for (unsigned int ocrr_i = 0; ocrr_i < ocrresults_.size();
					ocrr_i++) {
				OCRResultS &curocrresult = ocrresults_[ocrr_i];
				if ((lgocrresult.charcode_ != curocrresult.charcode_)
						&& (curocrresult.conflictremoved_)) {
					if (curocrresult.IsOverlapping(lgocrresult)) {

						if (maxconflictstrength_ < curocrresult.strength_) {
							maxconflictstrength_ = curocrresult.strength_;
							conflictindex = ocrr_i;
						}
					}
				}
			}

			if (conflictindex != (unsigned int) -1) {
				OCRResultS &confocrresult = ocrresults_[conflictindex];

				if ((lgocrresult.strength_ - confocrresult.strength_)
						>= (1.0f - secondarystrengththr_)) {
					secondaryOCRfilteredcount_ += 1;
				} else {
					curlinegroup.characters2_[char_i] = conflictindex;
					curlinegroup.text2_[char_i] =
							ocrresults_[conflictindex].charcode_;
					secondaryOCRcount_ += 1;
				}
			}
		}
	}

	for (unsigned int lg_i = 0; lg_i < linegroups_.size(); lg_i++) {
		LineGroupS &curlinegroup = linegroups_[lg_i];
		for (unsigned int char_i = 0; char_i < curlinegroup.charcount_;
				char_i++) {
			OCRResultS &curocrresult =
					ocrresults_[curlinegroup.characters_[char_i]];
			if (curlinegroup.posx_ > curocrresult.middlex_) {
				curlinegroup.posx_ = (unsigned int) curocrresult.middlex_;
				curlinegroup.posy_ = (unsigned int) curocrresult.topy_;
			}
		}
	}

	for (unsigned int lg_i = 0; lg_i < linegroups_.size(); lg_i++) {
		LineGroupS &curlinegroup = linegroups_[lg_i];

		float x0 = ocrresults_[curlinegroup.characters_[0]].middlex_;
		float y0 = ocrresults_[curlinegroup.characters_[0]].topy_;
		float x1 = x0;
		float y1 = y0;
		for (unsigned int char_i = 0; char_i < curlinegroup.charcount_;
				char_i++) {
			OCRResultS &curocrresult =
					ocrresults_[curlinegroup.characters_[char_i]];
			if (x0 > curocrresult.middlex_) {
				x0 = curocrresult.middlex_;
				y0 = curocrresult.topy_;
			}
			if (x1 < curocrresult.middlex_) {
				x1 = curocrresult.middlex_;
				y1 = curocrresult.topy_;
			}
		}

		float charheight = curlinegroup.charheight_;

		curlinegroup.histogramframe_[0].x = x0;
		curlinegroup.histogramframe_[0].y = y0;
		curlinegroup.histogramframe_[1].x = x1;
		curlinegroup.histogramframe_[1].y = y1;
		curlinegroup.histogramframe_[2].x = x1;
		curlinegroup.histogramframe_[2].y = y1 + charheight;
		curlinegroup.histogramframe_[3].x = x0;
		curlinegroup.histogramframe_[3].y = y0 + charheight;

		curlinegroup.frame_[0].x = x0 - charheight * 0.8f;
		curlinegroup.frame_[0].y = y0 - charheight * 0.4f;
		curlinegroup.frame_[1].x = x1 + charheight * 0.8f;
		curlinegroup.frame_[1].y = y1 - charheight * 0.4f;
		curlinegroup.frame_[2].x = x1 + charheight * 0.8f;
		curlinegroup.frame_[2].y = y1 + charheight * 1.4f;
		curlinegroup.frame_[3].x = x0 - charheight * 0.8f;
		curlinegroup.frame_[3].y = y0 + charheight * 1.4f;
	}
}

void CLPReader::CreatePlates() {

	plates_.clear();

	for (unsigned int line_i = 0; line_i < linegroups_.size(); line_i++) {
		LineGroupS &curline = linegroups_[line_i];

		if (curline.charcount_ > 3) {
			PlateS newplate;

			newplate.posx_ = curline.posx_;
			newplate.posy_ = curline.posy_;
			newplate.charheight_ = curline.charheight_;
			strcpy(newplate.fulltext_, curline.text_);
			strcpy(newplate.fulltext2_, curline.text2_);

			bool validtext = false;
			for (unsigned int ft_i = 0; ft_i < strlen(newplate.fulltext_);
					ft_i++) {
				char c = newplate.fulltext_[ft_i];
				if ((c != 'I') && (c != '1')) {
					validtext = true;
					break;
				}
			}
			if (!validtext) {
				continue;
			}

			newplate.frame_[0] = curline.frame_[0];
			newplate.frame_[1] = curline.frame_[1];
			newplate.frame_[2] = curline.frame_[2];
			newplate.frame_[3] = curline.frame_[3];

			newplate.histogramframe_[0] = curline.histogramframe_[0];
			newplate.histogramframe_[1] = curline.histogramframe_[1];
			newplate.histogramframe_[2] = curline.histogramframe_[2];
			newplate.histogramframe_[3] = curline.histogramframe_[3];

			plates_.push_back(newplate);
		}
	}
}

void CLPReader::RunTopFilterNet() {

	for (unsigned int sl_i = slmin_; sl_i <= slmax_; sl_i++) {

		CMotionFilterRLE * curmf = motionfilter_[sl_i];
		CBinRLE *curmotionRLE = curmf->GetMotionArea();
		unsigned int startx;
		unsigned int starty;
		unsigned int runlength;

		CBinRLE *curchartopfilterRLE = chartopfilterRLE_[sl_i];
		curchartopfilterRLE->Clear();

		CNetImage *mfimage = mipmapper_->GetFullMipMap(sl_i + 2);

		curmotionRLE->StartRun();
		while (curmotionRLE->GetNextRun(&startx, &starty, &runlength)) {

			if (starty > (mfimage->height_ - 3)) {
				break;
			}

			unsigned int lastaddedx = 500000;

			for (unsigned int rl_i = 0;
					(rl_i < runlength)
							&& ((startx + rl_i) < (mfimage->width_ - 1));
					rl_i++) {

				int leftc = (startx + rl_i) * 4 - 8;
				int topc = starty * 4 - 6;
				if ((leftc < 0) || (topc < 0)) {
					continue;
				}

				if (0 == ((startx + rl_i) % 2)) {
					continue;
				}

				Createcm3Image12x4AVG3AtBlockCenter(sl_i, startx + rl_i,
						starty);

				unsigned char cont1 = cm3imageextrachar_[1]
						- cm3imageextrachar_[2];
				unsigned char cont2 = cm3imageextrachar_[4]
						- cm3imageextrachar_[5];
				unsigned char cont3 = cm3imageextrachar_[7]
						- cm3imageextrachar_[8];

				unsigned char contthr = 15;
				unsigned char contnthr = 15;

				if ((cont1 < contthr) && (cont2 < contthr)
						&& (cont3 < contthr)) {

					unsigned char contn01;
					unsigned char contn12;
					if (cm3imageextrachar_[0] > cm3imageextrachar_[3]) {
						contn01 = cm3imageextrachar_[0] - cm3imageextrachar_[3];
					} else {
						contn01 = cm3imageextrachar_[3] - cm3imageextrachar_[0];
					}
					if (cm3imageextrachar_[3] > cm3imageextrachar_[6]) {
						contn12 = cm3imageextrachar_[3] - cm3imageextrachar_[6];
					} else {
						contn12 = cm3imageextrachar_[6] - cm3imageextrachar_[3];
					}

					if ((contn01 >= contnthr) || (contn12 >= contnthr)) {

					} else {

						continue;
					}
				}

				bool filteredout = false;
				cm3topfilternet_valuebufferarray_->CopyImage(
						cm3image_->imgdata_, cm3image_->width_,
						cm3image_->height_, cm3image_->scanline_);
				cm3topfilternet_valuebufferarray_->CopyValues(18 * 16,
						cm3imageextrachar_, 9);
				for (unsigned char net_i = 0; (net_i < 5) && (!filteredout);
						net_i++) {
					cm3topfilternetarray_[net_i]->EvaluateFromBuffer();
					f16_16 output16_16 =
							cm3topfilternetarray_[net_i]->GetOutputValue();

					float outputfloat = (float) output16_16 / 65536.0f;
					if (outputfloat < topfilterthreshold_[net_i]) {
						filteredout = true;
						break;
					}
				}

				if (filteredout) {
					continue;
				}

				if (lastaddedx == (startx + rl_i - 2)) {
					curchartopfilterRLE->AddRun(startx + rl_i - 1, starty, 1);
				}
				curchartopfilterRLE->AddRun(startx + rl_i, starty, 1);
				lastaddedx = startx + rl_i;
			}
		}

		curchartopfilterRLE->ExpandHorizontal(mfimage->width_);
	}
}

void CLPReader::AddLineGroupsToMotionFilter() {

	for (unsigned int lg_i = 0; lg_i < linegroups_.size(); lg_i++) {
		LineGroupS &curlinegroup = linegroups_[lg_i];

		unsigned int minx = curlinegroup.posx_;
		unsigned int miny = curlinegroup.posy_;
		unsigned int maxx = curlinegroup.posx_;
		unsigned int maxy = curlinegroup.posy_;
		for (unsigned int c_i = 0; c_i < curlinegroup.charcount_; c_i++) {
			OCRResultS &curOCRRes = ocrresults_[curlinegroup.characters_[c_i]];
			if (minx > (unsigned int) curOCRRes.middlex_) {
				minx = (unsigned int) curOCRRes.middlex_;
			}
			if (maxx < (unsigned int) curOCRRes.middlex_) {
				maxx = (unsigned int) curOCRRes.middlex_;
			}
			if (miny > (unsigned int) curOCRRes.topy_) {
				miny = (unsigned int) curOCRRes.topy_;
			}
			if (maxy < (unsigned int) curOCRRes.topy_) {
				maxy = (unsigned int) curOCRRes.topy_;
			}
		}

		for (unsigned int sl_i = slmin_; sl_i <= slmax_; sl_i++) {

			unsigned int charsizesl = ((unsigned int) curlinegroup.charheight_)
					>> sl_i;
			if ((charsizesl < 8) || (charsizesl > 16)) {
				continue;
			}

			unsigned int linestarty = miny >> (sl_i + 2);
			unsigned int lineendy = (maxy >> (sl_i + 2)) + 1;
			unsigned int linestartx = (minx >> (sl_i + 2)) - 1;
			unsigned int lineendx = (maxx >> (sl_i + 2)) + 1;

			CMotionFilterRLE * curmf = motionfilter_[sl_i];
			CBinRLE *curmotionRLE = curmf->GetMotionArea();
			for (unsigned int liney = linestarty; liney <= lineendy; liney++) {
				curmotionRLE->AddRun(linestartx, liney, lineendx - linestartx);
			}
		}
	}
}

void CLPReader::RunHistogramCalc() {

	CNetImage *srcimg = mipmapper_->GetFullMipMap(0);
	for (unsigned int plate_i = 0; plate_i < plates_.size(); plate_i++) {
		PlateS &curplate = plates_[plate_i];

		unsigned int histogramlong[ LPREADER_HISTORY_LENGTH];
		unsigned int histogramtotal = 0;
		for (unsigned int h_i = 0; h_i < LPREADER_HISTORY_LENGTH; h_i++) {
			histogramlong[h_i] = 0;
		}

		const int samplelinecount = 10;
		float decline =
				(curplate.histogramframe_[1].y - curplate.histogramframe_[0].y)
						/ (curplate.histogramframe_[1].x
								- curplate.histogramframe_[0].x);
		for (unsigned int sampleline_i = 0; sampleline_i < samplelinecount;
				sampleline_i++) {
			for (unsigned int sx = (unsigned int) curplate.histogramframe_[0].x;
					sx < (unsigned int) curplate.histogramframe_[1].x; sx++) {
				unsigned int sy = (unsigned int) (curplate.histogramframe_[0].y
						+ decline * (sx - curplate.histogramframe_[0].x)
						+ ((float) curplate.charheight_) * (sampleline_i + 1)
								/ (samplelinecount + 2));

				unsigned char *sptr = srcimg->GetPtr(sx, sy);
				histogramlong[(*sptr) / LPREADER_HISTORY_LENGTH] += 1;
				histogramtotal++;
			}
		}

		for (unsigned int h_i = 0; h_i < LPREADER_HISTORY_LENGTH; h_i++) {
			curplate.histogram_[h_i] = histogramlong[h_i] * 255
					/ histogramtotal;
		}

		unsigned int cursum = 0;
		const int thrlow = 13;
		const int thrhigh = 200;
		int low_i = 0;
		int high_i = 0;
		for (unsigned int h_i = 0; h_i < LPREADER_HISTORY_LENGTH; h_i++) {
			cursum += curplate.histogram_[h_i];
			if ((low_i == 0) && (cursum >= thrlow)) {
				low_i = h_i;
			}
			if (cursum >= thrhigh) {
				high_i = h_i;
				break;
			}
		}

		curplate.histbrws_ = (float) low_i
				- ( LPREADER_HISTORY_LENGTH - high_i - 1);

		if (curplate.histbrws_ < 0) {
			curplate.brhint_ = -1;
		} else if (curplate.histbrws_ > 4) {
			curplate.brhint_ = 1;
		} else {
			curplate.brhint_ = 0;
		}

	}
}

void CLPReader::InitLineSearchPosVect() {

	linesearchposvect_->clear();
	for (unsigned int sl_i = slmin_; sl_i <= slmax_; sl_i++) {
		CBinRLE *curchartopfilterRLE = chartopfilterRLE_[sl_i];
		unsigned int startx;
		unsigned int starty;
		unsigned int runlength;

		curchartopfilterRLE->StartRun();
		while (curchartopfilterRLE->GetNextRun(&startx, &starty, &runlength)) {
			if ((runlength >= 5) && (runlength <= 20)) {
				LineSearchPosS newlsp;
				newlsp.searchlevel_ = sl_i;
				newlsp.x_ = (startx << 2) + (runlength << 1);
				newlsp.topy_ = (starty << 2) - 6;

				linesearchposvect_->push_back(newlsp);
			}
		}
	}
}

void CLPReader::RunAngleLineSearch() {

	const unsigned int sareahalfwidth = 8;
	const int samplingdist = 24;
	const int vertextend = 7;

	for (int lsp_i = (int) linesearchposvect_->size() - 1; lsp_i >= 0;
			lsp_i--) {
		CLPReader::LineSearchPosS &curlsp = (*linesearchposvect_)[lsp_i];

		if (curlsp.searchlevel_ == 0) {

			linesearchposvect_->erase(linesearchposvect_->begin() + lsp_i);
			continue;
		}

		CNetImage *srcimage = mipmapper_->GetFullMipMap(
				curlsp.searchlevel_ - 1);

		if (((curlsp.x_ * 2 - samplingdist - sareahalfwidth) > 500000)
				|| ((curlsp.x_ * 2 + samplingdist + sareahalfwidth)
						>= srcimage->width_)
				|| ((curlsp.topy_ * 2 - vertextend) > 500000)
				|| ((curlsp.topy_ * 2
						+ LPREADER_LINE_SSEARCH_POSITION_HEIGHT * 2 + vertextend)
						>= srcimage->height_)) {
			linesearchposvect_->erase(linesearchposvect_->begin() + lsp_i);
			continue;
		}

		for (unsigned int ca_i = 0;
				ca_i < LPREADER_LINE_SSEARCH_POSITION_HEIGHT * 2; ca_i++) {
			unsigned char *lptr = srcimage->GetPtr(
					curlsp.x_ * 2 - sareahalfwidth, curlsp.topy_ * 2 + ca_i);

			unsigned int avgsum = 0;
			for (unsigned int ax = 0; ax < sareahalfwidth * 2; ax++) {
				avgsum += lptr[ax];
			}
			unsigned char avg = avgsum / (2 * sareahalfwidth);
			curlsp.centeravg_[ca_i] = avg;

			unsigned int avgasum = 0;
			unsigned int avgacount = 0;
			for (unsigned int ax = 0; ax < sareahalfwidth * 2; ax++) {
				if (lptr[ax] >= avg) {
					avgasum += lptr[ax];
					avgacount++;
				}
			}
			unsigned char avga;
			if (avgacount == 0) {
				avga = 128;
			} else {
				avga = avgasum / avgacount;
			}
			curlsp.centeravga_[ca_i] = avga;

			unsigned int varsum = 0;
			for (unsigned int ax = 0; ax < sareahalfwidth * 2; ax++) {
				if (avg >= lptr[ax]) {
					varsum += avg - lptr[ax];
				} else {
					varsum += lptr[ax] - avg;
				}
			}
			unsigned char var = varsum / (2 * sareahalfwidth);
			curlsp.centervar_[ca_i] = var;
		}
	}

	for (int lsp_i = (int) linesearchposvect_->size() - 1; lsp_i >= 0;
			lsp_i--) {
		CLPReader::LineSearchPosS &curlsp = (*linesearchposvect_)[lsp_i];

		if (curlsp.searchlevel_ == 0) {

			linesearchposvect_->erase(linesearchposvect_->begin() + lsp_i);
			continue;
		}

		CNetImage *srcimage = mipmapper_->GetFullMipMap(
				curlsp.searchlevel_ - 1);

		unsigned int matchscore1[vertextend * 2];
		unsigned int matchscore2[vertextend * 2];

		unsigned int samplingcenterx = 0;

		for (unsigned int side = 0; side < 2; side++) {
			switch (side) {
			case 0:

				samplingcenterx = curlsp.x_ * 2 - samplingdist;
				break;
			case 1:

				samplingcenterx = curlsp.x_ * 2 + samplingdist;
			}

			unsigned char stripavga[ LPREADER_LINE_SSEARCH_POSITION_HEIGHT * 2
					+ vertextend * 2];
			unsigned char stripvar[ LPREADER_LINE_SSEARCH_POSITION_HEIGHT * 2
					+ vertextend * 2];

			for (unsigned int ca_i = 0;
					ca_i
							< LPREADER_LINE_SSEARCH_POSITION_HEIGHT * 2
									+ vertextend * 2; ca_i++) {
				int samplingtopy = curlsp.topy_ * 2 - vertextend + ca_i;
				unsigned char *lptr = srcimage->GetPtr(
						samplingcenterx - sareahalfwidth, samplingtopy);

				unsigned int avgsum = 0;
				if (samplingtopy < 0) {
					avgsum = 0;
				} else {
					for (unsigned int ax = 0; ax < sareahalfwidth * 2; ax++) {
						avgsum += lptr[ax];
					}
				}
				unsigned char avg = avgsum / (2 * sareahalfwidth);

				unsigned int avgasum = 0;
				unsigned int avgacount = 0;
				if (samplingtopy < 0) {
				} else {
					for (unsigned int ax = 0; ax < sareahalfwidth * 2; ax++) {
						if (lptr[ax] >= avg) {
							avgasum += lptr[ax];
							avgacount++;
						}
					}
				}
				unsigned char avga;
				if (avgacount == 0) {
					avga = 128;
				} else {
					avga = avgasum / avgacount;
				}

				stripavga[ca_i] = avga;

				unsigned int varsum = 0;
				if (samplingtopy < 0) {
					varsum = 255 * 2 * sareahalfwidth;
				} else {
					for (unsigned int ax = 0; ax < sareahalfwidth * 2; ax++) {
						if (avg >= lptr[ax]) {
							varsum += avg - lptr[ax];
						} else {
							varsum += lptr[ax] - avg;
						}
					}
				}
				unsigned char var = varsum / (2 * sareahalfwidth);
				stripvar[ca_i] = var;
			}

			switch (side) {
			case 0:

				for (unsigned int stripy = 0; stripy < vertextend * 2;
						stripy++) {
					unsigned int curscore = GetMatchScore(curlsp,
							stripavga + stripy, stripvar + stripy);
					matchscore1[stripy] = curscore;
				}
				curlsp.xleft_ = samplingcenterx;
				break;
			case 1:

				for (unsigned int stripy = 0; stripy < vertextend * 2;
						stripy++) {
					unsigned int curscore = GetMatchScore(curlsp,
							stripavga + stripy, stripvar + stripy);
					matchscore2[stripy] = curscore;
				}
				curlsp.xright_ = samplingcenterx;
				break;
			}
		}

		unsigned int totalminscore = 500000;
		unsigned int bestleft = 0;
		unsigned int bestright = 0;
		for (unsigned int stripy1 = 0; stripy1 < vertextend * 2; stripy1++) {
			unsigned int stripy2 = vertextend * 2 - stripy1 - 1;
			unsigned int matchsum;

			matchsum = matchscore1[stripy1] + matchscore2[stripy2];
			if (totalminscore > matchsum) {
				totalminscore = matchsum;
				bestleft = stripy1;
				bestright = stripy2;
			}

			if (stripy1 != (vertextend * 2 - 1)) {
				matchsum = matchscore1[stripy1] + matchscore2[stripy2 - 1];
				if (totalminscore > matchsum) {
					totalminscore = matchsum;
					bestleft = stripy1;
					bestright = stripy2 - 1;
				}
			}

			if (stripy1 != (vertextend * 2)) {
				matchsum = matchscore1[stripy1] + matchscore2[stripy2 + 1];
				if (totalminscore > matchsum) {
					totalminscore = matchsum;
					bestleft = stripy1;
					bestright = stripy2 + 1;
				}
			}
		}
		curlsp.yleft_ = curlsp.topy_ * 2 - vertextend + bestleft;
		curlsp.yright_ = curlsp.topy_ * 2 - vertextend + bestright;

	}
}

unsigned int CLPReader::GetMatchScore(LineSearchPosS &_lsp,
		unsigned char *_stripavg, unsigned char *_stripvar) {

	unsigned int ret = 0;
	for (unsigned int ca_i = 0;
			ca_i < LPREADER_LINE_SSEARCH_POSITION_HEIGHT * 2; ca_i++) {
		unsigned int curdif = 0;
		if (_stripavg[ca_i] > _lsp.centeravga_[ca_i]) {
			curdif = _stripavg[ca_i] - _lsp.centeravga_[ca_i];
		} else {
			curdif = _lsp.centeravga_[ca_i] - _stripavg[ca_i];
		}

		unsigned char curvar;
		if (_stripvar[ca_i] > _lsp.centervar_[ca_i]) {
			curvar = _stripvar[ca_i];
		} else {
			curvar = _lsp.centervar_[ca_i];
		}

		unsigned int curweight = 5;
		if (curvar > 5) {
			curweight = 4;
			if (curvar > 10) {
				curweight = 3;
				if (curvar > 17) {
					curweight = 2;
					if (curvar > 30) {
						curweight = 1;
					}
				}
			}
		}
		ret += curweight * curdif;
	}
	return ret;
}

void CLPReader::FindCharLines() {

	for (int lsp_i = (int) linesearchposvect_->size() - 1; lsp_i >= 0;
			lsp_i--) {
		CLPReader::LineSearchPosS &curlsp = (*linesearchposvect_)[lsp_i];

		if (curlsp.searchlevel_ == 0) {

			curlsp.charlinecount_ = 0;
			continue;
		}

		CNetImage *srcimage = mipmapper_->GetFullMipMap(
				curlsp.searchlevel_ - 1);

		if (((curlsp.yleft_ + LPREADER_FIND_CHAR_SAMPLE_HEIGHT)
				>= srcimage->height_)
				|| ((curlsp.yright_ + LPREADER_FIND_CHAR_SAMPLE_HEIGHT)
						>= srcimage->height_)) {

			curlsp.charlinecount_ = 0;
			continue;
		}

		for (unsigned int cursline = 0;
				cursline < LPREADER_FIND_CHAR_SAMPLE_HEIGHT; cursline++) {

			unsigned int avgsum = 0;
			unsigned int avgcount = 0;
			unsigned int hconsum = 0;
			unsigned char hcv1 = 0;
			unsigned char hcv2 = 0;
			for (unsigned int x = curlsp.xleft_; x < curlsp.xright_; x++) {
				int y = (int) LPRLinTran((float) x, (float) curlsp.xleft_,
						(float) curlsp.xright_, (float) curlsp.yleft_,
						(float) curlsp.yright_);
				y += cursline;
				unsigned char *lptr = srcimage->GetPtr(x, y);
				unsigned char curv = *lptr;
				avgsum += curv;
				avgcount++;

				if ((x - curlsp.xleft_) == 0) {
					hcv1 = curv;
				} else {
					if ((x - curlsp.xleft_) == 1) {
						hcv2 = curv;
					} else {
						unsigned char curdif;
						if (0 == ((x - curlsp.xleft_) % 2)) {
							if (hcv1 > curv) {
								curdif = hcv1 - curv;
							} else {
								curdif = curv - hcv1;
							}
							hcv1 = curv;
						} else {
							if (hcv2 > curv) {
								curdif = hcv2 - curv;
							} else {
								curdif = curv - hcv2;
							}
							hcv2 = curv;
						}
						hconsum += curdif;
					}
				}
			}

			unsigned char avg;
			if (avgcount == 0) {
				avg = 128;
			} else {
				avg = avgsum / avgcount;
			}
			curlsp.fcsampleavg_[cursline] = avg;

			unsigned char hcon = (unsigned char) (hconsum
					/ (curlsp.xright_ - curlsp.xleft_ - 2));
			curlsp.fcsamplehcon_[cursline] = hcon;

			unsigned int varsum = 0;
			unsigned int varcount = 0;
			for (unsigned int x = curlsp.xleft_; x < curlsp.xright_; x++) {
				int y = (int) LPRLinTran((float) x, (float) curlsp.xleft_,
						(float) curlsp.xright_, (float) curlsp.yleft_,
						(float) curlsp.yright_);
				y += cursline;
				unsigned char *lptr = srcimage->GetPtr(x, y);
				if (*lptr > avg) {
					varsum += *lptr - avg;
				} else {
					varsum += avg - *lptr;
				}
				varcount++;
			}
			unsigned char var;
			if (varcount == 0) {
				var = 128;
			} else {
				var = varsum / varcount;
			}

			curlsp.fcsamplevar_[cursline] = var;
		}

		int maxpoint = -300;
		unsigned int maxvary = 0;
		for (unsigned int vy = 0; vy < ( LPREADER_FIND_CHAR_SAMPLE_HEIGHT - 3);
				vy++) {

			int curvardiff1;
			int curvardiff2;
			int curvardiff3;
			int curhcondiff1;
			int curhcondiff2;
			int curhcondiff3;
			int curpoint;

			curvardiff1 = (int) curlsp.fcsamplevar_[vy + 1]
					- (int) curlsp.fcsamplevar_[vy];
			curvardiff2 = (int) curlsp.fcsamplevar_[vy + 2]
					- (int) curlsp.fcsamplevar_[vy];
			curhcondiff1 = (int) curlsp.fcsamplehcon_[vy + 1]
					- (int) curlsp.fcsamplehcon_[vy];
			curhcondiff2 = (int) curlsp.fcsamplehcon_[vy + 2]
					- (int) curlsp.fcsamplehcon_[vy];
			curpoint = (curvardiff1 + curvardiff2) * 3
					+ (curhcondiff1 + curhcondiff2) * 6;
			if (maxpoint < curpoint) {
				maxpoint = curpoint;
				maxvary = vy;
			}

			curvardiff3 = (int) curlsp.fcsamplevar_[vy + 3]
					- (int) curlsp.fcsamplevar_[vy];
			curhcondiff3 = (int) curlsp.fcsamplehcon_[vy + 3]
					- (int) curlsp.fcsamplehcon_[vy];
			curpoint = (curvardiff2 + curvardiff3) * 3
					+ (curhcondiff2 + curhcondiff3) * 6;
			if (maxpoint < curpoint) {
				maxpoint = curpoint;
				maxvary = vy;
			}

		}

		unsigned int cursltoplineyleft = curlsp.yleft_ + maxvary;
		unsigned int cursltoplineyright = curlsp.yright_ + maxvary;

		if (((cursltoplineyleft + LPREADER_FIND_CHAR_MAX_CHAR_HEIGHT + 4)
				>= srcimage->height_)
				|| ((cursltoplineyright + LPREADER_FIND_CHAR_MAX_CHAR_HEIGHT + 4)
						>= srcimage->height_)) {

			curlsp.charlinecount_ = 0;
			continue;
		}

		curlsp.toplineyleft_ = (float) cursltoplineyleft
				* (1 << (curlsp.searchlevel_ - 1));
		curlsp.toplineyright_ = (float) cursltoplineyright
				* (1 << (curlsp.searchlevel_ - 1));

		for (unsigned int cursline = 0;
				cursline
						< ( LPREADER_FIND_CHAR_MAX_CHAR_HEIGHT
								- LPREADER_FIND_CHAR_MIN_CHAR_HEIGHT + 4);
				cursline++) {

			unsigned int avgsum = 0;
			unsigned int avgcount = 0;
			unsigned int hconsum = 0;
			unsigned char hcv1 = 0;
			unsigned char hcv2 = 0;
			for (unsigned int x = curlsp.xleft_; x < curlsp.xright_; x++) {
				int y = (int) LPRLinTran((float) x, (float) curlsp.xleft_,
						(float) curlsp.xright_,
						(float) cursltoplineyleft
								+ LPREADER_FIND_CHAR_MIN_CHAR_HEIGHT - 3,
						(float) cursltoplineyright
								+ LPREADER_FIND_CHAR_MIN_CHAR_HEIGHT - 3);
				y += cursline;
				unsigned char *lptr = srcimage->GetPtr(x, y);
				unsigned char curv = *lptr;
				avgsum += curv;
				avgcount++;

				if ((x - curlsp.xleft_) == 0) {
					hcv1 = curv;
				} else {
					if ((x - curlsp.xleft_) == 1) {
						hcv2 = curv;
					} else {
						unsigned char curdif;
						if (0 == ((x - curlsp.xleft_) % 2)) {
							if (hcv1 > curv) {
								curdif = hcv1 - curv;
							} else {
								curdif = curv - hcv1;
							}
							hcv1 = curv;
						} else {
							if (hcv2 > curv) {
								curdif = hcv2 - curv;
							} else {
								curdif = curv - hcv2;
							}
							hcv2 = curv;
						}
						hconsum += curdif;
					}
				}
			}

			unsigned char avg;
			if (avgcount == 0) {
				avg = 128;
			} else {
				avg = avgsum / avgcount;
			}
			curlsp.fcbottomavg_[cursline] = avg;

			unsigned char hcon = (unsigned char) (hconsum
					/ (curlsp.xright_ - curlsp.xleft_ - 2));
			curlsp.fcbottomhcon_[cursline] = hcon;

			unsigned int varsum = 0;
			unsigned int varcount = 0;
			for (unsigned int x = curlsp.xleft_; x < curlsp.xright_; x++) {
				int y = (int) LPRLinTran((float) x, (float) curlsp.xleft_,
						(float) curlsp.xright_,
						(float) cursltoplineyleft
								+ LPREADER_FIND_CHAR_MIN_CHAR_HEIGHT - 3,
						(float) cursltoplineyright
								+ LPREADER_FIND_CHAR_MIN_CHAR_HEIGHT - 3);
				y += cursline;
				unsigned char *lptr = srcimage->GetPtr(x, y);
				if (*lptr > avg) {
					varsum += *lptr - avg;
				} else {
					varsum += avg - *lptr;
				}
				varcount++;
			}
			unsigned char var;
			if (varcount == 0) {
				var = 128;
			} else {
				var = varsum / varcount;
			}

			curlsp.fcbottomvar_[cursline] = var;
		}

		maxpoint = -300;
		maxvary = 0;
		for (unsigned int vy = 3;
				vy
						< ( LPREADER_FIND_CHAR_MAX_CHAR_HEIGHT
								- LPREADER_FIND_CHAR_MIN_CHAR_HEIGHT + 4);
				vy++) {

			int curvardiff1;
			int curvardiff2;
			int curvardiff3;
			int curhcondiff1;
			int curhcondiff2;
			int curhcondiff3;
			int curpoint;

			curvardiff1 = (int) curlsp.fcbottomvar_[vy - 1]
					- (int) curlsp.fcbottomvar_[vy];
			curvardiff2 = (int) curlsp.fcbottomvar_[vy - 2]
					- (int) curlsp.fcbottomvar_[vy];
			curhcondiff1 = (int) curlsp.fcbottomhcon_[vy - 1]
					- (int) curlsp.fcbottomhcon_[vy];
			curhcondiff2 = (int) curlsp.fcbottomhcon_[vy - 2]
					- (int) curlsp.fcbottomhcon_[vy];
			curpoint = (curvardiff1 + curvardiff2) * 3
					+ (curhcondiff1 + curhcondiff2) * 6;
			if (maxpoint < curpoint) {
				maxpoint = curpoint;
				maxvary = vy;
			}

			curvardiff3 = (int) curlsp.fcbottomvar_[vy - 3]
					- (int) curlsp.fcbottomvar_[vy];
			curhcondiff3 = (int) curlsp.fcbottomhcon_[vy - 3]
					- (int) curlsp.fcbottomhcon_[vy];
			curpoint = (curvardiff2 + curvardiff3) * 3
					+ (curhcondiff2 + curhcondiff3) * 6;
			if (maxpoint < curpoint) {
				maxpoint = curpoint;
				maxvary = vy;
			}

		}

		curlsp.bottomlineyleft_ = curlsp.toplineyleft_
				+ ( LPREADER_FIND_CHAR_MIN_CHAR_HEIGHT + maxvary - 4)
						* (1 << (curlsp.searchlevel_ - 1));
		curlsp.bottomlineyright_ = curlsp.toplineyright_
				+ ( LPREADER_FIND_CHAR_MIN_CHAR_HEIGHT + maxvary - 4)
						* (1 << (curlsp.searchlevel_ - 1));

		curlsp.charlinecount_ = 1;
	}

	for (int lsp_i = (int) linesearchposvect_->size() - 1; lsp_i >= 0;
			lsp_i--) {
		CLPReader::LineSearchPosS &curlsp = (*linesearchposvect_)[lsp_i];
		if (curlsp.charlinecount_ == 1) {
			curlsp.linexleft_ = (float) curlsp.xleft_
					* (1 << (curlsp.searchlevel_ - 1));
			curlsp.linexright_ = (float) curlsp.xright_
					* (1 << (curlsp.searchlevel_ - 1));
		}
	}

	for (int lsp_i = (int) linesearchposvect_->size() - 1; lsp_i >= 0;
			lsp_i--) {
		CLPReader::LineSearchPosS &curlsp = (*linesearchposvect_)[lsp_i];
		if (curlsp.charlinecount_ == 1) {
			float topadjust = (curlsp.bottomlineyleft_ - curlsp.toplineyleft_)
					* (lsptopadjust_ - 0.5f) / 2.0f;
			float bottomadjust =
					(curlsp.bottomlineyleft_ - curlsp.toplineyleft_)
							* (lspbottomadjust_ - 0.5f) / 2.0f;
			curlsp.toplineyleft_ += topadjust;
			curlsp.toplineyright_ += topadjust;
			curlsp.bottomlineyleft_ += bottomadjust;
			curlsp.bottomlineyright_ += bottomadjust;
		}
	}

	for (int lsp_i = (int) linesearchposvect_->size() - 1; lsp_i >= 0;
			lsp_i--) {
		CLPReader::LineSearchPosS &curlsp = (*linesearchposvect_)[lsp_i];
		if (curlsp.charlinecount_ == 1) {
			float charheight = curlsp.bottomlineyleft_ - curlsp.toplineyleft_;
			float newlspwidth = charheight * 2.0f;

			float centerx = (curlsp.linexright_ + curlsp.linexleft_) / 2.0f;

			float newlinexleft = centerx - newlspwidth / 2.0f;
			float newlinexright = newlinexleft + newlspwidth;

			float newtoplineyleft = LPRLinTran(newlinexleft, curlsp.linexleft_,
					curlsp.linexright_, curlsp.toplineyleft_,
					curlsp.toplineyright_);
			float newtoplineyright = LPRLinTran(newlinexright,
					curlsp.linexleft_, curlsp.linexright_, curlsp.toplineyleft_,
					curlsp.toplineyright_);

			curlsp.linexleft_ = newlinexleft;
			curlsp.linexright_ = newlinexright;
			curlsp.toplineyleft_ = newtoplineyleft;
			curlsp.toplineyright_ = newtoplineyright;
			curlsp.bottomlineyleft_ = curlsp.toplineyleft_ + charheight;
			curlsp.bottomlineyright_ = curlsp.toplineyright_ + charheight;
		}
	}
}

void CLPReader::RunOCROnLSP() {

	ocrcandidates_->clear();
	ocrresults_.clear();

	for (unsigned int lsp_i = 0; lsp_i < linesearchposvect_->size(); lsp_i++) {
		CLPReader::LineSearchPosS &curlsp = (*linesearchposvect_)[lsp_i];
		if (curlsp.charlinecount_ == 0) {
			continue;
		}

		float charheight = curlsp.bottomlineyleft_ - curlsp.toplineyleft_;
		float linexleft = curlsp.linexleft_;
		float toplineyleft = curlsp.toplineyleft_;
		float linexright = curlsp.linexright_;
		float toplineyright = curlsp.toplineyright_;

		unsigned int ocrcount = RunOCROnLine(linexleft, toplineyleft,
				linexright, toplineyright, charheight, lsp_i);

		if (ocrcount == 0) {
			continue;
		}

		float prevxright = linexright;
		float prevyright = toplineyright;
		curlsp.exttoplinexright_ = linexright;
		do {
			float ext_xright = prevxright + charheight * 3 / 2;
			float ext_topliney = LPRLinTran(ext_xright, linexleft, linexright,
					toplineyleft, toplineyright);
			ocrcount = RunOCROnLine(prevxright, prevyright, ext_xright,
					ext_topliney, charheight, lsp_i);
			prevxright = ext_xright;
			prevyright = ext_topliney;
			curlsp.exttoplinexright_ = ext_xright;
		} while (0 != ocrcount);

		float prevxleft = linexleft;
		float prevyleft = toplineyleft;
		curlsp.exttoplinexleft_ = linexleft;
		do {
			if (prevxleft < (charheight * 3 / 2)) {
				break;
			}
			float ext_xleft = prevxleft - charheight * 3 / 2;
			float ext_topliney = LPRLinTran(ext_xleft, linexleft, linexright,
					toplineyleft, toplineyright);
			ocrcount = RunOCROnLine(ext_xleft, ext_topliney, prevxleft,
					prevyleft, charheight, lsp_i);
			prevxleft = ext_xleft;
			prevyleft = ext_topliney;
			curlsp.exttoplinexleft_ = ext_xleft;
		} while (0 != ocrcount);
	}
}

bool CLPReader::RunMiddleNetAtPos() {

	msrunposcount_++;

	{

		unsigned int sum;
		unsigned int sumabove;
		unsigned int sumbelove;
		unsigned int countabove;
		unsigned int countbelove;
		unsigned char avgval;

		for (unsigned int blokk_i = 0; blokk_i < 3; blokk_i++) {

			sum = 0;
			for (unsigned int y_i = 6; y_i <= 9; y_i++) {
				unsigned char* lines = msimage_->GetPtr(0, y_i);
				for (unsigned int x_i = 3 + blokk_i * 4;
						x_i <= (6 + blokk_i * 4); x_i++) {
					unsigned char c = lines[x_i];
					sum += c;
				}
			}
			msimageextra_[0 + blokk_i * 3] = ((float) sum) / 16.0f;

			sumabove = 0;
			sumbelove = 0;
			countabove = 0;
			countbelove = 0;
			avgval = (unsigned char) msimageextra_[0 + blokk_i * 3];

			for (unsigned int y_i = 6; y_i <= 9; y_i++) {
				unsigned char* lines = msimage_->GetPtr(0, y_i);
				for (unsigned int x_i = 3 + blokk_i * 4;
						x_i <= (6 + blokk_i * 4); x_i++) {
					unsigned char c = lines[x_i];

					if (c > avgval) {
						sumabove += c;
						countabove++;
					} else if (c < avgval) {
						sumbelove += c;
						countbelove++;
					}
				}
			}
			if (countabove == 0) {
				msimageextra_[1 + blokk_i * 3] = msimageextra_[0 + blokk_i * 3];
			} else {
				msimageextra_[1 + blokk_i * 3] = (float) sumabove
						/ (float) countabove;
			}
			if (countbelove == 0) {
				msimageextra_[2 + blokk_i * 3] = msimageextra_[0 + blokk_i * 3];
			} else {
				msimageextra_[2 + blokk_i * 3] = (float) sumbelove
						/ (float) countbelove;
			}
		}

		for (unsigned int e_i = 0; e_i < 9; e_i++) {
			msimageextrachar_[e_i] = (unsigned char) msimageextra_[e_i];
		}
	}

	unsigned char cont1 = msimageextrachar_[1] - msimageextrachar_[2];
	unsigned char cont2 = msimageextrachar_[4] - msimageextrachar_[5];
	unsigned char cont3 = msimageextrachar_[7] - msimageextrachar_[8];

	if ((cont1 < mscontthr1_) && (cont2 < mscontthr1_)
			&& (cont3 < mscontthr1_)) {
		msconthrcount1_++;

		unsigned char contn01;
		unsigned char contn12;
		if (msimageextrachar_[0] > msimageextrachar_[3]) {
			contn01 = msimageextrachar_[0] - msimageextrachar_[3];
		} else {
			contn01 = msimageextrachar_[3] - msimageextrachar_[0];
		}
		if (msimageextrachar_[3] > msimageextrachar_[6]) {
			contn12 = msimageextrachar_[3] - msimageextrachar_[6];
		} else {
			contn12 = msimageextrachar_[6] - msimageextrachar_[3];
		}

		if ((contn01 >= mscontthr2_) || (contn12 >= mscontthr2_)) {

		} else {

			msconthrcount2_++;
			return false;
		}
	}

	msnet_valuebufferarray_->CopyImage(msimage_->imgdata_, msimage_->width_,
			msimage_->height_, msimage_->scanline_);
	msnet_valuebufferarray_->CopyValues(18 * 20, msimageextrachar_, 9);

	for (unsigned int ms_i = 0; ms_i < 4; ms_i++) {
		if (nullptr == msnetarray_[ms_i]) {
			break;
		}
		msnetarray_[ms_i]->EvaluateFromBuffer();
		msnetruncount_[ms_i] = msnetruncount_[ms_i] + 1;
		f16_16 output16_16 = msnetarray_[ms_i]->GetOutputValue();

		float outputfloat = (float) output16_16 / 65536.0f;
		if (outputfloat < msnetthreshold_[ms_i]) {
			return false;
		}
		msnetpasscount_[ms_i] = msnetpasscount_[ms_i] + 1;
	}

	return true;
}

unsigned int CLPReader::RunOCROnLine(float _toplinexleft, float _toplineyleft,
		float _toplinexright, float _toplineyright, float _charheight,
		unsigned int _checklsp_i) {

	unsigned int ocrcount = 0;

	unsigned int samwidth = (unsigned int) ((_toplinexright - _toplinexleft)
			* 14 / _charheight + 0.5f);
	lsplineimg_->Create(9 + samwidth + 9 + 1, 20);

	CPointf16_16 p0s;
	CPointf16_16 p1s;
	CPointf16_16 p0d;
	CPointf16_16 p1d;
	p0s.Set((f16_16) (_toplinexleft * 65536.0f),
			(f16_16) (_toplineyleft * 65536.0f));
	p1s.Set((f16_16) (_toplinexleft * 65536.0f),
			(f16_16) ((_toplineyleft + _charheight) * 65536.0f));
	p0d.Set(9 << 16, 3 << 16);
	p1d.Set(9 << 16, 17 << 16);

	imgtran_->Set(p0s, p1s, p0d, p1d);
	f16_16 mx;
	mx = (f16_16) ((_toplineyright - _toplineyleft)
			/ (_toplinexright - _toplinexleft) * 65536.0f);
	imgtran_->Shear(mx, 0);
	imgtran_->TransformTrilinear(*lsplineimg_);

	float imgwidth = (float) ((mipmapper_->GetFullMipMap(0))->width_);
	float imgheight = (float) ((mipmapper_->GetFullMipMap(0))->height_);
	for (unsigned int mscol = 0; mscol <= lsplineimg_->width_ - 18; mscol++) {

		float ocrcenterx = (float) LPRLinTran((float) mscol + 9.0f, 9.0f,
				(float) samwidth + 9.0f, _toplinexleft, _toplinexright);
		float ocrtopy = (float) LPRLinTran((float) mscol + 9.0f, 9.0f,
				(float) samwidth + 9.0f, _toplineyleft, _toplineyright);

		if ((ocrcenterx < 20.0f) || (ocrcenterx > (imgwidth - 20.0f))) {
			continue;
		}

		float ocrbottomy = ocrtopy + _charheight;

		if ((ocrtopy < 10.0f) || (ocrbottomy > (imgheight - 10.f))) {
			continue;
		}

		if (CheckIfFitsLSP(ocrcenterx, ocrtopy, _charheight, _checklsp_i)) {
			continue;
		}

		msimage_->CreateMap(18, 20, lsplineimg_->scanline_,
				lsplineimg_->GetPtr(mscol, 0));

		bool middlenetresult = RunMiddleNetAtPos();

		if (!middlenetresult) {
			continue;
		}

		ocrcandidates_->emplace_back(
				OCRCandidateS(ocrtopy, ocrbottomy, ocrcenterx));

		ocr_->Process(msimage_, msimageextrachar_);

		unsigned char charcode1;
		float strength1;
		unsigned char charcode2;
		float strength2;
		if (ocr_->GetCharCodes(&charcode1, &strength1, &charcode2,
				&strength2)) {

			ocrresults_.emplace_back();
			OCRResultS &newresult = ocrresults_.back();
			newresult.middlex_ = ocrcenterx;
			newresult.topy_ = ocrtopy;
			newresult.charheight_ = ocrbottomy - ocrtopy;
			newresult.charcode_ = charcode1;
			newresult.strength_ = strength1;

			ocrcount++;

			if (charcode2 != 0) {

				ocrresults_.emplace_back();
				OCRResultS &newresult2 = ocrresults_.back();
				newresult2.middlex_ = ocrcenterx;
				newresult2.topy_ = ocrtopy;
				newresult2.charheight_ = ocrbottomy - ocrtopy;
				newresult2.charcode_ = charcode2;
				newresult2.strength_ = strength2;

				ocrcount++;
			}
		}

	}

	return ocrcount;
}

bool CLPReader::CheckIfFitsLSP(float _x, float _topy, float _charheight,
		unsigned int _checklsp_i) {

	for (unsigned int lsp_i = 0; lsp_i < _checklsp_i; lsp_i++) {
		CLPReader::LineSearchPosS &curlsp = (*linesearchposvect_)[lsp_i];

		if (curlsp.charlinecount_ == 0) {
			continue;
		}

		float lspcharheight = curlsp.bottomlineyleft_ - curlsp.toplineyleft_;
		float charheightdiff = (float) lspcharheight - _charheight;
		if (charheightdiff < 0) {
			charheightdiff = -charheightdiff;
		}
		float charheightdiffpercent = charheightdiff / _charheight;
		if (charheightdiffpercent > 0.09f) {
			continue;
		}

		if ((curlsp.exttoplinexleft_ > _x) || (curlsp.exttoplinexright_ < _x)) {
			continue;
		}

		int lsptopliney = (int) LPRLinTran((float) _x, curlsp.linexleft_,
				curlsp.linexright_, curlsp.toplineyleft_,
				curlsp.toplineyright_);
		float ydiff = (float) lsptopliney - _topy;
		if (ydiff < 0) {
			ydiff = -ydiff;
		}
		float ydiffpercent = ydiff / _charheight;
		if (ydiffpercent < 0.05f) {
			return true;
		}
	}
	return false;
}

bool CLPReader::SaveFormatList(const char *_filename) {

	FILE *fh = fopen(_filename, "wb");
	if (fh == nullptr) {
		return false;
	}

	for (unsigned int plate_i = 0; plate_i < plates_.size(); plate_i++) {
		PlateS &curplate = plates_[plate_i];
		for (unsigned int format_i = 0; format_i < curplate.formats_.size();
				format_i++) {
			LNPFormatS &curformat = curplate.formats_[format_i];
			char aline[200];
			sprintf(aline, "%s  %s\r\n", curformat.text_,
					curformat.formatname_);
			fwrite(aline, 1, strlen(aline), fh);
		}
	}

	fclose(fh);
	return true;
}

void CLPReader::CheckLetterNumberCollisions() {

	for (unsigned int lg_i = 0; lg_i < linegroups_.size(); lg_i++) {
		LineGroupS &curlinegroup = linegroups_[lg_i];

		bool isnumc1[ LPREADER_MAX_CHAR_PER_LINE];
		bool isnumc2[ LPREADER_MAX_CHAR_PER_LINE];

		bool needcheck[ LPREADER_MAX_CHAR_PER_LINE];

		bool hasanyconflict = false;
		for (unsigned int c_i = 0; c_i < curlinegroup.charcount_; c_i++) {
			char c1 = curlinegroup.text_[c_i];
			char c2 = curlinegroup.text2_[c_i];
			if ((c1 >= '0') && (c1 <= '9')) {
				isnumc1[c_i] = true;
			} else {
				isnumc1[c_i] = false;
			}
			if (c2 == ' ') {
				needcheck[c_i] = false;
			} else {
				if ((c2 >= '0') && (c2 <= '9')) {
					isnumc2[c_i] = true;
				} else {
					isnumc2[c_i] = false;
				}
				if (isnumc1[c_i] == isnumc2[c_i]) {
					needcheck[c_i] = false;
				} else {
					needcheck[c_i] = true;
					hasanyconflict = true;
				}
			}
		}

		if (!hasanyconflict) {
			continue;
		}

		bool inheritleft[ LPREADER_MAX_CHAR_PER_LINE];
		for (unsigned int c_i = 0; c_i < curlinegroup.charcount_; c_i++) {
			if (needcheck[c_i]) {
				if (c_i == 0) {
					inheritleft[c_i] = false;
				} else if (c_i == (curlinegroup.charcount_ - 1)) {
					inheritleft[c_i] = true;
				} else {
					float distleft =
							ocrresults_[curlinegroup.characters_[c_i]].middlexavg_
									- ocrresults_[curlinegroup.characters_[c_i
											- 1]].middlexavg_;
					float distright =
							ocrresults_[curlinegroup.characters_[c_i + 1]].middlexavg_
									- ocrresults_[curlinegroup.characters_[c_i]].middlexavg_;
					if (distleft < distright) {
						inheritleft[c_i] = true;
					} else {
						inheritleft[c_i] = false;
					}
				}
			}
		}

		for (unsigned int c_i = 0; c_i < (curlinegroup.charcount_ - 1); c_i++) {
			if (needcheck[c_i] && needcheck[c_i + 1]) {
				if ((!inheritleft[c_i]) && inheritleft[c_i + 1]) {

					if (c_i == 0) {
						inheritleft[c_i + 1] = false;
					} else if (c_i == (curlinegroup.charcount_ - 2)) {
						inheritleft[c_i] = true;
					} else {

						float distleft =
								ocrresults_[curlinegroup.characters_[c_i]].middlexavg_
										- ocrresults_[curlinegroup.characters_[c_i
												- 1]].middlexavg_;
						float distright =
								ocrresults_[curlinegroup.characters_[c_i + 2]].middlexavg_
										- ocrresults_[curlinegroup.characters_[c_i
												+ 1]].middlexavg_;
						if (distleft < distright) {
							inheritleft[c_i] = true;
						} else {
							inheritleft[c_i + 1] = false;
						}
					}
				}
			}
		}

		bool haschanged = true;
		unsigned int safetycounter = 0;
		while (haschanged && (safetycounter < 20)) {
			safetycounter++;
			haschanged = false;

			for (unsigned int c_i = 1; c_i < curlinegroup.charcount_; c_i++) {
				if (needcheck[c_i] && inheritleft[c_i]
						&& (!needcheck[c_i - 1])) {

					if (isnumc1[c_i - 1]) {

						if (!isnumc1[c_i]) {

							unsigned int swapchar =
									curlinegroup.characters_[c_i];
							curlinegroup.characters_[c_i] =
									curlinegroup.characters2_[c_i];
							curlinegroup.characters2_[c_i] = swapchar;
							char swaptext = curlinegroup.text_[c_i];
							curlinegroup.text_[c_i] = curlinegroup.text2_[c_i];
							curlinegroup.text2_[c_i] = swaptext;
							isnumc1[c_i] = true;
						}
					} else {

						if (isnumc1[c_i]) {

							unsigned int swapchar =
									curlinegroup.characters_[c_i];
							curlinegroup.characters_[c_i] =
									curlinegroup.characters2_[c_i];
							curlinegroup.characters2_[c_i] = swapchar;
							char swaptext = curlinegroup.text_[c_i];
							curlinegroup.text_[c_i] = curlinegroup.text2_[c_i];
							curlinegroup.text2_[c_i] = swaptext;
							isnumc1[c_i] = false;
						}
					}

					needcheck[c_i] = false;
					haschanged = true;
				}
			}

			for (unsigned int c_i = 0; c_i < (curlinegroup.charcount_ - 1);
					c_i++) {
				if (needcheck[c_i] && (!inheritleft[c_i])
						&& (!needcheck[c_i + 1])) {

					if (isnumc1[c_i + 1]) {

						if (!isnumc1[c_i]) {

							unsigned int swapchar =
									curlinegroup.characters_[c_i];
							curlinegroup.characters_[c_i] =
									curlinegroup.characters2_[c_i];
							curlinegroup.characters2_[c_i] = swapchar;
							char swaptext = curlinegroup.text_[c_i];
							curlinegroup.text_[c_i] = curlinegroup.text2_[c_i];
							curlinegroup.text2_[c_i] = swaptext;
							isnumc1[c_i] = true;
						}
					} else {

						if (isnumc1[c_i]) {

							unsigned int swapchar =
									curlinegroup.characters_[c_i];
							curlinegroup.characters_[c_i] =
									curlinegroup.characters2_[c_i];
							curlinegroup.characters2_[c_i] = swapchar;
							char swaptext = curlinegroup.text_[c_i];
							curlinegroup.text_[c_i] = curlinegroup.text2_[c_i];
							curlinegroup.text2_[c_i] = swaptext;
							isnumc1[c_i] = false;
						}
					}

					needcheck[c_i] = false;
					haschanged = true;
				}
			}
		}

	}
}

void CLPReader::FindCharLines2() {

	CNetImage *fullimage = mipmapper_->GetFullMipMap(0);

	for (int lsp_i = (int) linesearchposvect_->size() - 1; lsp_i >= 0;
			lsp_i--) {
		CLPReader::LineSearchPosS &curlsp = (*linesearchposvect_)[lsp_i];

		if (curlsp.searchlevel_ == 0) {

			curlsp.charlinecount_ = 0;
			continue;
		}

		unsigned int mincharsizesl;
		if (curlsp.searchlevel_ == 1) {
			mincharsizesl = 8;
		} else {
			mincharsizesl = 12;
		}
		unsigned int maxcharsizesl = 28;

		float ralsx = float(
				(curlsp.xleft_ + curlsp.xright_) << (curlsp.searchlevel_ - 1))
				/ 2.0f;
		float ralsy = float(
				(curlsp.yleft_ + curlsp.yright_) << (curlsp.searchlevel_ - 1))
				/ 2.0f;

		unsigned int samplingwidth = 40 << (curlsp.searchlevel_ - 1);
		if ((ralsx < (samplingwidth / 2 + 4))
				|| (ralsx > (fullimage->width_ - samplingwidth / 2 - 4))) {

			curlsp.charlinecount_ = 0;
			continue;
		}

		unsigned int charheightmax = maxcharsizesl << (curlsp.searchlevel_ - 1);

		float samplingytop = ralsy - (6 << (curlsp.searchlevel_ - 1));
		float samplingybottom = ralsy + charheightmax * 2
				+ (6 << (curlsp.searchlevel_ - 1));

		if (samplingytop < fclsamplingedgedist_ * 100.0f) {
			samplingytop = fclsamplingedgedist_;
		}

		if (samplingybottom
				> (fullimage->height_ - fclsamplingedgedist_ * 100.0f)) {
			samplingybottom = (fullimage->height_
					- fclsamplingedgedist_ * 100.0f);
		}

		unsigned int simageheight = (unsigned int) (float(
				samplingybottom - samplingytop) / samplingwidth
				* (6 + maxcharsizesl + 6));

		if (simageheight < (8 + mincharsizesl + 8)) {
			curlsp.charlinecount_ = 0;
			continue;
		}

		fclimage_->Create(40, simageheight);

		CPointf16_16 p0s;
		CPointf16_16 p1s;
		CPointf16_16 p0d;
		CPointf16_16 p1d;
		p0s.Set((f16_16) (ralsx * 65536.0f),
				(f16_16) (samplingytop * 65536.0f));
		p1s.Set((f16_16) (ralsx * 65536.0f),
				(f16_16) (samplingybottom * 65536.0f));
		p0d.Set(20 << 16, 0 << 16);
		p1d.Set(20 << 16, (simageheight - 1) << 16);

		imgtran_->Set(p0s, p1s, p0d, p1d);
		f16_16 mx;
		mx = (f16_16) (((float) curlsp.yright_ - curlsp.yleft_)
				/ ((float) curlsp.xright_ - curlsp.xleft_) * 65536.0f);
		imgtran_->Shear(mx, 0);
		imgtran_->TransformTrilinear(*fclimage_);

		if (lsvaluescount_ < simageheight) {
			lsvaluescount_ = simageheight;
			delete lstopvalues_;
			lstopvalues_ = new f16_16[lsvaluescount_];
			delete lsbottomvalues_;
			lsbottomvalues_ = new f16_16[lsvaluescount_];
		}

		fclimagestats_->Create(6, simageheight);
		const unsigned int SAMPLINGWIDTH = 40;
		for (unsigned int y_i = 0; y_i < simageheight; y_i++) {
			unsigned char *lines = fclimage_->GetPtr(0, y_i);
			unsigned char *linep = fclimagestats_->GetPtr(0, y_i);

			unsigned int pixelsum = 0;
			for (unsigned int x_i = 0; x_i < SAMPLINGWIDTH; x_i++) {
				pixelsum += lines[x_i];
			}
			unsigned char pixelavg = pixelsum / SAMPLINGWIDTH;
			linep[0] = pixelavg;

			unsigned int beloveavgcount = 0;
			for (unsigned int x_i = 0; x_i < SAMPLINGWIDTH; x_i++) {
				if (lines[x_i] < pixelavg) {
					beloveavgcount++;
				}
			}
			unsigned char beloveavgratio = (beloveavgcount * 255)
					/ SAMPLINGWIDTH;
			linep[1] = beloveavgratio;

			unsigned int vsuma = 0;
			unsigned int vsumb = 0;
			for (unsigned int x_i = 0; x_i < (SAMPLINGWIDTH - 2); x_i++) {
				unsigned char c1 = lines[x_i];
				unsigned char c2 = lines[x_i + 2];
				if (c1 > c2) {
					vsuma += c1;
					vsuma -= c2;
				} else {
					vsumb += c2;
					vsumb -= c1;
				}
			}
			linep[2] = vsuma / (SAMPLINGWIDTH - 2);
			linep[3] = vsumb / (SAMPLINGWIDTH - 2);

			if (y_i < 2) {
				linep[4] = 0;
				linep[5] = 0;
			} else {
				unsigned char* lines2 = fclimage_->GetPtr(0, y_i - 2);
				unsigned int hsuma = 0;
				unsigned int hsumb = 0;
				for (unsigned int x_i = 0; x_i < SAMPLINGWIDTH; x_i++) {
					unsigned char c1 = lines[x_i];
					unsigned char c2 = lines2[x_i];
					if (c1 > c2) {
						hsuma += c1;
						hsuma -= c2;
					} else {
						hsumb += c2;
						hsumb -= c1;
					}
				}
				linep[4] = hsuma / SAMPLINGWIDTH;
				linep[5] = hsumb / SAMPLINGWIDTH;
			}

		}

		bool foundtopline = false;
		for (unsigned int topy = 0; topy <= (simageheight - mincharsizesl - 16);
				topy++) {
			fclimagemap_->CreateMap(6, 16, fclimagestats_->scanline_,
					fclimagestats_->GetPtr(0, topy));

			lstopnet_valuebufferarray_->CopyImage(fclimagemap_->imgdata_,
					fclimagemap_->width_, fclimagemap_->height_,
					fclimagemap_->scanline_);

			f16_16 curvalue;

			lstopnetarray_[0]->EvaluateFromBuffer();
			curvalue = lstopnetarray_[0]->GetOutputValue();

			float outputfloat = (float) curvalue / 65536.0f;
			if (outputfloat < lstopnetthreshold_[0]) {
				curvalue = 0;
			} else {
				lstopnetarray_[1]->EvaluateFromBuffer();
				curvalue = lstopnetarray_[1]->GetOutputValue();

				outputfloat = (float) curvalue / 65536.0f;
				if (outputfloat < lstopnetthreshold_[1]) {
					curvalue = 0;
				} else {
					foundtopline = true;
				}
			}

			lstopvalues_[topy] = curvalue;
		}

		if (!foundtopline) {
			curlsp.charlinecount_ = 0;
			continue;
		}

		for (unsigned int bv_i = 0; bv_i < lsvaluescount_; bv_i++) {
			lsbottomvalues_[bv_i] = 0;
		}
		for (unsigned int topy = 0; topy <= (simageheight - mincharsizesl - 16);
				topy++) {
			if (lstopvalues_[topy] != 0) {
				for (unsigned int by = topy + mincharsizesl;
						(by < (topy + maxcharsizesl))
								&& (by < simageheight - 16); by++) {
					lsbottomvalues_[by] = 1;
				}
			}
		}

		bool foundbottomline = false;
		for (unsigned int bottomy = 0; bottomy <= (simageheight - 16);
				bottomy++) {

			if (lsbottomvalues_[bottomy] == 0) {
				continue;
			}

			fclimagemap_->CreateMap(6, 16, fclimagestats_->scanline_,
					fclimagestats_->GetPtr(0, bottomy));

			lsbottomnet_valuebufferarray_->CopyImage(fclimagemap_->imgdata_,
					fclimagemap_->width_, fclimagemap_->height_,
					fclimagemap_->scanline_);

			f16_16 curvalue;

			lsbottomnetarray_[0]->EvaluateFromBuffer();
			curvalue = lsbottomnetarray_[0]->GetOutputValue();

			float outputfloat = (float) curvalue / 65536.0f;
			if (outputfloat < lsbottomnetthreshold_[0]) {
				curvalue = 0;
			} else {
				lsbottomnetarray_[1]->EvaluateFromBuffer();
				curvalue = lsbottomnetarray_[1]->GetOutputValue();

				outputfloat = (float) curvalue / 65536.0f;
				if (outputfloat < lsbottomnetthreshold_[1]) {
					curvalue = 0;
				} else {
					foundbottomline = true;
				}
			}

			lsbottomvalues_[bottomy] = curvalue;
		}

		if (!foundbottomline) {
			curlsp.charlinecount_ = 0;
			continue;
		}

		f16_16 maxscore = 0;
		unsigned int mstop = 0;
		unsigned int msbottom = 0;
		for (unsigned int topy = 0; topy <= (simageheight - mincharsizesl - 16);
				topy++) {
			if (lstopvalues_[topy] != 0) {
				for (unsigned int by = topy + mincharsizesl;
						(by < (topy + maxcharsizesl))
								&& (by < simageheight - 16); by++) {
					if (lsbottomvalues_[by] != 0) {
						f16_16 score = lstopvalues_[topy] + lsbottomvalues_[by];
						if (maxscore < score) {
							maxscore = score;
							mstop = topy;
							msbottom = by;
						}
					}
				}
			}
		}

		if (maxscore == 0) {
			curlsp.charlinecount_ = 0;
			continue;
		}

		CPointf16_16 src;
		CPointf16_16 dst;
		dst.Set(20 << 16, (mstop + 8) << 16);
		imgtran_->TransformPoint(src, dst);
		float topcentery = (float) src.y / 65536.0f;
		dst.Set(20 << 16, (msbottom + 8) << 16);
		imgtran_->TransformPoint(src, dst);
		float bottomcentery = (float) src.y / 65536.0f;

		float acomp = float(
				((int) curlsp.yright_ - (int) curlsp.yleft_)
						<< (curlsp.searchlevel_ - 1)) / 2.0f;

		curlsp.toplineyleft_ = topcentery - acomp;
		curlsp.toplineyright_ = topcentery + acomp;
		curlsp.bottomlineyleft_ = bottomcentery - acomp;
		curlsp.bottomlineyright_ = bottomcentery + acomp;

		curlsp.linexleft_ = (float) curlsp.xleft_
				* (1 << (curlsp.searchlevel_ - 1));
		curlsp.linexright_ = (float) curlsp.xright_
				* (1 << (curlsp.searchlevel_ - 1));

		curlsp.charlinecount_ = 1;
	}

	for (int lsp_i = (int) linesearchposvect_->size() - 1; lsp_i >= 0;
			lsp_i--) {
		CLPReader::LineSearchPosS &curlsp = (*linesearchposvect_)[lsp_i];
		if (curlsp.charlinecount_ == 1) {
			float topadjust = (curlsp.bottomlineyleft_ - curlsp.toplineyleft_)
					* (lsptopadjust_ - 0.5f) / 2.0f;
			float bottomadjust =
					(curlsp.bottomlineyleft_ - curlsp.toplineyleft_)
							* (lspbottomadjust_ - 0.5f) / 2.0f;
			curlsp.toplineyleft_ += topadjust;
			curlsp.toplineyright_ += topadjust;
			curlsp.bottomlineyleft_ += bottomadjust;
			curlsp.bottomlineyright_ += bottomadjust;
		}
	}

	for (int lsp_i = (int) linesearchposvect_->size() - 1; lsp_i >= 0;
			lsp_i--) {
		CLPReader::LineSearchPosS &curlsp = (*linesearchposvect_)[lsp_i];
		if (curlsp.charlinecount_ == 1) {
			float charheight = curlsp.bottomlineyleft_ - curlsp.toplineyleft_;
			float newlspwidth = charheight * 2.0f;

			float centerx = (curlsp.linexright_ + curlsp.linexleft_) / 2.0f;

			float newlinexleft = centerx - newlspwidth / 2.0f;
			float newlinexright = newlinexleft + newlspwidth;

			float newtoplineyleft = LPRLinTran(newlinexleft, curlsp.linexleft_,
					curlsp.linexright_, curlsp.toplineyleft_,
					curlsp.toplineyright_);
			float newtoplineyright = LPRLinTran(newlinexright,
					curlsp.linexleft_, curlsp.linexright_, curlsp.toplineyleft_,
					curlsp.toplineyright_);

			curlsp.linexleft_ = newlinexleft;
			curlsp.linexright_ = newlinexright;
			curlsp.toplineyleft_ = newtoplineyleft;
			curlsp.toplineyright_ = newtoplineyright;
			curlsp.bottomlineyleft_ = curlsp.toplineyleft_ + charheight;
			curlsp.bottomlineyright_ = curlsp.toplineyright_ + charheight;
		}
	}
}
