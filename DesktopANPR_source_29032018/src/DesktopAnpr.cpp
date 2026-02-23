#include <iostream>
#include <ctime>
#include <fstream>
#include <thread>
#include <chrono>

#include "logger.h"
#include "license_plate_reader.h"

inline int64_t GetMonotonTimeMs()
{
	struct timespec t;
	clock_gettime(CLOCK_MONOTONIC_RAW, &t);
	return (t.tv_sec) * 1000ll + (t.tv_nsec) / 1000000ll;
}

int main()
{
	LicensePlateReader reader;

	LOG_INFO("Init reader");
	reader.Initialize("data/lpr.lprdat");
	LOG_INFO("Init complete");

	LOG_INFO("Configure");
	AnprConfiguration config;
	config.enableEventResend = false;
	config.minimumCharacterSize = 10;
	config.maximumCharacterSize = 60;
	config.directionFilter = AnprDirection::UNDEFINED;
	reader.Configure(config);
	LOG_INFO("Configure complete");

	LOG_INFO("Start detection");
	std::vector< std::vector<uint8_t> > images;
	std::vector<std::string> paths = {
			"test/photo_2018-03-12_10-27-30.gray",
			"test/photo_2018-03-12_10-28-08.gray",
	};

	for(const auto& path : paths)
	{
		std::ifstream file(path, std::ios::binary | std::ios::ate);
		std::streamsize size = file.tellg();
		file.seekg(0, std::ios::beg);

		std::vector<uint8_t> image;
		image.resize(size);
		if (!file.read((char*)image.data(), size))
		{
			LOG_ERROR("Loading image failed!");
		} else {
			images.emplace_back(image);
			LOG_INFO("Image size: %u", image.size());
		}
	}

	size_t image_show = 100;
	size_t counter = 0;
	size_t idx = 0;
	for(size_t i=0; i<300; ++i)
	{

		AnprImage anprImage;
		anprImage.width = 1280;
		anprImage.height = 960;
		anprImage.scanline = anprImage.width;
		counter++;
		if(counter == image_show)
		{
			counter = 0;
			idx++;
			LOG_INFO("Change image");
		}
		anprImage.data = images[idx % images.size()].data();

		auto ts = GetMonotonTimeMs();
		LOG_INFO("Detect @ %lld", ts);
		auto result = reader.Process(anprImage, ts);
		if(result)
		{
			for(const auto& item : *result)
			{
				LOG_INFO("License plate detected: %s (%d - %lld)", item.text, item.direction, item.timestamp);
			}
		} else {
			LOG_INFO("No detection");
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	return 0;
}
