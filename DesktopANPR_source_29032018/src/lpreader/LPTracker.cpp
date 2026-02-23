#include "LPTracker.h"
#include <cmath>
#include <cstring>

CLPTracker::CLPTracker() :
		nextplateID_(0), nexttrackID_(0), lastimagetimestamp_(
		LPT_INVALID_TIMESTAMP), enableeventresend_(false), eventresenddelay_(0), eventresendmaxcount_(
				0) {
}

CLPTracker::~CLPTracker() {
	ResetTracks();
}

struct STrackMatch {
	int plateindex_;
	int trackindex_;
	float poss_;
};

void CLPTracker::TrackPlates(long long _timestamp,
		std::vector<CLPReader::PlateS> &_plates, unsigned int, unsigned int) {
	DeleteTracks();

	std::vector<STrackMatch> matches;

	for (int plate_i = 0; plate_i < (int) _plates.size(); plate_i++) {
		CLPReader::PlateS &curplate = _plates[plate_i];
		CLPTrack::CLPTrackLP newtrackplate;
		newtrackplate.Set(curplate, _timestamp);

		bool hasmatch = false;
		for (int track_i = 0; track_i < (int) tracks_.size(); track_i++) {
			CLPTrack &curtrack = tracks_[track_i];

			STrackMatch newmatch;
			newmatch.poss_ = curtrack.GetMatchPossByText(newtrackplate);
			if (newmatch.poss_ > 0.0f) {
				newmatch.plateindex_ = plate_i;
				newmatch.trackindex_ = track_i;
				matches.push_back(newmatch);
				hasmatch = true;
			}
		}

		if (!hasmatch) {

			STrackMatch newmatch;
			newmatch.plateindex_ = plate_i;
			newmatch.trackindex_ = -1;
			newmatch.poss_ = 0.0f;
			matches.push_back(newmatch);
		}
	}

	float maxp;
	int maxp_match_i = 0;

	do {
		maxp = 0.0f;
		for (int match_i = 0; match_i < (int) matches.size(); match_i++) {
			STrackMatch &curmatch = matches[match_i];
			if (curmatch.poss_ > maxp) {
				maxp = curmatch.poss_;
				maxp_match_i = match_i;
			}
		}
		if (maxp > 0.0f) {

			int plate_i = matches[maxp_match_i].plateindex_;
			int track_i = matches[maxp_match_i].trackindex_;

			CLPTrack::CLPTrackLP newtrackplate;
			newtrackplate.Set(_plates[plate_i], _timestamp);
			newtrackplate.plateID_ = nextplateID_++;
			tracks_[track_i].AddPlate(newtrackplate);

			for (int match_i = (int) matches.size() - 1; match_i >= 0;
					match_i--) {
				STrackMatch &curmatch = matches[match_i];
				if ((curmatch.plateindex_ == plate_i)
						|| (curmatch.trackindex_ == track_i)) {
					matches.erase(matches.begin() + match_i);
				}
			}
		}
	} while (maxp > 0.0f);

	for (unsigned int track_i = 0; track_i < tracks_.size(); track_i++) {
		tracks_[track_i].CheckValidation(_timestamp);
	}

	for (int match_i = 0; match_i < (int) matches.size(); match_i++) {
		int plate_i = matches[match_i].plateindex_;
		CLPTrack::CLPTrackLP newtrackplate;
		newtrackplate.Set(_plates[plate_i], _timestamp);
		newtrackplate.plateID_ = nextplateID_++;

		CLPTrack newtrack;
		tracks_.push_back(newtrack);
		CLPTrack &curtrack = tracks_[tracks_.size() - 1];
		curtrack.Init(nexttrackID_++);
		curtrack.AddPlate(newtrackplate);
	}

	lastimagetimestamp_ = _timestamp;

	SetBeforeDelete();

	CreateEvents();
}

void CLPTracker::ResetTracks() {

	for (int track_i = 0; track_i < (int) tracks_.size(); track_i++) {
		tracks_[track_i].Destroy();
	}
	tracks_.clear();
}

void CLPTracker::DeleteTracks() {

	for (int track_i = (int) tracks_.size() - 1; track_i >= 0; track_i--) {
		if (tracks_[track_i].beforedelete_) {

			tracks_[track_i].Destroy();
			tracks_.erase(tracks_.begin() + track_i);
		}
	}
}

void CLPTracker::SetBeforeDelete() {

	for (int track_i = (int) tracks_.size() - 1; track_i >= 0; track_i--) {

		long long inactivetime = lastimagetimestamp_
				- tracks_[track_i].lastactivetime_;
		if (inactivetime >= deletetime_) {
			tracks_[track_i].beforedelete_ = true;
		}
	}
}

void CLPTracker::CreateEvents() {

	events_.clear();

	for (int track_i = (int) tracks_.size() - 1; track_i >= 0; track_i--) {
		CLPTrack &curtrack = tracks_[track_i];
		if (curtrack.starttimestamp_ == lastimagetimestamp_) {
			events_.emplace_back();
			LPTEventS &newevent = events_.back();
			newevent.type_ = LPTEVENT_START;
			newevent.trackID_ = curtrack.trackID_;
			newevent.plateID_ = curtrack.trackplates_->back().plateID_;
			newevent.lastplatetimestamp_ =
					curtrack.trackplates_->back().timestamp_;
			strcpy(newevent.guess_, curtrack.guess_[0]);
			newevent.valid_ = curtrack.valid_;
			CLPTrack::CLPTrackLP &lastplate = curtrack.trackplates_->back();
			newevent.frame_[0] = lastplate.frame_[0];
			newevent.frame_[1] = lastplate.frame_[1];
			newevent.frame_[2] = lastplate.frame_[2];
			newevent.frame_[3] = lastplate.frame_[3];

			if (curtrack.incomingtimestamp_ != LPT_INVALID_TIMESTAMP) {
				newevent.isincoming_ = true;
			} else {
				newevent.isincoming_ = false;
			}
			if (curtrack.outgoingtimestamp_ != LPT_INVALID_TIMESTAMP) {
				newevent.isoutgoing_ = true;
			} else {
				newevent.isoutgoing_ = false;
			}
		}
	}

	for (int track_i = (int) tracks_.size() - 1; track_i >= 0; track_i--) {
		CLPTrack &curtrack = tracks_[track_i];
		if (curtrack.beforedelete_) {
			events_.emplace_back();
			LPTEventS &newevent = events_.back();
			newevent.type_ = LPTEVENT_END;
			newevent.trackID_ = curtrack.trackID_;
			newevent.plateID_ = curtrack.trackplates_->back().plateID_;
			newevent.lastplatetimestamp_ =
					curtrack.trackplates_->back().timestamp_;
			strcpy(newevent.guess_, curtrack.guess_[0]);
			newevent.valid_ = curtrack.valid_;
			CLPTrack::CLPTrackLP &lastplate = curtrack.trackplates_->back();
			newevent.frame_[0] = lastplate.frame_[0];
			newevent.frame_[1] = lastplate.frame_[1];
			newevent.frame_[2] = lastplate.frame_[2];
			newevent.frame_[3] = lastplate.frame_[3];

			if (curtrack.incomingtimestamp_ != LPT_INVALID_TIMESTAMP) {
				newevent.isincoming_ = true;
			} else {
				newevent.isincoming_ = false;
			}
			if (curtrack.outgoingtimestamp_ != LPT_INVALID_TIMESTAMP) {
				newevent.isoutgoing_ = true;
			} else {
				newevent.isoutgoing_ = false;
			}
		}
	}

	for (int track_i = (int) tracks_.size() - 1; track_i >= 0; track_i--) {
		CLPTrack &curtrack = tracks_[track_i];
		if ((curtrack.validatedtimestamp_ != LPT_INVALID_TIMESTAMP)
				&& (!curtrack.valideventgenerated_)) {
			events_.emplace_back();
			LPTEventS &newevent = events_.back();
			newevent.type_ = LPTEVENT_VALID;
			newevent.trackID_ = curtrack.trackID_;
			newevent.plateID_ = curtrack.trackplates_->back().plateID_;
			newevent.lastplatetimestamp_ =
					curtrack.trackplates_->back().timestamp_;
			strcpy(newevent.guess_, curtrack.guess_[0]);
			newevent.valid_ = curtrack.valid_;
			CLPTrack::CLPTrackLP &lastplate = curtrack.trackplates_->back();
			newevent.frame_[0] = lastplate.frame_[0];
			newevent.frame_[1] = lastplate.frame_[1];
			newevent.frame_[2] = lastplate.frame_[2];
			newevent.frame_[3] = lastplate.frame_[3];

			if (curtrack.incomingtimestamp_ != LPT_INVALID_TIMESTAMP) {
				newevent.isincoming_ = true;
			} else {
				newevent.isincoming_ = false;
			}
			if (curtrack.outgoingtimestamp_ != LPT_INVALID_TIMESTAMP) {
				newevent.isoutgoing_ = true;
			} else {
				newevent.isoutgoing_ = false;
			}

			curtrack.valideventgenerated_ = true;
			curtrack.lastvalideventtimestamp_ =
					curtrack.trackplates_->back().timestamp_;
			curtrack.eventresendcount_ = 0;
		}
	}

	if (enableeventresend_) {
		for (int track_i = (int) tracks_.size() - 1; track_i >= 0; track_i--) {
			CLPTrack &curtrack = tracks_[track_i];
			if (curtrack.valideventgenerated_
					&& (curtrack.eventresendcount_ < eventresendmaxcount_)
					&& ((curtrack.lastvalideventtimestamp_ + eventresenddelay_)
							<= curtrack.lastactivetime_)) {
				events_.emplace_back();
				LPTEventS &newevent = events_.back();
				newevent.type_ = LPTEVENT_VALID;
				newevent.trackID_ = curtrack.trackID_;
				newevent.plateID_ = curtrack.trackplates_->back().plateID_;
				newevent.lastplatetimestamp_ =
						curtrack.trackplates_->back().timestamp_;
				strcpy(newevent.guess_, curtrack.guess_[0]);
				newevent.valid_ = curtrack.valid_;
				CLPTrack::CLPTrackLP &lastplate = curtrack.trackplates_->back();
				newevent.frame_[0] = lastplate.frame_[0];
				newevent.frame_[1] = lastplate.frame_[1];
				newevent.frame_[2] = lastplate.frame_[2];
				newevent.frame_[3] = lastplate.frame_[3];

				if (curtrack.incomingtimestamp_ != LPT_INVALID_TIMESTAMP) {
					newevent.isincoming_ = true;
				} else {
					newevent.isincoming_ = false;
				}
				if (curtrack.outgoingtimestamp_ != LPT_INVALID_TIMESTAMP) {
					newevent.isoutgoing_ = true;
				} else {
					newevent.isoutgoing_ = false;
				}

				curtrack.lastvalideventtimestamp_ =
						curtrack.trackplates_->back().timestamp_;
				curtrack.eventresendcount_ += 1;
			}
		}
	}

	for (int track_i = (int) tracks_.size() - 1; track_i >= 0; track_i--) {
		CLPTrack &curtrack = tracks_[track_i];
		if ((curtrack.valid_)
				&& (curtrack.incomingtimestamp_ != LPT_INVALID_TIMESTAMP)) {
			if ((curtrack.incomingtimestamp_ == lastimagetimestamp_)
					|| (curtrack.guesschangetimestamp_ == lastimagetimestamp_)
					|| (curtrack.validatedtimestamp_ == lastimagetimestamp_)) {
				events_.emplace_back();
				LPTEventS &newevent = events_.back();
				newevent.type_ = LPTEVENT_INCOMING;
				newevent.trackID_ = curtrack.trackID_;
				newevent.plateID_ = curtrack.trackplates_->back().plateID_;
				newevent.lastplatetimestamp_ =
						curtrack.trackplates_->back().timestamp_;
				strcpy(newevent.guess_, curtrack.guess_[0]);
				newevent.valid_ = curtrack.valid_;
				CLPTrack::CLPTrackLP &lastplate = curtrack.trackplates_->back();
				newevent.frame_[0] = lastplate.frame_[0];
				newevent.frame_[1] = lastplate.frame_[1];
				newevent.frame_[2] = lastplate.frame_[2];
				newevent.frame_[3] = lastplate.frame_[3];

				if (curtrack.incomingtimestamp_ != LPT_INVALID_TIMESTAMP) {
					newevent.isincoming_ = true;
				} else {
					newevent.isincoming_ = false;
				}
				if (curtrack.outgoingtimestamp_ != LPT_INVALID_TIMESTAMP) {
					newevent.isoutgoing_ = true;
				} else {
					newevent.isoutgoing_ = false;
				}
			}
		}
	}

	for (int track_i = (int) tracks_.size() - 1; track_i >= 0; track_i--) {
		CLPTrack &curtrack = tracks_[track_i];
		if ((curtrack.valid_)
				&& (curtrack.outgoingtimestamp_ != LPT_INVALID_TIMESTAMP)) {
			if ((curtrack.outgoingtimestamp_ == lastimagetimestamp_)
					|| (curtrack.guesschangetimestamp_ == lastimagetimestamp_)
					|| (curtrack.validatedtimestamp_ == lastimagetimestamp_)) {
				events_.emplace_back();
				LPTEventS &newevent = events_.back();
				newevent.type_ = LPTEVENT_OUTGOING;
				newevent.trackID_ = curtrack.trackID_;
				newevent.plateID_ = curtrack.trackplates_->back().plateID_;
				newevent.lastplatetimestamp_ =
						curtrack.trackplates_->back().timestamp_;
				strcpy(newevent.guess_, curtrack.guess_[0]);
				newevent.valid_ = curtrack.valid_;
				CLPTrack::CLPTrackLP &lastplate = curtrack.trackplates_->back();
				newevent.frame_[0] = lastplate.frame_[0];
				newevent.frame_[1] = lastplate.frame_[1];
				newevent.frame_[2] = lastplate.frame_[2];
				newevent.frame_[3] = lastplate.frame_[3];

				if (curtrack.incomingtimestamp_ != LPT_INVALID_TIMESTAMP) {
					newevent.isincoming_ = true;
				} else {
					newevent.isincoming_ = false;
				}
				if (curtrack.outgoingtimestamp_ != LPT_INVALID_TIMESTAMP) {
					newevent.isoutgoing_ = true;
				} else {
					newevent.isoutgoing_ = false;
				}
			}
		}
	}

	for (int track_i = (int) tracks_.size() - 1; track_i >= 0; track_i--) {
		CLPTrack &curtrack = tracks_[track_i];
		if ((curtrack.valid_)
				&& (curtrack.stoppedtimestamp_ != LPT_INVALID_TIMESTAMP)) {
			if ((curtrack.stoppedtimestamp_ == lastimagetimestamp_)
					|| (curtrack.guesschangetimestamp_ == lastimagetimestamp_)
					|| (curtrack.validatedtimestamp_ == lastimagetimestamp_)) {
				events_.emplace_back();
				LPTEventS &newevent = events_.back();
				newevent.type_ = LPTEVENT_STOPPED;
				newevent.trackID_ = curtrack.trackID_;
				newevent.plateID_ = curtrack.trackplates_->back().plateID_;
				newevent.lastplatetimestamp_ =
						curtrack.trackplates_->back().timestamp_;
				strcpy(newevent.guess_, curtrack.guess_[0]);
				newevent.valid_ = curtrack.valid_;
				CLPTrack::CLPTrackLP &lastplate = curtrack.trackplates_->back();
				newevent.frame_[0] = lastplate.frame_[0];
				newevent.frame_[1] = lastplate.frame_[1];
				newevent.frame_[2] = lastplate.frame_[2];
				newevent.frame_[3] = lastplate.frame_[3];

				if (curtrack.incomingtimestamp_ != LPT_INVALID_TIMESTAMP) {
					newevent.isincoming_ = true;
				} else {
					newevent.isincoming_ = false;
				}
				if (curtrack.outgoingtimestamp_ != LPT_INVALID_TIMESTAMP) {
					newevent.isoutgoing_ = true;
				} else {
					newevent.isoutgoing_ = false;
				}
			}
		}
	}
}

void CLPTracker::SetEvenResend(bool _enableeventresend,
		long long _eventresenddelay, unsigned int _eventresendmaxcount) {
	enableeventresend_ = _enableeventresend;
	eventresenddelay_ = _eventresenddelay;
	eventresendmaxcount_ = _eventresendmaxcount;
}

void CLPTracker::GetEvenResend(bool *_enableeventresend,
		long long *_eventresenddelay,
		unsigned int *_eventresendmaxcount) const {
	*_enableeventresend = enableeventresend_;
	*_eventresenddelay = eventresenddelay_;
	*_eventresendmaxcount = eventresendmaxcount_;
}

void CLPTracker::CLPTrack::Init(unsigned int _trackID) {
	trackID_ = _trackID;
	trackplates_ = new std::vector<CLPTrackLP>;
	valid_ = false;
	totaltime_ = 0;
	validatedtimestamp_ = LPT_INVALID_TIMESTAMP;
	guesschangetimestamp_ = LPT_INVALID_TIMESTAMP;
	incomingtimestamp_ = LPT_INVALID_TIMESTAMP;
	outgoingtimestamp_ = LPT_INVALID_TIMESTAMP;
	stoppedtimestamp_ = LPT_INVALID_TIMESTAMP;
	beforedelete_ = false;
	guess_[0][0] = 0;
	guess_[1][0] = 0;
	valideventgenerated_ = false;
	lastvalideventtimestamp_ = LPT_INVALID_TIMESTAMP;
}

void CLPTracker::CLPTrack::Destroy() {
	delete trackplates_;
}

void CLPTracker::CLPTrack::AddPlate(CLPTracker::CLPTrack::CLPTrackLP &_plate) {

	trackplates_->push_back(_plate);
	lastactivetime_ = _plate.timestamp_;

	if (trackplates_->size() == 1) {
		starttimestamp_ = _plate.timestamp_;
	}

	UpdateStats();
}

void CLPTracker::CLPTrack::UpdateStats() {

	if (trackplates_->size() < 2) {
		totaltime_ = 0;
	} else {
		totaltime_ =
				(unsigned int) ((*trackplates_)[trackplates_->size() - 1].timestamp_
						- (*trackplates_)[0].timestamp_);
	}

	unsigned int start_i;
	if (trackplates_->size() > 50) {
		start_i = trackplates_->size() - 50;
	} else {
		start_i = 0;
	}

	char guess[ LPREADER_MAX_CHAR_PER_PLATE + 1][50];
	unsigned int count[50];
	unsigned int guessused = 0;
	for (unsigned int tp_i = start_i; tp_i < trackplates_->size(); tp_i++) {
		CLPTrackLP &curplate = (*trackplates_)[tp_i];
		unsigned int guess_i;
		for (guess_i = 0; guess_i < guessused; guess_i++) {
			if (0 == strcmp(guess[guess_i], curplate.fulltext_)) {

				count[guess_i] += 1;
				break;
			}
		}
		if (guess_i == guessused) {

			strcpy(guess[guessused], curplate.fulltext_);
			count[guessused] = 1;
			guessused++;
		}
	}

	unsigned int bestc[2] { };
	unsigned int best_i[2] { };
	for (unsigned int guess_i = 0; guess_i < guessused; guess_i++) {
		if (count[guess_i] > bestc[1]) {
			bestc[1] = count[guess_i];
			best_i[1] = guess_i;
			if (bestc[1] > bestc[0]) {
				bestc[1] = bestc[0];
				best_i[1] = best_i[0];
				bestc[0] = count[guess_i];
				best_i[0] = guess_i;
			}
		}
	}
	guessmatchcount_ = bestc[0];

	if (0 != strcmp(guess_[0], guess[best_i[0]])) {
		strcpy(guess_[0], guess[best_i[0]]);
		guesschangetimestamp_ = lastactivetime_;
	}
	if (bestc[1] == 0) {
		guess_[1][0] = 0;
	} else {
		strcpy(guess_[1], guess[best_i[1]]);
	}

	if (trackplates_->size() != 1) {
		float lastcharsize = trackplates_->back().charsize_;

		const float sizechangefactor = 1.1f;

		if (incomingtimestamp_ == LPT_INVALID_TIMESTAMP) {
			float charsizethr = lastcharsize / sizechangefactor;
			unsigned int tp_i;
			for (tp_i = 0; tp_i < trackplates_->size() - 1; tp_i++) {
				CLPTrackLP &curplate = (*trackplates_)[tp_i];
				if (curplate.charsize_ < charsizethr) {
					break;
				}
			}
			if (tp_i != (trackplates_->size() - 1)) {
				incomingtimestamp_ = lastactivetime_;
			}
		}

		if (outgoingtimestamp_ == LPT_INVALID_TIMESTAMP) {
			float charsizethr = lastcharsize * sizechangefactor;
			unsigned int tp_i;
			for (tp_i = 0; tp_i < trackplates_->size() - 1; tp_i++) {
				CLPTrackLP &curplate = (*trackplates_)[tp_i];
				if (curplate.charsize_ > charsizethr) {
					break;
				}
			}
			if (tp_i != (trackplates_->size() - 1)) {
				outgoingtimestamp_ = lastactivetime_;
			}
		}

		const long long stoppedinterval = 500;
		const float stoppedmaxdistx = 0.05f;
		const float stoppedmaxdisty = 0.05f;
		if (stoppedtimestamp_ == LPT_INVALID_TIMESTAMP) {
			CPointf16_16 &lastplatecenter = trackplates_->back().center_;
			float &lastplatecharsize = trackplates_->back().charsize_;
			bool isinstoppeddist = true;
			bool outsideintervalchecked = false;

			unsigned int checkingtp_i = trackplates_->size() - 2;
			while (isinstoppeddist && (!outsideintervalchecked)) {

				CLPTrackLP &curplate = (*trackplates_)[checkingtp_i];

				f16_16 curdistx = lastplatecenter.x - curplate.center_.x;
				f16_16 curdisty = lastplatecenter.y - curplate.center_.y;
				if (curdistx < 0) {
					curdistx = -curdistx;
				}
				if (curdisty < 0) {
					curdisty = -curdisty;
				}

				float curdistxch = ((float) curdistx / 65536.0f)
						/ lastplatecharsize;
				float curdistych = ((float) curdisty / 65536.0f)
						/ lastplatecharsize;

				if ((curdistxch > stoppedmaxdistx)
						|| (curdistych > stoppedmaxdisty)) {
					isinstoppeddist = false;
				}

				long long timedist = lastactivetime_ - curplate.timestamp_;
				if (timedist >= stoppedinterval) {
					outsideintervalchecked = true;
				}

				if (checkingtp_i == 0) {
					break;
				} else {
					checkingtp_i--;
				}
			}

			if (isinstoppeddist && outsideintervalchecked) {
				stoppedtimestamp_ = lastactivetime_;
			}
		}
	}
}

void CLPTracker::CLPTrack::CheckValidation(long long _curtimestamp) {

	if (validatedtimestamp_ == LPT_INVALID_TIMESTAMP) {

		if (((_curtimestamp - (*trackplates_)[0].timestamp_ > 2000)
				&& (guessmatchcount_ >= 2))
				|| ((_curtimestamp
						- (*trackplates_)[trackplates_->size() - 1].timestamp_
						> 500) && (guessmatchcount_ >= 2))
				|| ((guessmatchcount_ >= 3)
						&& ((incomingtimestamp_ != LPT_INVALID_TIMESTAMP)
								|| (outgoingtimestamp_ != LPT_INVALID_TIMESTAMP)))) {
			valid_ = true;
			validatedtimestamp_ = lastactivetime_;
		}
	}
}

float CLPTracker::CLPTrack::GetMatchPossByText(CLPTrack::CLPTrackLP &_plate) {

	long long elapsedtime = _plate.timestamp_ - lastactivetime_;
	if (elapsedtime == 0) {
		return -1.0f;
	}

	const float maxretv = 29.0f;

	unsigned int minmissscore = 9999;
	for (int guess_i = 0; guess_i < 2; guess_i++) {

		unsigned int missscore = 0;
		for (unsigned int char_i = 0; char_i < strlen(_plate.fulltext_);
				char_i++) {
			char curchar = _plate.fulltext_[char_i];
			unsigned int curscore = 15;
			for (unsigned int char_i2 = 0; char_i2 < strlen(guess_[guess_i]);
					char_i2++) {
				if (curchar == guess_[guess_i][char_i2]) {

					unsigned int curdist;
					if (char_i2 > char_i) {
						curdist = char_i2 - char_i;
					} else {
						curdist = char_i - char_i2;
					}
					if (curscore > curdist) {
						curscore = curdist;
					}
					if (curscore == 0) {
						break;
					}
				}
			}
			missscore += curscore;
		}

		int charnumdiff = strlen(guess_[guess_i]) - strlen(_plate.fulltext_);
		if (charnumdiff > 0) {
			missscore += 15 * charnumdiff;
		}

		if (minmissscore > missscore) {
			minmissscore = missscore;
		}
		if (minmissscore == 0) {
			return maxretv;
		}
	}

	if (strlen(_plate.fulltext_) < strlen(guess_[0])) {

		if ((0
				== strcmp(_plate.fulltext_,
						guess_[0] + strlen(guess_[0])
								- strlen(_plate.fulltext_)))) {
			minmissscore = (unsigned int) maxretv - 1;
		}
	}
	if (strlen(_plate.fulltext_) < strlen(guess_[1])) {

		if ((0
				== strcmp(_plate.fulltext_,
						guess_[1] + strlen(guess_[1])
								- strlen(_plate.fulltext_)))) {
			minmissscore = (unsigned int) maxretv - 1;
		}
	}

	return ((float) maxretv - minmissscore);
}

void CLPTracker::CLPTrack::CLPTrackLP::Set(CLPReader::PlateS &_plate,
		long long _timestamp) {
	timestamp_ = _timestamp;

	charsize_ = _plate.charheight_;

	center_.x = _plate.posx_ << 16;
	center_.y = _plate.posy_ << 16;

	strcpy(fulltext_, _plate.fulltext_);

	frame_[0] = _plate.frame_[0];
	frame_[1] = _plate.frame_[1];
	frame_[2] = _plate.frame_[2];
	frame_[3] = _plate.frame_[3];
}
