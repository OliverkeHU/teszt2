#ifndef _IMAGETRANSFORM_H_INCLUDED_
#define _IMAGETRANSFORM_H_INCLUDED_

#include "nettypes.h"

class CMipMapper;
class CNetImage;

enum ETrilinearTransformMode {
	ttmNormal, ttmAverages, ttmOSMM, ttmFilterMinMinMax
};

class CImageTransform {
private:
	CPointf16_16 shiftmatrix_[2];

	CNetImage *srcimg_;
	CNetImage *tlimg1_;
	CNetImage *tlimg2_;
	CPointf16_16 refsrc_;
	CPointf16_16 refdst_;
	CMipMapper *mipmapper_;
	bool ownmipmapper_;

	ETrilinearTransformMode tltransmode_;
	float tltransavg_;
	float tltransavgh_;
	float tltransavgl_;

	void ApplyMatrix(CPointf16_16 _matrix[2]);

public:

	CImageTransform();
	~CImageTransform();

	void SetMipMapper(CMipMapper *_mipmapper);

	void Reset();
	void Scale(f16_16 _factor);
	void Shear(f16_16 _mx, f16_16 _my);

	void Set(CPointf16_16 &_p0s, CPointf16_16 &_p1s, CPointf16_16 &_p0d,
			CPointf16_16 &_p1d);
	void Set(CNetImage &_destimg, CPointf16_16 &_p0s, CPointf16_16 &_p1s,
			CPointf16_16 &_p2s);

	void TransformPoint(CPointf16_16 &_on_src,
			const CPointf16_16 &_on_dst) const;

	void TransformNearestN(CNetImage &_destimg);
	void TransformBilinear(CNetImage &_destimg, int32_t _resvl = 0x00020000,
			int32_t *_veclen = nullptr);

	void TransformTrilinear(CNetImage &_destimg);
};

#endif 
