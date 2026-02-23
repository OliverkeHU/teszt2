#ifndef _MIPMAPPER_H_INCLUDED_
#define _MIPMAPPER_H_INCLUDED_

#include "nettypes.h"
#include <vector>

class CNetImage;

class CMipMapper {
private:
	void FreeMipMaps();
	std::vector<CNetImage *> mipmaps_;
	CNetImage *srcimg_;
	unsigned int srcdx_;
	unsigned int srcdy_;
	unsigned int validmipmapcount_;
	void PrepareMipMap(unsigned int _mapindex);

public:
	CMipMapper();
	~CMipMapper();
	bool SetSrcImage(CNetImage *_srcimg);

	void GetMipMap(CNetImage **_mipmapimg, CPointf16_16 _shiftmatrix[2],
			CPointf16_16 *_refsrc, CPointf16_16 _min, CPointf16_16 _max,
			f16_16 _resvl, f16_16 *_veclen = nullptr);

	CNetImage * GetFullMipMap(int _level);

	void Invalidate();
};

#endif 
