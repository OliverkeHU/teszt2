#ifndef _MOTIONFILTERRLE_H_INCLUDED_
#define _MOTIONFILTERRLE_H_INCLUDED_

class CMipMapper;
class CNetImage;
class CBinRLE;

class CMotionFilterRLE {
private:
	class CNetImage *baseimage_;
	class CBinRLE *motionarea_;

public:
	CMotionFilterRLE();
	~CMotionFilterRLE();

	void Update(class CMipMapper *_mipmapper, unsigned int _level,
			unsigned char _threshold, class CBinRLE *_area);
	CBinRLE *GetMotionArea() {
		return motionarea_;
	}
};

#endif 
