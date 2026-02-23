#include "ANPROCR.h"
#include "NeuralNet_f16_16.h"
#include "HAData.h"
#include "SharedNetValueBuffer.h"
#include "NetImage.h"
#include "ImageTransform.h"

#include <string.h>

CANPROCR::CANPROCR() {
}

void CANPROCR::Init(CHAData *_lprdata) {

	storedinputimg_ = new CNetImage;
	storedinputimg_->Create(18, 20);

	trp1y_ = 3 << 16;
	trp2y_ = 17 << 16;

	nets_ = new std::vector<OCRNetS>;

	LoadNetData("L0_B0", "A4", _lprdata);
	LoadNetData("L0_B1", "KXVY", _lprdata);
	LoadNetData("L0_B2", "I1T", _lprdata);
	LoadNetData("L0_B3", "Z27LEF", _lprdata);
	LoadNetData("L0_B4", "G6B8S5PRDOC93JU0Q", _lprdata);
	LoadNetData("L0_B5", "MWHN", _lprdata);
	LoadNetData("L1_B0", "KX", _lprdata);
	LoadNetData("L1_B1", "VY", _lprdata);
	LoadNetData("L1_B2", "I1", _lprdata);
	LoadNetData("L1_B3", "Z27", _lprdata);
	LoadNetData("L1_B4", "LEF", _lprdata);
	LoadNetData("L1_B5", "G6B8S5PR", _lprdata);
	LoadNetData("L1_B6", "DOC0Q", _lprdata);
	LoadNetData("L1_B7", "93JU", _lprdata);
	LoadNetData("L1_B8", "MW", _lprdata);
	LoadNetData("L1_B9", "HN", _lprdata);
	LoadNetData("L2_B0", "Z2", _lprdata);
	LoadNetData("L2_B1", "EF", _lprdata);
	LoadNetData("L2_B2", "G6B8", _lprdata);
	LoadNetData("L2_B3", "S5", _lprdata);
	LoadNetData("L2_B4", "PR", _lprdata);
	LoadNetData("L2_B5", "DO0Q", _lprdata);
	LoadNetData("L2_B6", "93", _lprdata);
	LoadNetData("L2_B7", "JU", _lprdata);
	LoadNetData("L3_B0", "G6", _lprdata);
	LoadNetData("L3_B1", "B8", _lprdata);
	LoadNetData("L3_B2", "O0Q", _lprdata);
	char netname[3];
	char filtercode[2];
	netname[0] = 'C';
	netname[2] = 0;
	filtercode[1] = 0;
	for (unsigned char ccode = 'A'; ccode <= 'Z'; ccode++) {
		netname[1] = ccode;
		filtercode[0] = ccode;
		LoadNetData(netname, filtercode, _lprdata);
	}
	for (unsigned char ccode = '0'; ccode <= '9'; ccode++) {
		netname[1] = ccode;
		filtercode[0] = ccode;
		LoadNetData(netname, filtercode, _lprdata);
	}

	CHAData *OCRthresholddata = _lprdata->GetHAData("OCRthresholds");
	for (unsigned int net_i = 0; net_i < nets_->size(); net_i++) {
		OCRNetS &curnet = (*nets_)[net_i];

		if (nullptr == OCRthresholddata) {
			curnet.threshold_[0] = 0.5f;
			curnet.threshold_[1] = 0.5f;
			curnet.threshold_[2] = 0.5f;
			curnet.threshold_[3] = 0.5f;
		} else {
			char thrname[20];
			sprintf(thrname, "%ia", net_i);
			float thra = OCRthresholddata->GetFloat(thrname, 0.5f);
			sprintf(thrname, "%ib", net_i);
			float thrb = OCRthresholddata->GetFloat(thrname, 0.5f);
			sprintf(thrname, "%ic", net_i);
			float thrc = OCRthresholddata->GetFloat(thrname, 0.5f);
			sprintf(thrname, "%id", net_i);
			float thrd = OCRthresholddata->GetFloat(thrname, 0.5f);

			curnet.threshold_[0] = thra;
			curnet.threshold_[1] = thrb;
			curnet.threshold_[2] = thrc;
			curnet.threshold_[3] = thrd;
		}
	}

}

CANPROCR::~CANPROCR() {
	delete storedinputimg_;
	for (unsigned int net_i = 0; net_i < nets_->size(); net_i++) {
		OCRNetS &curnet = (*nets_)[net_i];
		delete curnet.network_[0];
		delete curnet.network_[1];
		delete curnet.network_[2];
		delete curnet.network_[3];
		delete curnet.buffer_;
	}
	delete nets_;
}

bool CANPROCR::LoadNetData(const char *_name, const char *_filtercode,
		CHAData *_lprdata) {
	char netfilenamea[25];
	char netfilenameb[25];
	char netfilenamec[25];
	char netfilenamed[25];
	char stradjfilenamec[25];
	sprintf(netfilenamea, "OCR%sa.net", _name);
	sprintf(netfilenameb, "OCR%sb.net", _name);
	sprintf(netfilenamec, "OCR%sc.net", _name);
	sprintf(netfilenamed, "OCR%sd.net", _name);
	sprintf(stradjfilenamec, "OCR%scstradj", _name);

	nets_->emplace_back();
	OCRNetS &newOCRNEt = nets_->back();
	strcpy(newOCRNEt.filtercode_, _filtercode);
	newOCRNEt.buffer_ = new CSharedNetValueBuffer;
	newOCRNEt.buffer_->SetAllocationSize(18 * 20 + 9);

	for (unsigned int ns_i = 0; ns_i < 4; ns_i++) {
		newOCRNEt.runcount_[ns_i] = 0;
		newOCRNEt.passcount_[ns_i] = 0;
	}

	newOCRNEt.network_[0] = new CNeuralNet_f16_16;
	newOCRNEt.network_[0]->SetValueBuffer(newOCRNEt.buffer_);
	if (!LoadNet(newOCRNEt.network_[0], netfilenamea, _lprdata)) {
		delete newOCRNEt.buffer_;
		nets_->pop_back();
		return false;
	}

	newOCRNEt.network_[1] = new CNeuralNet_f16_16;
	newOCRNEt.network_[1]->SetValueBuffer(newOCRNEt.buffer_);
	if (!LoadNet(newOCRNEt.network_[1], netfilenameb, _lprdata)) {
		delete newOCRNEt.network_[0];
		delete newOCRNEt.buffer_;
		nets_->pop_back();
		return false;
	}

	newOCRNEt.network_[2] = new CNeuralNet_f16_16;
	newOCRNEt.network_[2]->SetValueBuffer(newOCRNEt.buffer_);
	if (!LoadNet(newOCRNEt.network_[2], netfilenamec, _lprdata)) {
		delete newOCRNEt.network_[0];
		delete newOCRNEt.network_[1];
		delete newOCRNEt.buffer_;
		nets_->pop_back();
		return false;
	}

	newOCRNEt.network_[3] = new CNeuralNet_f16_16;
	newOCRNEt.network_[3]->SetValueBuffer(newOCRNEt.buffer_);
	if (!LoadNet(newOCRNEt.network_[3], netfilenamed, _lprdata)) {
		delete newOCRNEt.network_[3];
		newOCRNEt.network_[3] = nullptr;
	}

	newOCRNEt.network_[0]->CheckValuesBuffer();
	newOCRNEt.network_[1]->CheckValuesBuffer();
	newOCRNEt.network_[2]->CheckValuesBuffer();
	if (newOCRNEt.network_[3] != nullptr) {
		newOCRNEt.network_[3]->CheckValuesBuffer();
	}

	CHAData *netdataroot = _lprdata->GetHAData("nets");
	unsigned int datalen;
	float *stradjdata = (float*) netdataroot->GetRawData(stradjfilenamec,
			&datalen);
	if (stradjdata != nullptr) {
		newOCRNEt.stradj_.resize(100);
		for (unsigned int f_i = 0; f_i < 100; f_i++) {
			float f = stradjdata[f_i];
			newOCRNEt.stradj_[f_i] = f;
		}
	}

	return true;
}

bool CANPROCR::LoadNet(CNeuralNet_f16_16 *_net, const char*_netfilename,
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

void CANPROCR::CalculateInputImgExtra() {

	float inputimgextra[9];
	unsigned int sum;
	unsigned int sumabove;
	unsigned int sumbelove;
	unsigned int countabove;
	unsigned int countbelove;
	unsigned char avgval;

	for (unsigned int blokk_i = 0; blokk_i < 3; blokk_i++) {

		sum = 0;
		for (unsigned int y_i = 6; y_i <= 9; y_i++) {
			unsigned char* lines = storedinputimg_->GetPtr(0, y_i);
			for (unsigned int x_i = 3 + blokk_i * 4; x_i <= (6 + blokk_i * 4);
					x_i++) {
				unsigned char c = lines[x_i];
				sum += c;
			}
		}
		inputimgextra[0 + blokk_i * 3] = ((float) sum) / 16.0f;

		sumabove = 0;
		sumbelove = 0;
		countabove = 0;
		countbelove = 0;
		avgval = (unsigned char) inputimgextra[0 + blokk_i * 3];

		for (unsigned int y_i = 6; y_i <= 9; y_i++) {
			unsigned char* lines = storedinputimg_->GetPtr(0, y_i);
			for (unsigned int x_i = 3 + blokk_i * 4; x_i <= (6 + blokk_i * 4);
					x_i++) {
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
			inputimgextra[1 + blokk_i * 3] = inputimgextra[0 + blokk_i * 3];
		} else {
			inputimgextra[1 + blokk_i * 3] = (float) sumabove
					/ (float) countabove;
		}
		if (countbelove == 0) {
			inputimgextra[2 + blokk_i * 3] = inputimgextra[0 + blokk_i * 3];
		} else {
			inputimgextra[2 + blokk_i * 3] = (float) sumbelove
					/ (float) countbelove;
		}
	}

	for (unsigned int s_i = 0; s_i < 9; s_i++) {
		storedinputimgextrachar_[s_i] = (unsigned char) inputimgextra[s_i];
	}
}

void CANPROCR::Process(CNetImage *_img, unsigned char *_inputimgextrachar) {

	usinginputimg_ = _img;
	usinginputimgextrachar_ = _inputimgextrachar;

	RunNets();
}

void CANPROCR::RunNets() {

	for (unsigned int nr_i = 0; nr_i < 256; nr_i++) {
		netresult_[nr_i] = 0;
	}

	for (unsigned int net_i = 0; net_i < nets_->size(); net_i++) {

		OCRNetS &curnet = (*nets_)[net_i];
		bool needtorun = false;
		for (unsigned int fc_i = 0; fc_i < strlen(curnet.filtercode_); fc_i++) {
			unsigned char curfc = curnet.filtercode_[fc_i];
			if (netresult_[curfc] >= 0) {
				needtorun = true;
				break;
			}
		}

		if (needtorun) {

			f16_16 output16_16;
			float outputfloat;

			curnet.buffer_->CopyImage(usinginputimg_->imgdata_,
					usinginputimg_->width_, usinginputimg_->height_,
					usinginputimg_->scanline_);
			curnet.buffer_->CopyValues(18 * 20, usinginputimgextrachar_, 9);

			curnet.runcount_[0] += 1;
			curnet.network_[0]->EvaluateFromBuffer();
			output16_16 = curnet.network_[0]->GetOutputValue();
			outputfloat = (float) output16_16 / 65536.0f;
			if (outputfloat < curnet.threshold_[0]) {
				outputfloat = -1;
			} else {
				curnet.passcount_[0] += 1;

				curnet.runcount_[1] += 1;
				curnet.network_[1]->EvaluateFromBuffer();
				output16_16 = curnet.network_[1]->GetOutputValue();
				outputfloat = (float) output16_16 / 65536.0f;
				if (outputfloat < curnet.threshold_[1]) {
					outputfloat = -1;
				} else {
					curnet.passcount_[1] += 1;

					curnet.runcount_[2] += 1;
					curnet.network_[2]->EvaluateFromBuffer();
					output16_16 = curnet.network_[2]->GetOutputValue();
					outputfloat = (float) output16_16 / 65536.0f;
					if (outputfloat < curnet.threshold_[2]) {
						outputfloat = -1;
					} else {
						curnet.passcount_[2] += 1;

						outputfloat = curnet.AdjustStrength(outputfloat);

						if (nullptr != curnet.network_[3]) {
							curnet.runcount_[3] += 1;
							curnet.network_[3]->EvaluateFromBuffer();
							output16_16 = curnet.network_[3]->GetOutputValue();

							float outputfloat3 = (float) output16_16 / 65536.0f;
							if (outputfloat3 < curnet.threshold_[3]) {
								outputfloat = -1;
							} else {
								curnet.passcount_[3] += 1;
							}
						}
					}
				}
			}

			for (unsigned int fc_i = 0; fc_i < strlen(curnet.filtercode_);
					fc_i++) {
				unsigned char curfc = curnet.filtercode_[fc_i];
				netresult_[curfc] = outputfloat;
			}
		}
	}
}

bool CANPROCR::GetCharCodes(unsigned char *_charcode1, float *_strength1,
		unsigned char *_charcode2, float *_strength2) {

	*_charcode1 = 0;
	*_strength1 = 0.0f;
	*_charcode2 = 0;
	*_strength2 = 0.0f;

	for (unsigned char ccode = '0'; ccode <= 'Z'; ccode++) {
		if (ccode == ':') {
			ccode = 'A';
		}

		float curstr = netresult_[ccode];
		if (curstr > *_strength2) {
			*_strength2 = curstr;
			*_charcode2 = ccode;
			if (*_strength2 > *_strength1) {
				float ss = *_strength2;
				*_strength2 = *_strength1;
				*_strength1 = ss;
				unsigned char sc = *_charcode2;
				*_charcode2 = *_charcode1;
				*_charcode1 = sc;
			}
		}
	}

	if (*_charcode1 == 0) {
		return false;
	} else {
		return true;
	}
}
