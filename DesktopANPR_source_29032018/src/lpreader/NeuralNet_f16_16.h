#ifndef _NEURALNET_F16_16_H_INCLUDED_
#define _NEURALNET_F16_16_H_INCLUDED_

#include "nettypes.h"
#include <vector>

#define NET_SIGMRES 1000

#define NET_SIGMLIM 12

class CSharedNetValueBuffer;
struct CHAData;

enum EMinMinMaxMode {
	mmm10x12 = 1012,
	mmm20x12 = 2012,
	mmm20x24 = 2024,
	mmm16x10 = 1610,
	mmm16x18 = 1618,
	mmm32x20 = 3220
};

class CNeuralNet_f16_16 {
protected:
	unsigned int bufferoffset_;
	class CSharedNetValueBuffer* valuebuffer_;
	bool usingprivatebuffer_;

	f16_16 *stripe_data_;

	f16_16 sigm_gain_;
	float sigm_gain_float_;

	f16_16 *values_;

	f16_16 *sigm_table_;

	f16_16 GetSigm(float _x);
	void CreateSigmTable(f16_16 _gain = (1 << 16));

	bool using_distributed_output_;
	std::vector<f16_16> distributed_output_thresholds_;
	f16_16 distributed_output_result_;
	f16_16 distributed_output_fitness_;

	void CalcDistributedOutput();

public:
	CNeuralNet_f16_16();
	~CNeuralNet_f16_16();

	unsigned int evalneuroncount_;
	unsigned int inputneuroncount_;
	unsigned int outputneuroncount_;

	void LoadFromStripedHAData(CHAData *_data);

	void LoadDistributedOutputData(CHAData *_data);

	void EvaluateImage_NoAVG(uint8_t *_imagedata);
	void EvaluateImage_CalcAVG(uint8_t *_imagedata);
	void EvaluateImage_CalcMinMinMax(uint8_t *_imagedata, EMinMinMaxMode _mode);

	static void CalcMinMinMax(uint8_t *_imagedata, EMinMinMaxMode _mode,
			uint8_t *_min1, uint8_t *_min2, uint8_t *_max,
			unsigned int _scanlinelength = 0xffffffff);

	void EvaluateImage_AppendAVG(uint8_t *_imagedata, uint8_t _avg,
			uint8_t _avgl, uint8_t _avgh);

	inline void EvaluateFromBuffer() {

		f16_16 *curstripedata = stripe_data_ + 3;
		f16_16 *valuesarray = values_;
		f16_16 *hivalue = valuesarray + inputneuroncount_;
		for (unsigned int evalneuron_i = 0; evalneuron_i < evalneuroncount_;
				evalneuron_i++) {
			f16_16 sumvalue;

			sumvalue = *curstripedata++;

			int stripecount = *curstripedata++;

			for (int stripe_i = 0; stripe_i < stripecount; stripe_i++) {

				int linkcount = *curstripedata++;
				if (linkcount > 0) {

					f16_16 *valueptr = valuesarray + *curstripedata++;

					while (linkcount > 0) {

						f16_16 curwv = (*valueptr++) * (*curstripedata++);

						sumvalue += curwv;
						linkcount--;
					}

				} else {

					linkcount = -linkcount;

					while (linkcount > 0) {
						f16_16 curwv = valuesarray[*curstripedata++];
						curwv *= (*curstripedata++);
						sumvalue += curwv;
						linkcount--;
					}
				}
			}

			sumvalue = sumvalue >> 8;

			int st_i = (int32_t((sumvalue * NET_SIGMRES) >> 16))
					+ ( NET_SIGMRES * NET_SIGMLIM);
			if (st_i < 0) {
				st_i = 0;
			} else if (st_i > ( NET_SIGMRES * NET_SIGMLIM * 2 - 1)) {
				st_i = ( NET_SIGMRES * NET_SIGMLIM * 2 - 1);
			}

			*hivalue++ = sigm_table_[st_i];

		}

		if (using_distributed_output_) {
			CalcDistributedOutput();
		}
	}

	inline f16_16 GetOutputValue(bool _accumulated = true,
			unsigned int _outputindex = 0) {

		if (using_distributed_output_ && _accumulated) {
			return distributed_output_result_;
		} else {

			return values_[inputneuroncount_ + evalneuroncount_
					- outputneuroncount_ + _outputindex] << 8;
		}
	}

	void CopyDistributedOutput(f16_16 *_output, unsigned int _outputsize);
	void GetDistributedOutputFitness(f16_16 *_fitness);

	void SetValueBuffer(CSharedNetValueBuffer* _valuebuffer);
	void CheckValuesBuffer();
	unsigned int GetValueBufferOffset();
};

#endif 
