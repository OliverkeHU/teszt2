#ifndef _NETTYPES_H_INCLUDED_
#define _NETTYPES_H_INCLUDED_

#include <cstdint>

typedef int32_t f16_16;
#define F16_16_DEFINED

struct CPointf16_16 {
	f16_16 x;
	f16_16 y;
	CPointf16_16(f16_16 _x, f16_16 _y) {
		x = _x;
		y = _y;
	}
	CPointf16_16() {
	}
	void Set(const struct CPointf16_16 &_param) {
		x = _param.x;
		y = _param.y;
	}
	void Set(f16_16 _x, f16_16 _y) {
		x = _x;
		y = _y;
	}
};

struct CPointfloat {
	float x;
	float y;
	CPointfloat& operator=(const CPointfloat &_pf) {
		x = _pf.x;
		y = _pf.y;
		return *this;
	}
	;
	void Set(float _x, float _y) {
		x = _x;
		y = _y;
	}
	;
};

#endif 
