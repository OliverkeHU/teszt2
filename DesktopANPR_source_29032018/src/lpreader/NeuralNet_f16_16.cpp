#include "NeuralNet_f16_16.h"
#include "SharedNetValueBuffer.h"
#include "HAData.h"
#include <math.h>
#include <algorithm>

CNeuralNet_f16_16::CNeuralNet_f16_16() {
	values_ = nullptr;

	stripe_data_ = nullptr;
	using_distributed_output_ = false;
	distributed_output_fitness_ = 0x0000ffff;
	valuebuffer_ = nullptr;
	usingprivatebuffer_ = false;
	evalneuroncount_ = 0;
	inputneuroncount_ = 0;
	outputneuroncount_ = 0;
	sigm_table_ = nullptr;
}

CNeuralNet_f16_16::~CNeuralNet_f16_16() {
	if (usingprivatebuffer_) {
		delete valuebuffer_;
		valuebuffer_ = nullptr;
	}

	if (nullptr != stripe_data_) {
		delete[] stripe_data_;
		stripe_data_ = nullptr;
	}

	if (nullptr != sigm_table_) {
		delete[] sigm_table_;
		sigm_table_ = nullptr;
	}
}

void CNeuralNet_f16_16::SetValueBuffer(CSharedNetValueBuffer* _valuebuffer) {

	if (usingprivatebuffer_ && (nullptr != valuebuffer_)) {
		delete valuebuffer_;
	}
	valuebuffer_ = _valuebuffer;
	usingprivatebuffer_ = false;
}

void CNeuralNet_f16_16::LoadFromStripedHAData(CHAData *_data) {

	unsigned int datalen = 0;
	f16_16 *sdata = (f16_16*) _data->GetRawData("netdata", &datalen);
	if (nullptr != stripe_data_) {
		delete stripe_data_;
		stripe_data_ = nullptr;
	}
	if (0 == datalen) {

		return;
	}
	stripe_data_ = new f16_16[datalen / 4];
	std::copy_n(sdata, datalen / 4, stripe_data_);

	f16_16 sigm_gain = _data->GetInt("Sigma gain", 1 << 16);
	unsigned int stdatalen;
	f16_16 *sigmtabledata = (f16_16*) _data->GetRawData("Sigma table",
			&stdatalen);
	if (nullptr == sigmtabledata) {

		CreateSigmTable(sigm_gain);
	} else {

		if (nullptr == sigm_table_) {
			sigm_table_ = new f16_16[ NET_SIGMRES * NET_SIGMLIM * 2];
		}
		std::copy_n(sigmtabledata, stdatalen / 4, sigm_table_);

		sigm_gain_ = sigm_gain;
		sigm_gain_float_ = (1.0f * sigm_gain) / 0x10000;
	}

	evalneuroncount_ = stripe_data_[0];
	inputneuroncount_ = stripe_data_[1];
	outputneuroncount_ = stripe_data_[2];

	if (nullptr != valuebuffer_) {

		bufferoffset_ = valuebuffer_->SetAllocationSize(evalneuroncount_);
	}

	using_distributed_output_ = false;
}

void CNeuralNet_f16_16::CreateSigmTable(f16_16 _gain) {

	if (nullptr == sigm_table_) {
		sigm_table_ = new f16_16[ NET_SIGMRES * NET_SIGMLIM * 2];
	}

	sigm_gain_ = _gain;
	sigm_gain_float_ = (float) _gain / 65536.0f;
	for (int i = - NET_SIGMRES * NET_SIGMLIM; i < NET_SIGMRES * NET_SIGMLIM;
			i++) {
		sigm_table_[i + NET_SIGMRES * NET_SIGMLIM] = (GetSigm(
				float(i) / float(NET_SIGMRES))) >> 8;
	}
}

f16_16 CNeuralNet_f16_16::GetSigm(float _x) {

	float expparam = -sigm_gain_float_ * _x;
	if (expparam < -40) {
		expparam = -40;
	} else if (expparam > 40) {
		expparam = 40;
	}

	float res = 1.0f / (1.0f + std::exp(expparam));
	f16_16 resf = (f16_16) (0x10000 * res);
	return resf;
}

void CNeuralNet_f16_16::EvaluateImage_NoAVG(uint8_t *_imagedata) {

	f16_16 *inputvalues = values_;
	for (unsigned int input_i = 0; input_i < inputneuroncount_; input_i++) {
		*inputvalues = (uint32_t) (*_imagedata);
		_imagedata++;
		inputvalues++;
	}

	EvaluateFromBuffer();
}

void CNeuralNet_f16_16::EvaluateImage_AppendAVG(uint8_t *_imagedata,
		uint8_t _avg, uint8_t _avgl, uint8_t _avgh) {

	f16_16 *inputvalues = values_;
	for (unsigned int input_i = 0; input_i < inputneuroncount_ - 3; input_i++) {
		*inputvalues = (uint32_t) (*_imagedata);
		_imagedata++;
		inputvalues++;
	}

	inputvalues[0] = _avg;
	inputvalues[1] = _avgl;
	inputvalues[2] = _avgh;

	EvaluateFromBuffer();
}

void CNeuralNet_f16_16::EvaluateImage_CalcAVG(uint8_t *_imagedata) {

	f16_16 *inputvalues = values_;

	f16_16 valuesum = 0;
	for (unsigned int input_i = 0; input_i < inputneuroncount_ - 3; input_i++) {

		uint8_t curdata = *_imagedata;
		f16_16 curvalue = (uint32_t) curdata;
		*inputvalues = curvalue;
		valuesum += curvalue;
		_imagedata++;
		inputvalues++;
	}
	f16_16 avg = valuesum / (inputneuroncount_ - 3);

	f16_16 avgh = 0;
	f16_16 avgl = 0;
	int avghc = 0;
	int avglc = 0;
	inputvalues = values_;
	for (unsigned int input_i = 0; input_i < inputneuroncount_ - 3; input_i++) {
		f16_16 curvalue = *inputvalues;
		if (curvalue < avg) {
			avgl += curvalue;
			avglc++;
		} else if (curvalue > avg) {
			avgh += curvalue;
			avghc++;
		}
		inputvalues++;
	}
	if (0 != avglc) {
		avgl /= avglc;
	} else {
		avgl = avg;
	}
	if (0 != avghc) {
		avgh /= avghc;
	} else {
		avgh = avg;
	}

	inputvalues[0] = avg;
	inputvalues[1] = avgl;
	inputvalues[2] = avgh;

	EvaluateFromBuffer();
}

void CNeuralNet_f16_16::CheckValuesBuffer() {

	if (values_ == nullptr) {

		if (valuebuffer_ == nullptr) {

			valuebuffer_ = new CSharedNetValueBuffer;
			usingprivatebuffer_ = true;
			bufferoffset_ = valuebuffer_->SetAllocationSize(
					evalneuroncount_ + inputneuroncount_);
			values_ = valuebuffer_->GetBuffer(bufferoffset_);
		} else {

			values_ = valuebuffer_->GetBuffer(0);
		}
	}
}

void CNeuralNet_f16_16::CalcDistributedOutput() {

	f16_16* outputneurons = values_ + inputneuroncount_ + evalneuroncount_
			- outputneuroncount_;
	f16_16 targetweight = *outputneurons;
	int targetindex = 0;
	for (unsigned int n_i = 1; n_i < outputneuroncount_; n_i++) {
		if (targetweight < outputneurons[n_i]) {
			targetweight = outputneurons[n_i];
			targetindex = n_i;
		}
	}

	f16_16 leftv;
	f16_16 leftw;
	f16_16 rightv;
	f16_16 rightw;
	if (targetindex == 0) {
		leftv = 655;
		leftw = outputneurons[0];
		rightv = distributed_output_thresholds_[1];
		rightw = outputneurons[1];
	} else if (targetindex == (int) (outputneuroncount_ - 1)) {
		leftv =
				distributed_output_thresholds_[distributed_output_thresholds_.size()
						- 2];
		leftw = outputneurons[outputneuroncount_ - 2];
		rightv = 65535;
		rightw = outputneurons[outputneuroncount_ - 1];
	} else {
		if (outputneurons[targetindex - 1] > outputneurons[targetindex + 1]) {

			leftv = distributed_output_thresholds_[targetindex];
			leftw = outputneurons[targetindex - 1];
			rightv = (distributed_output_thresholds_[targetindex]
					+ distributed_output_thresholds_[targetindex + 1]) / 2;
			rightw = outputneurons[targetindex];
		} else {

			leftv = (distributed_output_thresholds_[targetindex]
					+ distributed_output_thresholds_[targetindex + 1]) / 2;
			leftw = outputneurons[targetindex];
			rightv = distributed_output_thresholds_[targetindex + 1];
			rightw = outputneurons[targetindex + 1];
		}
	}

	leftw = leftw >> 8;
	rightw = rightw >> 8;
	if ((leftw + rightw) == 0) {
		distributed_output_result_ = 0;
	} else {
		distributed_output_result_ = (leftv * leftw + rightv * rightw)
				/ (leftw + rightw);
	}

	f16_16 maxweight = outputneurons[0];
	int maxindex = 0;
	for (unsigned int n_i = 1; n_i < outputneuroncount_; n_i++) {
		if (maxweight < outputneurons[n_i]) {
			maxweight = outputneurons[n_i];
			maxindex = n_i;
		}
	}

	f16_16 maxweightnb;
	unsigned int skipidx1;
	unsigned int skipidx2;
	if (maxindex == 0) {
		skipidx1 = 0;
		skipidx2 = 1;
		maxweightnb = outputneurons[1];
	} else if (maxindex == (int) (outputneuroncount_ - 1)) {
		skipidx1 = outputneuroncount_ - 1;
		skipidx2 = outputneuroncount_ - 2;
		maxweightnb = outputneurons[outputneuroncount_ - 2];
	} else {
		if (outputneurons[maxindex - 1] > outputneurons[maxindex + 1]) {

			skipidx1 = maxindex - 1;
			skipidx2 = maxindex;
			maxweightnb = outputneurons[maxindex - 1];
		} else {

			skipidx1 = maxindex;
			skipidx2 = maxindex + 1;
			maxweightnb = outputneurons[maxindex + 1];
		}
	}

	f16_16 outputsum = 0;
	for (unsigned int n_i = 0; n_i < outputneuroncount_; n_i++) {
		if (n_i < skipidx1) {
			outputsum += outputneurons[n_i] * (skipidx1 - n_i + 1);
		} else if (n_i > skipidx2) {
			outputsum += outputneurons[n_i] * (n_i - skipidx2 + 1);
		}
	}

	f16_16 fittnessbase = maxweight + maxweightnb / 2;
	f16_16 fittnessdiv = outputsum + fittnessbase;
	if (fittnessdiv == 0) {
		distributed_output_fitness_ = 0;
	} else {
		distributed_output_fitness_ = (fittnessbase << 8) / (fittnessdiv >> 8);
	}

	distributed_output_fitness_ = (distributed_output_fitness_ >> 8)
			* ((65536 + fittnessbase) >> 10);
}

void CNeuralNet_f16_16::LoadDistributedOutputData(CHAData *_data) {
	distributed_output_thresholds_.clear();

	distributed_output_thresholds_.push_back(0);

	for (unsigned int do_i = 0; do_i < _data->GetCount(); do_i++) {
		int v = _data->GetInt(do_i);
		distributed_output_thresholds_.push_back(v);
	}

	distributed_output_thresholds_.push_back(65535);

	using_distributed_output_ = true;
}

void CNeuralNet_f16_16::CopyDistributedOutput(f16_16 *_output,
		unsigned int _outputsize) {

	unsigned int curindex = 0;
	if (using_distributed_output_) {
		f16_16* outputneurons = values_ + inputneuroncount_ + evalneuroncount_
				- outputneuroncount_;
		while ((curindex < outputneuroncount_) && (curindex < _outputsize)) {
			f16_16 curv = outputneurons[curindex];
			_output[curindex] = curv << 8;
			curindex++;
		}
	}

	while (curindex < _outputsize) {
		_output[curindex++] = -1;
	}
}

void CNeuralNet_f16_16::GetDistributedOutputFitness(f16_16 *_fitness) {

	*_fitness = distributed_output_fitness_;
}

void CNeuralNet_f16_16::EvaluateImage_CalcMinMinMax(uint8_t *_imagedata,
		EMinMinMaxMode _mode) {

	f16_16 *inputvalues = values_;
	uint8_t *pixdata = _imagedata;
	for (unsigned int input_i = 0; input_i < inputneuroncount_ - 3; input_i++) {
		*inputvalues = (uint32_t) (*pixdata);
		pixdata++;
		inputvalues++;
	}

	uint8_t min1;
	uint8_t min2;
	uint8_t max;

	CalcMinMinMax(_imagedata, _mode, &min1, &min2, &max);

	inputvalues[0] = min1;
	inputvalues[1] = min2;
	inputvalues[2] = max;

	EvaluateFromBuffer();
}

void CNeuralNet_f16_16::CalcMinMinMax(uint8_t *_imagedata, EMinMinMaxMode _mode,
		uint8_t *_min1, uint8_t *_min2, uint8_t *_max,
		unsigned int _scanlinelength) {

	unsigned int maxn_x0;
	unsigned int maxn_x1;
	unsigned int maxn_y0;
	unsigned int maxn_y1;

	unsigned int min1_x0;
	unsigned int min1_x1;
	unsigned int min2_x0;
	unsigned int min2_x1;

	unsigned int min_y0;
	unsigned int min_y1;

	switch (_mode) {
	case mmm10x12:

		maxn_x0 = 3;
		maxn_x1 = 6;
		maxn_y0 = 4;
		maxn_y1 = 6;

		min1_x0 = 2;
		min1_x1 = 3;
		min2_x0 = 6;
		min2_x1 = 7;

		min_y0 = 2;
		min_y1 = 4;

		if (_scanlinelength == 0xffffffff) {
			_scanlinelength = 10;
		}
		break;
	case mmm20x12:

		maxn_x0 = 6;
		maxn_x1 = 13;
		maxn_y0 = 5;
		maxn_y1 = 9;

		min1_x0 = 2;
		min1_x1 = 8;
		min2_x0 = 11;
		min2_x1 = 17;

		min_y0 = 2;
		min_y1 = 9;

		if (_scanlinelength == 0xffffffff) {
			_scanlinelength = 20;
		}
		break;
	case mmm20x24:

		maxn_x0 = 6;
		maxn_x1 = 13;
		maxn_y0 = 8;
		maxn_y1 = 13;

		min1_x0 = 4;
		min1_x1 = 7;
		min2_x0 = 12;
		min2_x1 = 15;

		min_y0 = 4;
		min_y1 = 7;

		if (_scanlinelength == 0xffffffff) {
			_scanlinelength = 20;
		}
		break;
	case mmm16x10:

		maxn_x0 = 6;
		maxn_x1 = 9;
		maxn_y0 = 5;
		maxn_y1 = 9;

		min1_x0 = 2;
		min1_x1 = 6;
		min2_x0 = 9;
		min2_x1 = 13;

		min_y0 = 2;
		min_y1 = 6;

		if (_scanlinelength == 0xffffffff) {
			_scanlinelength = 16;
		}
		break;
	case mmm16x18:

		maxn_x0 = 6;
		maxn_x1 = 9;
		maxn_y0 = 5;
		maxn_y1 = 9;

		min1_x0 = 3;
		min1_x1 = 6;
		min2_x0 = 9;
		min2_x1 = 12;

		min_y0 = 3;
		min_y1 = 5;

		if (_scanlinelength == 0xffffffff) {
			_scanlinelength = 16;
		}
		break;
	case mmm32x20:

		maxn_x0 = 11;
		maxn_x1 = 20;
		maxn_y0 = 10;
		maxn_y1 = 19;

		min1_x0 = 5;
		min1_x1 = 11;
		min2_x0 = 20;
		min2_x1 = 26;

		min_y0 = 2;
		min_y1 = 10;

		if (_scanlinelength == 0xffffffff) {
			_scanlinelength = 32;
		}
		break;
	default:

		*_min1 = 0;
		*_min2 = 0;
		*_max = 0;
		return;
		break;
	};

	uint8_t maxn = 0;
	for (unsigned int y = maxn_y0; y <= maxn_y1; y++) {
		uint8_t *imgsl = _imagedata + y * _scanlinelength;
		for (unsigned int x = maxn_x0; x <= maxn_x1; x++) {
			uint8_t pixdata = imgsl[x];
			if (maxn < pixdata) {
				maxn = pixdata;
			}
		}
	}

	uint8_t mine1 = 255;
	uint8_t mine2 = 255;
	for (unsigned int y = min_y0; y <= min_y1; y++) {
		uint8_t *imgsl = _imagedata + y * _scanlinelength;
		for (unsigned int x = min1_x0; x <= min1_x1; x++) {
			uint8_t pixdata = imgsl[x];
			if (mine1 > pixdata) {
				mine1 = pixdata;
			}
		}
		for (unsigned int x = min2_x0; x <= min2_x1; x++) {
			uint8_t pixdata = imgsl[x];
			if (mine2 > pixdata) {
				mine2 = pixdata;
			}
		}
	}

	*_min1 = mine1;
	*_min2 = mine2;
	*_max = maxn;
}

unsigned int CNeuralNet_f16_16::GetValueBufferOffset() {

	return bufferoffset_;
}
