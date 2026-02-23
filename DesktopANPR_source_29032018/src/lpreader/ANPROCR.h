#ifndef _ANPROCR_H_INCLUDED_
#define _ANPROCR_H_INCLUDED_

#include "nettypes.h"
#include <vector>

class CNetImage;
class CNeuralNet_f16_16;
class CSharedNetValueBuffer;
class CImageTransform;
struct CHAData;

struct OCRNetS {
	char filtercode_[40];
	class CNeuralNet_f16_16 *network_[4];
	float threshold_[4];
	CSharedNetValueBuffer *buffer_;

	unsigned int runcount_[4];
	unsigned int passcount_[4];

	std::vector<float> stradj_;

	inline float OCRNLinTran(float _pos1, float _begin1, float _end1,
			float _begin2, float _end2) {

		return _begin2
				+ (_pos1 - _begin1) / (_end1 - _begin1) * (_end2 - _begin2);
	}

	OCRNetS() {
	}
	~OCRNetS() {
	}

	float AdjustStrength(float _value) {
		if (stradj_.empty()) {
			return _value;
		}
		unsigned int i1 = (unsigned int) (_value * 100.0f);
		if (i1 > 98) {
			return 1.0f;
		} else if (i1 < 1) {
			return 0.0f;
		}
		return OCRNLinTran(_value * 100.0f, (float) i1, (float) i1 + 1.0f,
				stradj_[i1 - 1], stradj_[i1]);
	}
};

class CANPROCR {
private:
	float netresult_[256];
	CNetImage *storedinputimg_;
	unsigned char storedinputimgextrachar_[9];
	CNetImage *usinginputimg_;
	unsigned char *usinginputimgextrachar_;

	bool LoadNetData(const char *_name, const char *_filtercode,
			CHAData *_lprdata);

	f16_16 trp1y_;
	f16_16 trp2y_;

	void CalculateInputImgExtra();

	void RunNets();

	bool LoadNet(CNeuralNet_f16_16 *_net, const char*_netfilename,
			CHAData *_initdata);

	std::vector<OCRNetS> *nets_;

public:
	CANPROCR();
	~CANPROCR();

	void Init(CHAData *_lprdata = nullptr);

	void Process(CNetImage *_img, unsigned char *_inputimgextrachar);

	bool GetCharCodes(unsigned char *_charcode1, float *_strength1,
			unsigned char *_charcode2, float *_strength2);
};

#endif 
