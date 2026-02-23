#ifndef _VCA_LPREADER_H_INCLUDED_
#define _VCA_LPREADER_H_INCLUDED_

#include <stdint.h>
#include <vector>
#include <list>
#include <memory>

#include "logger.h"

class CLPReader;
class CLPTracker;
class CMipMapper;

/**
 * Direction of the license plate
 */
enum class AnprDirection : int32_t
{
	UNDEFINED = 0, //!< License plate direction is not defined
	ANY = UNDEFINED, //!< License plate filter allows any direction
    APPROACHING = 1, //!< License plate is approaching viewport
    MOVING_AWAY = 2 //!< License plate is moving away from viewport
};

/**
 * Configuration for the license plate reader
 */
struct AnprConfiguration {
	// Recommended default values for testing
	static const int32_t DEFAULT_MINIMUM_CHARACTER_SIZE = 32;
	static const int32_t DEFAULT_MAXIMUM_CHARACTER_SIZE = 128;
	static const AnprDirection DEFAULT_DIRECTION_FILTER = AnprDirection::ANY;

	// License plate height - bigger height range will result in slower detection
	int32_t minimumCharacterSize; // Minimum height of license plate characters in pixels
	int32_t maximumCharacterSize; // Maximum height of license plate characters in pixels

	AnprDirection directionFilter; // Filter license plate by direction

	// Event resending allows periodic repetition of a license plate
	// Useful for application where a single read may be missed (like crossing gates)
	bool enableEventResend; // Enabled event repetition
	int64_t eventResendDelay; // Delay between two repetitions
	int32_t eventResendMaximumCount; // Number of times an event is repeated

	// Mask of the detection are in virtual coordinates. Empty means entire image.
	// Each 'std::vector<int16_t>' defines the coordinates of a polygon like this:
	//    [x0, y0, x1, y1, x2, y2, ...]
	// The coordinates are virtual coordinates and can be calculated like this:
	//    virtual_x = ( image_x / 16384 + image_width ) / image_width
	//    virtual_y = ( image_y / 16384 + image_width ) / image_width
	std::vector<std::vector<int16_t> > masks;
};

/**
 * Grayscale image descriptor
 */
struct AnprImage {
	size_t width; // Width of the image in pixels
	size_t scanline; // Width of a single row in bytes
	size_t height; // Height of the image in pixels

	// A raw pointer to a grayscale image containing 8bit pixels
	// The length of the data should be (scanline * height) bytes
	const uint8_t* data;
};

/**
 * License plate reader detection result
 */
struct AnprResult {
	std::string text; // Detected license plate
	AnprDirection direction; // Moving direction of the license plate
	int64_t timestamp; // Timestamp of the detection
};

typedef std::list<AnprResult> AnprResults; // List of license plate detections
typedef std::unique_ptr<AnprResults> AnprResults_uptr;

/**
 * License plate reader is capable of detecting license plates in a series of images.
 * For a validated license plate detection multiple images of a moving license plate must be provided.
 * Before usage the reader must be initialized with a data file then configured.
 */
class LicensePlateReader final {
private:
	bool mIsInitialized;
	std::unique_ptr<CLPTracker> mTracker;
	std::unique_ptr<CLPReader> mReader;
	std::unique_ptr<CMipMapper> mMipMapper;
	std::unique_ptr<AnprConfiguration> mConfiguration;

public:
	LicensePlateReader();
	~LicensePlateReader();

	LicensePlateReader(const LicensePlateReader&) = delete;
	LicensePlateReader(LicensePlateReader&&) = delete;
	LicensePlateReader& operator=(LicensePlateReader&&) = delete;

	/**
	 * License plate reader must be initialized before usage
	 * @param path Path to the license plate data file
	 * @return True if the data file could be loaded, false otherwise
	 */
	bool Initialize(const std::string& path);

	/**
	 * A configuration must be set before usage
	 * @param config License plate reader configuration
	 */
	void Configure(const AnprConfiguration& config);

	/**
	 * Run the license plate reader on an image
	 * @param image An image possibly containing a license plate
	 * @param timestamp A monotonic timestamp in milliseconds to evaluate movements in time domain
	 * @return Possible detection results. A nullptr is returned if the license plate reader is in an error state.
	 */
	AnprResults_uptr Process(const AnprImage& image, int64_t timestamp);
};

#endif //_VCA_LPREADER_H_INCLUDED_
