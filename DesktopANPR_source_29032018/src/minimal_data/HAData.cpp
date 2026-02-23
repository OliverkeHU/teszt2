#include <minimal_data/data_serializer.h>
#include <minimal_data/HAData.h>
#include <stdio.h>

CHADataManager::CHADataManager() :
		root_(new CHAData(this, root_data_)) {
}

CHADataManager::~CHADataManager() {
	delete root_;
	root_ = nullptr;
}

CHAData *CHADataManager::GetRoot() {
	return root_;
}

bool CHADataManager::GetHAData(struct CHAData * _HADhead, unsigned int _index,
		struct CHAData **_hadata) {
	if (_HADhead->data_.type_of(_index)
			== _HADhead->data_.type_of<MinimalData>()) {
		MinimalData* ptr = &(_HADhead->data_.get<MinimalData>(_index));
		auto structIt = CHADataMap.find(ptr);
		if (structIt == CHADataMap.end()) {
			CHADataMap.emplace(ptr,
					std::unique_ptr<CHAData>(new CHAData(this, *ptr)));
		}

		*_hadata = &(*CHADataMap[ptr]);

		return true;
	} else {
		return false;
	}
}

bool CHADataManager::GetHAData(struct CHAData * _HADhead, const char* _name,
		struct CHAData **_hadata) {
	auto dIt = _HADhead->data_.find<MinimalData>(_name);
	if (dIt) {
		MinimalData* ptr = &(*dIt);
		auto structIt = CHADataMap.find(ptr);
		if (structIt == CHADataMap.end()) {
			CHADataMap.emplace(ptr,
					std::unique_ptr<CHAData>(new CHAData(this, *ptr)));
		}

		*_hadata = &(*CHADataMap[ptr]);

		return true;
	} else {
		return false;
	}
}

bool CHADataManager::LoadFromFile(const char *_filename) {
	FILE *fh = fopen(_filename, "rb");
	if (fh == nullptr) {
		return false;
	}

	std::vector<char> buff(4096, 0);
	size_t sum_read_bytes = 0;
	size_t currently_read_bytes = 0;

	currently_read_bytes = fread(buff.data(), 1, 512, fh);
	while (currently_read_bytes) {
		sum_read_bytes += currently_read_bytes;
		if (sum_read_bytes + 512 >= buff.size()) {
			buff.resize(buff.size() + 1024);
		}

		currently_read_bytes = fread(buff.data() + sum_read_bytes, 1, 512, fh);
	}
	fclose(fh);

	data_serializer::hadata::deserialize(root_->data_, buff.data(),
			sum_read_bytes);

	return true;
}

