#ifndef _HADATA_H_INCLUDED_
#define _HADATA_H_INCLUDED_

#include <unordered_map>
#include <assert.h>
#include <minimal_data/data.h>

enum HADATA_TYPE {
	HADATA_BOOL = 0,
	HADATA_INT = 1,
	HADATA_FLOAT = 2,
	HADATA_STRING = 3,
	HADATA_RAW = 4,
	HADATA_HAD = 5,
	HADATA_UNKNOWN = -1,
};

struct CHAData;

class CHADataManager {
private:
	MinimalData root_data_;
	CHAData *root_;
	std::unordered_map<MinimalData*, std::unique_ptr<CHAData> > CHADataMap;

public:
	CHADataManager();
	~CHADataManager();
	CHAData *GetRoot();

	bool LoadFromFile(const char *_filename);

	bool GetHAData(struct CHAData * _HADhead, unsigned int _index,
			struct CHAData **_hadata);
	bool GetHAData(struct CHAData * _HADhead, const char* _name,
			struct CHAData **_hadata);
};

struct CHAData {
	friend class CHADataManager;
private:
	CHADataManager *HADMan_;
	MinimalData &data_;

public:
	CHAData(CHADataManager* _mgr, MinimalData& _data) :
			HADMan_(_mgr), data_(_data) {
	}

	inline int GetInt(unsigned int _index, int _errorret = 0) {
		return data_.get<int32_t>(_index, _errorret);
	}
	inline CHAData* GetHAData(unsigned int _index) {
		CHAData* retval = nullptr;
		if (HADMan_->GetHAData(this, _index, &retval)) {
			return retval;
		}
		return nullptr;
	}

	inline int GetInt(const char *_name, int _errorret = 0) {
		return data_.get<int32_t>(_name, _errorret);
	}
	inline float GetFloat(const char *_name, float _errorret = 0.0f) {
		return static_cast<float>(data_.get<double>(_name,
				static_cast<double>(_errorret)));
	}
	inline char* GetRawData(const char *_name, unsigned int *_length,
			char *_errorret = nullptr) {
		*_length = 0;
		auto d = data_.find<std::vector<int8_t> >(_name);
		if (d) {
			*_length = (*d).size();
			return const_cast<char*>((const char*) (*d).data());
		} else {
			return _errorret;
		}
	}
	inline CHAData* GetHAData(const char *_name) {
		CHAData* retval = nullptr;
		if (HADMan_->GetHAData(this, _name, &retval)) {
			return retval;
		}
		return nullptr;
	}

	inline unsigned int GetCount() const {
		return data_.size();
	}
};

#endif 
