#ifndef _LPTRACKER_H_INCLUDED_
#define _LPTRACKER_H_INCLUDED_

#include "nettypes.h"
#include "LPReader.h"

#define LPT_INVALID_TIMESTAMP ((long long)-1)

enum LPT_Event {
	LPTEVENT_START,
	LPTEVENT_END,
	LPTEVENT_VALID,
	LPTEVENT_INCOMING,
	LPTEVENT_OUTGOING,
	LPTEVENT_STOPPED,
	LPTEVENT_GUESSCHANGED
};

struct LPTEventS {
	LPT_Event type_;
	unsigned int trackID_;
	unsigned int plateID_;
	bool valid_;
	char guess_[ LPREADER_MAX_CHAR_PER_PLATE + 1];
	CPointfloat frame_[4];
	bool isincoming_;
	bool isoutgoing_;
	long long lastplatetimestamp_;
};

class CLPTracker {
private:

	static const long long deletetime_ = 6000;

	unsigned int nextplateID_;
	unsigned int nexttrackID_;
	long long lastimagetimestamp_;
	bool enableeventresend_;
	long long eventresenddelay_;
	unsigned int eventresendmaxcount_;

	void DeleteTracks();
	void SetBeforeDelete();
	void CreateEvents();

public:
	std::vector<LPTEventS> events_;

	struct CLPTrack {
	public:
		struct CLPTrackLP {
			CPointf16_16 center_;
			char fulltext_[ LPREADER_MAX_CHAR_PER_PLATE + 1];
			long long timestamp_;
			float charsize_;
			unsigned int plateID_;
			CPointfloat frame_[4];

			void Set(CLPReader::PlateS &_plate, long long _timestamp);
		};

		void Init(unsigned int _trackID);
		void Destroy();
		void AddPlate(CLPTracker::CLPTrack::CLPTrackLP &_plate);
		void UpdateStats();
		void CheckValidation(long long _curtimestamp);

		float GetMatchPossByText(CLPTrack::CLPTrackLP &_plate);

		long long lastactivetime_;
		std::vector<CLPTrackLP> *trackplates_;
		unsigned int trackID_;
		bool valid_;
		long long validatedtimestamp_;
		bool beforedelete_;
		long long starttimestamp_;
		long long guesschangetimestamp_;
		long long incomingtimestamp_;
		long long outgoingtimestamp_;
		long long stoppedtimestamp_;

		char guess_[2][ LPREADER_MAX_CHAR_PER_PLATE + 1];

		unsigned int guessmatchcount_;

		unsigned int totaltime_;

		bool valideventgenerated_;
		long long lastvalideventtimestamp_;
		unsigned int eventresendcount_;
	};

	CLPTracker();
	~CLPTracker();

	void TrackPlates(long long _timestamp,
			std::vector<CLPReader::PlateS> &_plates, unsigned int _imagewidth,
			unsigned int _imageheight);

	void ResetTracks();

	std::vector<CLPTrack> tracks_;

	void SetEvenResend(bool _enableeventresend, long long _eventresenddelay,
			unsigned int _eventresendmaxcount);
	void GetEvenResend(bool *_enableeventresend, long long *_eventresenddelay,
			unsigned int *_eventresendmaxcount) const;
};

#endif 
