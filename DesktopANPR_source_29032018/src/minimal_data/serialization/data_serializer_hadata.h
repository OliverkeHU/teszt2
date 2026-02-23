#ifndef BASE_CDATA_SERIALIZATION_DATA_SERIALIZER_HADATA_H_
#define BASE_CDATA_SERIALIZATION_DATA_SERIALIZER_HADATA_H_

#include <algorithm>
#include "../data.h"

namespace data_serializer {
class hadata {
	struct deserialization_data {
		bool ok;
		MinimalDataType typ;
		std::string free_name;
		deserialization_data() :
				ok(true), typ(MinimalDataType::EMPTY), free_name() {
		}
	};

	static void deserialize_name_type(const char*& ptr,
			deserialization_data& d) {

		switch (*ptr) {
		case 0:
			d.typ = MinimalDataType::T_BOOL;
			break;
		case 1:
			d.typ = MinimalDataType::T_INT32;
			break;
		case 2:
			d.typ = MinimalDataType::T_DOUBLE;
			break;
		case 3:
			d.typ = MinimalDataType::T_STRING;
			break;
		case 4:
			d.typ = MinimalDataType::T_INT8_ARRAY;
			break;
		case 5:
			d.typ = MinimalDataType::NODE;
			break;
		default:
			d.ok = false;
			break;
		}
		if (d.ok) {
			uint32_t name_len = ptr[1];
			if (name_len) {
				std::string _name(ptr + 2, name_len);
				d.free_name = std::move(_name);
			}

			ptr = ptr + name_len + 2;
		}
	}

	template<typename F>
	static void deserialize_helper(MinimalData& cont,
			const deserialization_data& d, const char *&data, F f) {
		cont.add(d.free_name, f(data));
	}

	static bool deserialize_imp(MinimalData& cont, uint32_t _cnt,
			const char *&p, uint32_t _size) {
		bool ok = true;
		bool end = false;
		const char * const pbegin = p;
		cont.clear();
		uint32_t parsed_elems = 0;
		do {
			deserialization_data d;
			deserialize_name_type(p, d);
			if (!d.ok) {
				break;
			}
			switch (d.typ) {
			case MinimalDataType::T_BOOL:
				deserialize_helper(cont, d, p,
						[] (const char *&data) -> bool {int32_t res; std::copy_n(data, sizeof(res), (char*)&res); data += sizeof(res); return (bool)res;});
				break;
			case MinimalDataType::T_INT32:
				deserialize_helper(cont, d, p,
						[] (const char *&data) -> int32_t {int32_t res; std::copy_n(data, sizeof(res), (char*)&res); data += sizeof(res); return res;});
				break;
			case MinimalDataType::T_DOUBLE:
				deserialize_helper(cont, d, p,
						[] (const char *&data) -> double {float res; std::copy_n(data, sizeof(res), (char*)&res); data += sizeof(res); return (double)res;});
				break;
			case MinimalDataType::T_STRING:
				deserialize_helper(cont, d, p,
						[] (const char *&data) -> std::string {uint32_t size; std::copy_n(data, sizeof(size), (char*)&size); data += sizeof(size); std::string res(data, size); data += size; return res;});
				break;
			case MinimalDataType::T_INT8_ARRAY:
				deserialize_helper(cont, d, p,
						[] (const char *&data) -> std::vector<int8_t> {uint32_t size; std::copy_n(data, sizeof(size), (char*)&size); data += sizeof(size); std::vector<int8_t> res((int8_t*)data, (int8_t*)data+size); data += size; return res;});
				break;
			case MinimalDataType::NODE: {
				uint32_t cnt;
				std::copy_n(p, sizeof(cnt), (char*) &cnt);
				p += sizeof(cnt);
				MinimalData& subcont = cont.add<MinimalData>(d.free_name);
				ok = hadata::deserialize_imp(subcont, cnt, p,
						_size - (p - pbegin));
			}
				break;
			default:
				break;
			}
			if (!ok) {
				break;
			}
			++parsed_elems;
		} while (parsed_elems < _cnt && !end && (p < pbegin + _size - 1));
		if (!ok) {
			cont.clear();
		}
		return ok;
	}

public:
	static bool deserialize(MinimalData& data, const char *ptr, uint32_t size) {
		bool res = false;
		data.clear();
		if (ptr[0] == 5 && ptr[1] == 0) {
			uint32_t cnt;
			std::copy_n(ptr + 2, sizeof(cnt), (char*) &cnt);
			const char *p = ptr + 2 + sizeof(cnt);
			res = deserialize_imp(data, cnt, p, size - 2 + sizeof(cnt));
		}
		return res;
	}
};
}

#endif
