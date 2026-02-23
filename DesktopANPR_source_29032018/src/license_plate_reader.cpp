#include "license_plate_reader.h"

#include "minimal_data/HAData.h"
#include "lpreader/LPReader.h"
#include "lpreader/LPTracker.h"
#include "lpreader/MipMapper.h"
#include "lpreader/NetImage.h"

LicensePlateReader::LicensePlateReader() :
		mIsInitialized(false), mTracker(new CLPTracker), mReader(new CLPReader), mMipMapper(
				new CMipMapper), mConfiguration(nullptr) {
}

LicensePlateReader::~LicensePlateReader() {
}

void LicensePlateReader::Configure(const AnprConfiguration& config) {
	if (!mIsInitialized) {
		return;
	}

	mConfiguration.reset(new AnprConfiguration(config));

	if (mConfiguration->masks.size() != 1) {
		mReader->SetDetectionArea(nullptr);
	} else {
		const std::vector<int16_t>& mask = mConfiguration->masks[0];
		if (mask.empty()) {
			mReader->SetDetectionArea(nullptr);
		} else {
			mReader->SetDetectionArea(new const std::vector<int16_t>(mask));
		}
	}

	if (mConfiguration->maximumCharacterSize
			< mConfiguration->minimumCharacterSize) {
		auto max = mConfiguration->maximumCharacterSize;
		mConfiguration->maximumCharacterSize =
				mConfiguration->minimumCharacterSize;
		mConfiguration->minimumCharacterSize = max;
	}

	mReader->SetCharSizes(mConfiguration->minimumCharacterSize,
			mConfiguration->maximumCharacterSize);

	mTracker->SetEvenResend(mConfiguration->enableEventResend,
			mConfiguration->eventResendDelay,
			mConfiguration->eventResendMaximumCount);
}

bool LicensePlateReader::Initialize(const std::string& path) {
	mIsInitialized = false;

	CHADataManager lprdataman;
	CHAData *lprdataroot = lprdataman.GetRoot();
	if (lprdataman.LoadFromFile(path.c_str())) {

		if (mReader->Init(lprdataroot)) {
			mReader->SetMipMapper(mMipMapper.get());
			mIsInitialized = true;
		}
	}

	return mIsInitialized;
}

AnprResults_uptr LicensePlateReader::Process(const AnprImage& image,
		int64_t timestamp) {
	AnprResults_uptr result;
	if (!mIsInitialized) {
		return result;
	}
	if (!mConfiguration) {
		return result;
	}

	CNetImage netimage;

	// Map external image to internal container
	netimage.CreateMap(image.width, image.height, image.scanline, image.data);

	// Set new image
	mMipMapper->SetSrcImage(&netimage);

	// Detect plates
	mReader->ProcessImage();

    result.reset(new AnprResults);

    if( timestamp > -1 ) // still image assumed
    {
        // Detect movement
        mTracker->TrackPlates(timestamp, mReader->plates_, netimage.width_,
                netimage.height_);

        char lastaddedeventtext[ LPREADER_MAX_CHAR_PER_PLATE + 1];
        lastaddedeventtext[0] = 0;


        // Extract results
        for (unsigned int event_i = 0; event_i < mTracker->events_.size();
                event_i++) {
            LPTEventS &curevent = mTracker->events_[event_i];

            bool addthisevent = false;
            switch ((int32_t) mConfiguration->directionFilter) {
            case 0:
                if ((curevent.type_ == LPTEVENT_VALID)
                        || ((curevent.type_ == LPTEVENT_GUESSCHANGED)
                                && (curevent.valid_ == true))) {
                    addthisevent = true;
                }
                break;
            case 1:
                if ((curevent.type_ == LPTEVENT_INCOMING)
                        && (curevent.valid_ == true)) {
                    addthisevent = true;
                }
                break;
            case 2:
                if ((curevent.type_ == LPTEVENT_OUTGOING)
                        && (curevent.valid_ == true)) {
                    addthisevent = true;
                }
                break;
            };

            if (addthisevent) {
                if (0 != strcmp(lastaddedeventtext, curevent.guess_)) {
                    strcpy(lastaddedeventtext, curevent.guess_);

                    AnprResult data;
                    data.text = curevent.guess_;
                    data.timestamp = curevent.lastplatetimestamp_;
                    data.direction = AnprDirection::UNDEFINED;

                    if ((curevent.isincoming_ && curevent.isoutgoing_)
                            || ((!curevent.isincoming_) && (!curevent.isoutgoing_))) {
                        data.direction = AnprDirection::UNDEFINED;
                    } else if( curevent.isincoming_ ) {
                        data.direction = AnprDirection::APPROACHING;
                    } else if( curevent.isoutgoing_ ) {
                        data.direction = AnprDirection::MOVING_AWAY;
                    }

                    result->push_back(data);
                }
            }
        }
    }
    else
    {
        for (unsigned int plate_i = 0; plate_i < mReader->plates_.size(); plate_i++) {
            CLPReader::PlateS &curplate = mReader->plates_[plate_i];
            AnprResult data;
            data.text = curplate.fulltext_;
            data.timestamp = -1;
            data.direction = AnprDirection::UNDEFINED;

            result->push_back(data);
        }
    }

	return result;
}
