#ifndef _LPREADER_H_INCLUDED_
#define _LPREADER_H_INCLUDED_
#define USEPROFILER

#include <vector>
#include <memory>

#include "ErrorVal.h"
#include "nettypes.h"

#define PLATE_FORMAT_CHECK_MAX_FORMAT_NAME_LENGTH 40
#define LPREADER_MAXSEARCHLEVEL 4
#define LPREADER_MAX_CHAR_PER_LINE 16
#define LPREADER_MAX_CHAR_PER_PLATE 16
#define LPREADER_HISTORY_LENGTH 16
#define LPREADER_LINE_SSEARCH_POSITION_HEIGHT 14
#define LPREADER_FIND_CHAR_SAMPLE_HEIGHT 20
#define LPREADER_FIND_CHAR_MIN_CHAR_HEIGHT 12
#define LPREADER_FIND_CHAR_MAX_CHAR_HEIGHT 38

inline float LPRLinTran(float _pos1, float _begin1, float _end1, float _begin2,
		float _end2) {

	return _begin2 + (_pos1 - _begin1) / (_end1 - _begin1) * (_end2 - _begin2);
}

class CBinRLE;
class CMipMapper;
class CMotionFilterRLE;
class CNetImage;
class CSharedNetValueBuffer;
class CNeuralNet_f16_16;
class CANPROCR;
class CImageTransform;
struct CHAData;

struct OCRCandidateS {
	float topy_;
	float bottomy_;
	float centerx_;

	OCRCandidateS() :
			topy_(0), bottomy_(0), centerx_(0) {
	}
	OCRCandidateS(float _topy, float _bottomy, float _centerx) :
			topy_(_topy), bottomy_(_bottomy), centerx_(_centerx) {
	}
};

struct OCRResultS {
	float middlex_;
	float topy_;
	float charheight_;
	float strength_;
	char charcode_;

	float middlexavg_;
	float topyavg_;
	float charheightavg_;

	bool conflictremoved_;

	bool IsOverlapping(struct OCRResultS &_ocrr) const {

		float bottomy = topy_ + charheight_;
		float bottomy2 = _ocrr.topy_ + _ocrr.charheight_;
		if ((topy_ > bottomy2) || (_ocrr.topy_ > bottomy)) {
			return false;
		}

		float distneeded = 0;
		if (charcode_ == 'I') {
			distneeded += charheight_ * 12 / 100;
		} else if (charcode_ == '1') {
			distneeded += charheight_ * 13 / 100;
		} else {
			distneeded += charheight_ * 15 / 100;
		}
		if (_ocrr.charcode_ == 'I') {
			distneeded += _ocrr.charheight_ * 12 / 100;
		} else if (_ocrr.charcode_ == '1') {
			distneeded += _ocrr.charheight_ * 13 / 100;
		} else {
			distneeded += _ocrr.charheight_ * 15 / 100;
		}

		float dist = middlex_ - _ocrr.middlex_;
		if (dist > 0) {
			if (dist > distneeded) {
				return false;
			}
		} else {
			if ((-dist) > distneeded) {
				return false;
			}
		}

		return true;
	}

	bool IsNeighbour(struct OCRResultS &_ocrr) const {

		float maxcharheight;
		const float maxcharheightdiffratio = 0.75f;

		if (charheightavg_ > _ocrr.charheightavg_) {
			if ((charheightavg_ * maxcharheightdiffratio)
					> _ocrr.charheightavg_) {
				return false;
			}
			maxcharheight = charheightavg_;
		} else {
			if ((_ocrr.charheightavg_ * maxcharheightdiffratio)
					> charheightavg_) {
				return false;
			}
			maxcharheight = _ocrr.charheightavg_;
		}

		float bottomy = topyavg_ + charheightavg_;
		float bottomy2 = _ocrr.topyavg_ + _ocrr.charheightavg_;
		float centery1 = (topyavg_ + bottomy) / 2;
		float centery2 = (_ocrr.topyavg_ + bottomy2) / 2;
		if (centery1 > centery2) {
			if ((centery1 - centery2) > (maxcharheight / 2)) {
				return false;
			}
		} else {
			if ((centery2 - centery1) > (maxcharheight / 2)) {
				return false;
			}
		}

		float distallowed = (maxcharheight) * 2.0f;
		float dist = middlexavg_ - _ocrr.middlexavg_;
		if (dist > 0) {
			if (dist > distallowed) {
				return false;
			}
		} else {
			if ((-dist) > distallowed) {
				return false;
			}
		}

		return true;
	}
};

struct LineGroupS {
	unsigned int posx_;
	unsigned int posy_;
	float charheight_;
	unsigned int charcount_;
	unsigned int characters_[ LPREADER_MAX_CHAR_PER_LINE];
	char text_[ LPREADER_MAX_CHAR_PER_LINE + 1];

	unsigned int characters2_[ LPREADER_MAX_CHAR_PER_LINE];
	char text2_[ LPREADER_MAX_CHAR_PER_LINE + 1];

	CPointfloat histogramframe_[4];
	CPointfloat frame_[4];
};

struct LNPFormatS {
	char text_[ LPREADER_MAX_CHAR_PER_PLATE];
	char formatname_[ PLATE_FORMAT_CHECK_MAX_FORMAT_NAME_LENGTH];
	float middlex_[ LPREADER_MAX_CHAR_PER_PLATE];
};

class CLPReader {
public:
	struct PlateS {
		unsigned int posx_;
		unsigned int posy_;
		float charheight_;
		char fulltext_[ LPREADER_MAX_CHAR_PER_PLATE + 1];
		char fulltext2_[ LPREADER_MAX_CHAR_PER_PLATE + 1];

		CPointfloat frame_[4];

		CPointfloat histogramframe_[4];
		unsigned char histogram_[ LPREADER_HISTORY_LENGTH];
		float histbrws_;
		signed char brhint_;

		std::vector<LNPFormatS> formats_;
	};

	struct LineSearchPosS {
		unsigned int searchlevel_;
		unsigned int x_;
		unsigned int topy_;

		unsigned char centeravg_[ LPREADER_LINE_SSEARCH_POSITION_HEIGHT * 2];
		unsigned char centeravga_[ LPREADER_LINE_SSEARCH_POSITION_HEIGHT * 2];
		unsigned char centervar_[ LPREADER_LINE_SSEARCH_POSITION_HEIGHT * 2];

		unsigned int xleft_;
		unsigned int yleft_;
		unsigned int xright_;
		unsigned int yright_;

		unsigned int charlinecount_;

		unsigned char fcsampleavg_[ LPREADER_FIND_CHAR_SAMPLE_HEIGHT];
		unsigned char fcsamplevar_[ LPREADER_FIND_CHAR_SAMPLE_HEIGHT];
		unsigned char fcsamplehcon_[ LPREADER_FIND_CHAR_SAMPLE_HEIGHT];

		unsigned char fcbottomavg_[ LPREADER_FIND_CHAR_MAX_CHAR_HEIGHT
				- LPREADER_FIND_CHAR_MIN_CHAR_HEIGHT + 4];
		unsigned char fcbottomvar_[ LPREADER_FIND_CHAR_MAX_CHAR_HEIGHT
				- LPREADER_FIND_CHAR_MIN_CHAR_HEIGHT + 4];
		unsigned char fcbottomhcon_[ LPREADER_FIND_CHAR_MAX_CHAR_HEIGHT
				- LPREADER_FIND_CHAR_MIN_CHAR_HEIGHT + 4];

		float linexleft_;
		float linexright_;
		float toplineyleft_;
		float toplineyright_;
		float bottomlineyleft_;
		float bottomlineyright_;

		float exttoplinexright_;
		float exttoplinexleft_;
	};

protected:

	unsigned int slmin_;
	unsigned int slmax_;
	unsigned int charsizemin_;
	unsigned int charsizemax_;

	CMipMapper *mipmapper_;

	unsigned int lastimagewidth_;
	unsigned int lastimageheight_;

	class CBinRLE *detareaRLE_[ LPREADER_MAXSEARCHLEVEL + 1];
	bool detareachanged_;
	std::unique_ptr<const std::vector<int16_t> > detectionareapoints_;

	class CMotionFilterRLE *motionfilter_[ LPREADER_MAXSEARCHLEVEL + 1];

	unsigned char motionfilterthreshold_;

	void UpdateDetAreaRLE();
	void RunMotionFilters();

	void AddLineGroupsToMotionFilter();

	void RunAVGABRecount();

	void RunTopFilterNet();

	void RunOCROnLSP();

	unsigned int RunOCROnLine(float _toplinexleft, float _toplineyleft,
			float _toplinexright, float _toplineyright, float _charheight,
			unsigned int _checklsp_i);

	bool RunMiddleNetAtPos();

	bool CheckIfFitsLSP(float _x, float _topy, float _charheight,
			unsigned int _checklsp_i);

	void CreateLineGroups();
	void CreatePlates();

	void CheckLetterNumberCollisions();

	void RunHistogramCalc();

	CNetImage *avgA_[ LPREADER_MAXSEARCHLEVEL + 1];
	CNetImage *avgB_[ LPREADER_MAXSEARCHLEVEL + 1];

	CNetImage *fclimage_;
	CNetImage *fclimagestats_;
	CNetImage *fclimagemap_;

	CSharedNetValueBuffer *cm3topfilternet_valuebufferarray_;
	CNeuralNet_f16_16 *cm3topfilternetarray_[5];
	float topfilterthreshold_[5];
	CBinRLE *chartopfilterRLE_[ LPREADER_MAXSEARCHLEVEL + 1];

	CSharedNetValueBuffer *msnet_valuebufferarray_;
	CNeuralNet_f16_16 *msnetarray_[4];
	float msnetthreshold_[4];

	CSharedNetValueBuffer *lstopnet_valuebufferarray_;
	CNeuralNet_f16_16 *lstopnetarray_[2];
	float lstopnetthreshold_[2];
	CSharedNetValueBuffer *lsbottomnet_valuebufferarray_;
	CNeuralNet_f16_16 *lsbottomnetarray_[2];
	float lsbottomnetthreshold_[2];

	f16_16 *lstopvalues_;
	f16_16 *lsbottomvalues_;
	unsigned int lsvaluescount_;

	std::vector<OCRCandidateS> *ocrcandidates_;

	unsigned char mscontthr1_;
	unsigned char mscontthr2_;

	float secondarystrengththr_;

	CNetImage *lsplineimg_;

	CNetImage *msimage_;
	float msimageextra_[9];
	unsigned char msimageextrachar_[9];

	CNetImage *cm3image_;
	float cm3imageextra_[9];
	unsigned char cm3imageextrachar_[9];

	void Createcm3Image(unsigned int _searchlevel, int _x, int _y);
	void Createcm3Image12x4AVG3AtBlockCenter(unsigned int _searchlevel,
			int _blockx, int _blocky);

	CImageTransform *imgtran_;

	bool LoadNet(CNeuralNet_f16_16 *_net, const char*_netfilename,
			CHAData *_initdata);

	void InitLineSearchPosVect();
	void RunAngleLineSearch();
	void FindCharLines();
	void FindCharLines2();
	unsigned int GetMatchScore(LineSearchPosS &_lsp, unsigned char *_stripavg,
			unsigned char *_stripvar);

	float lsptopadjust_;
	float lspbottomadjust_;
	float fclsamplingedgedist_;

	CANPROCR *ocr_;

	bool InitMiddleSearchNets(CHAData *_lprdata);
	bool InitTopFilterNets(CHAData *_lprdata);
	bool InitLineSearchNets(CHAData *_lprdata);

public:
	CLPReader();
	~CLPReader();

	bool Init(CHAData *_lprdata = nullptr);

	void SetMipMapper(CMipMapper *_mipmapper);
	ERRORVAL ProcessImage();

	void SetDetectionArea(const std::vector<int16_t>* const _mask);

	void SetSearchLevels(unsigned int _slmin, unsigned int _slmax);
	void GetSearchLevels(unsigned int *_slmin, unsigned int *_slmax);

	void SetCharSizes(unsigned int _charsizemin, unsigned int _charsizemax);
	void GetCharSizes(unsigned int *_charsizemin,
			unsigned int *_charsizemax) const;

	class CMotionFilterRLE * GetMotionFilter(unsigned int _searchlevel) {
		return motionfilter_[_searchlevel];
	}
	CBinRLE *GetCharTopFilterRLE(unsigned int _sl) {
		return chartopfilterRLE_[_sl];
	}
	std::vector<OCRCandidateS> *GetOCRCandidates() {
		return ocrcandidates_;
	}
	bool SaveFormatList(const char *_filename);

	std::vector<OCRResultS> ocrresults_;
	std::vector<LineGroupS> linegroups_;
	std::vector<PlateS> plates_;

	std::vector<LineSearchPosS> *linesearchposvect_;

	unsigned int msrunposcount_;
	unsigned int msconthrcount1_;
	unsigned int msconthrcount2_;
	unsigned int msnetruncount_[5];
	unsigned int msnetpasscount_[5];

	unsigned int secondaryOCRcount_;
	unsigned int secondaryOCRfilteredcount_;

};

#endif 
