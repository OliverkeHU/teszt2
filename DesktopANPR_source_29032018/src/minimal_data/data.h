#ifndef __DATA_H__
#define __DATA_H__

#include <stddef.h>
#include <utility>
#include <cstdint>
#include <vector>
#include <memory>
#include <type_traits>
#include <string>
#include <limits>
#include <unordered_map>

enum class MinimalDataType
	: uint16_t
	{
		EMPTY = 0,
	T_BOOL = 1 << 0,
	T_INT32 = 1 << 1,
	T_DOUBLE = 1 << 2,
	T_STRING = 1 << 3,
	T_INT8_ARRAY = 1 << 4,
	NODE = 1 << 5,
};

template<size_t arg1, size_t ... others>
struct static_max;

template<size_t arg>
struct static_max<arg> {
	static const size_t value = arg;
};

template<size_t arg1, size_t arg2, size_t ... others>
struct static_max<arg1, arg2, others...> {
	static const size_t value =
			arg1 >= arg2 ?
					static_max<arg1, others...>::value :
					static_max<arg2, others...>::value;
};

template<typename ... Ts>
struct max_types_size {
	static const size_t data_size = static_max<sizeof(Ts)...>::value;
	static const size_t data_align = static_max<alignof(Ts)...>::value;
};

class MinimalData;

template<typename T> struct elem_traits;
template<> struct elem_traits<bool> {
	static const MinimalDataType id = MinimalDataType::T_BOOL;
};
template<> struct elem_traits<int32_t> {
	static const MinimalDataType id = MinimalDataType::T_INT32;
};
template<> struct elem_traits<double> {
	static const MinimalDataType id = MinimalDataType::T_DOUBLE;
};
template<> struct elem_traits<std::string> {
	static const MinimalDataType id = MinimalDataType::T_STRING;
};
template<> struct elem_traits<std::vector<int8_t>*> {
	static const MinimalDataType id = MinimalDataType::T_INT8_ARRAY;
};
template<> struct elem_traits<MinimalData*> {
	static const MinimalDataType id = MinimalDataType::NODE;
};

template<typename Type, typename Collection>
struct contains;

template<typename Type>
struct contains<Type, std::tuple<>> {
	static constexpr int value = std::false_type::value;
};

template<typename Type, typename ... Others>
struct contains<Type, std::tuple<Type, Others...>> {
	static constexpr int value = std::true_type::value;
};

template<typename Type, typename First, typename ... Others>
struct contains<Type, std::tuple<First, Others...>> {
	static constexpr int value = contains<Type, std::tuple<Others...>>::value;
};

typedef std::tuple<bool, int32_t, double, std::string> pod_elem_tuple_t;
typedef std::tuple<std::vector<int8_t>, MinimalData> ptr_elem_tuple_t;

template<typename T>
struct is_pod_elem {
	static constexpr int value = contains<T, pod_elem_tuple_t>::value;
};

template<typename T>
struct is_ptr_elem {
	static constexpr int value = contains<T, ptr_elem_tuple_t>::value;
};

class MinimalData {
private:
	class MinimalDataElem {
	public:
		typedef std::string str_t;
		typedef std::vector<int8_t> vec8_t;

		MinimalDataElem() :
				type_id(MinimalDataType::EMPTY) {
		}

		MinimalDataElem(const MinimalDataElem& e) {
			switch (e.type_id) {
			case MinimalDataType::EMPTY:
			case MinimalDataType::T_BOOL:
			case MinimalDataType::T_INT32:
			case MinimalDataType::T_DOUBLE:
				data = e.data;
				break;
			case MinimalDataType::T_STRING:
				new (&data) str_t(*(reinterpret_cast<const str_t*>(&e.data)));
				break;
			case MinimalDataType::T_INT8_ARRAY:
				*(reinterpret_cast<vec8_t**>(&data)) = new vec8_t(
						**reinterpret_cast<vec8_t* const *>(&e.data));
				break;
			case MinimalDataType::NODE:
				*(reinterpret_cast<MinimalData**>(&data)) = new MinimalData(
						**reinterpret_cast<MinimalData* const *>(&e.data));
				break;
			}
			type_id = e.type_id;
		}

		template<typename T>
		MinimalDataElem(const T v) :
				type_id(MinimalDataType::EMPTY) {
			set(v);
		}

		void set(bool v) {
			set_pod<MinimalDataType::T_BOOL, bool>(v);
		}
		void set(int32_t v) {
			set_pod<MinimalDataType::T_INT32, int32_t>(v);
		}
		void set(double v) {
			set_pod<MinimalDataType::T_DOUBLE, double>(v);
		}
		void set(const char* str) {
			set(str_t(str));
		}
		void set(const str_t& str) {
			set_class<MinimalDataType::T_STRING, str_t>(str);
		}
		void set(vec8_t* v) {
			set_ptr<MinimalDataType::T_INT8_ARRAY, vec8_t>(v);
		}
		void set(MinimalData* v) {
			set_ptr<MinimalDataType::NODE, MinimalData>(v);
		}

		MinimalDataType get_type() const {
			return type_id;
		}

		template<typename T>
		const T& get() const {
			return *(reinterpret_cast<const T*>(&data));
		}

		template<typename T>
		bool get(T& val, const T& def = T()) const {
			bool res = false;
			if (elem_traits<T>::id == type_id) {
				val = get<T>();
				res = true;
			} else {
				val = def;
			}
			return res;
		}

		~MinimalDataElem() {
			deleter();
		}

	private:
		using TypeSizes = max_types_size<bool, int16_t, int32_t, double, std::string, std::vector<int8_t>*, MinimalData*>;
		using data_t = std::aligned_storage<TypeSizes::data_size, TypeSizes::data_align>::type;

		data_t data;
		MinimalDataType type_id;

		template<MinimalDataType typ, typename T>
		void set_pod(T val) {
			if (type_id != MinimalDataType::EMPTY && type_id != typ) {
				deleter();
			}
			type_id = typ;
			*(reinterpret_cast<T*>(&data)) = val;
		}

		template<MinimalDataType typ, typename T>
		void set_class(const T& v) {
			if (type_id != MinimalDataType::EMPTY && type_id != typ) {
				deleter();
			}
			if (type_id == MinimalDataType::EMPTY) {
				new (&data) T(v);
			} else if (type_id == typ) {
				*reinterpret_cast<T*>(&data) = v;
			}
			type_id = typ;
		}

		template<MinimalDataType typ, typename T>
		void set_ptr(T* val) {
			if (type_id != typ || *(reinterpret_cast<T**>(&data)) != val) {
				deleter();
			}
			type_id = typ;
			*(reinterpret_cast<T**>(&data)) = val;
		}

		inline void deleter() {
			switch (type_id) {
			case MinimalDataType::EMPTY:
				break;
			case MinimalDataType::T_BOOL:
				break;
			case MinimalDataType::T_INT32:
				break;
			case MinimalDataType::T_DOUBLE:
				break;
			case MinimalDataType::T_STRING:
				(reinterpret_cast<str_t*>(&data))->~str_t();
				break;
			case MinimalDataType::T_INT8_ARRAY:
				delete (*reinterpret_cast<vec8_t**>(&data));
				break;
			case MinimalDataType::NODE:
				delete (*reinterpret_cast<MinimalData**>(&data));
				break;
			}
			type_id = MinimalDataType::EMPTY;
		}
	};

	struct TokenID {
		static const size_t HI_BIT = size_t(1u)
				<< (std::numeric_limits<size_t>::digits - 1);
		static const size_t MASK = HI_BIT - 1;
		size_t id_;

		TokenID(const size_t id) :
				id_(HI_BIT | id) {
		}

		TokenID& operator=(const size_t id) {
			id_ = HI_BIT | id;
			return *this;
		}

		bool operator==(const size_t id) const {
			return (is_free() && to_free() == id);
		}

		bool operator!=(const size_t id) const {
			return !operator==(id);
		}

		bool empty() const {
			return id_ == 0;
		}
		bool is_free() const {
			return (id_ & HI_BIT);
		}
		size_t to_free() const {
			return id_ & MASK;
		}
	};
	struct Elem {
		TokenID id_;
		MinimalDataElem data_;
		template<typename T>
		Elem(size_t free_id, T v) :
				id_(free_id), data_(v) {
		}
	};

	struct namehash_hasher {
		size_t operator()(const size_t& v) const {
			return v;
		}
	};
	struct namehash_equal {
		bool operator()(const size_t& v1, const size_t& v2) const {
			return v1 == v2;
		}
	};

	typedef std::vector<Elem> ElemVector_t;
	typedef std::vector<std::string> FreeTokenVector_t;
	typedef std::unordered_multimap<size_t, size_t, namehash_hasher,
			namehash_equal> NameMap_t;

	ElemVector_t elems_;
	FreeTokenVector_t free_tokens_;
	mutable std::unique_ptr<NameMap_t> name_map_;
	std::hash<std::string> str_hasher_;
	MinimalData(const MinimalData& o) :
			elems_(o.elems_), free_tokens_(o.free_tokens_), name_map_(
					o.name_map_ ? new NameMap_t(*o.name_map_.get()) : nullptr) {
	}
public:
	MinimalData& operator=(const MinimalData&) = delete;
	explicit MinimalData(size_t cnt = 16) {
		elems_.reserve(cnt);
	}
	~MinimalData() {
	}

	template<typename T>
	typename std::enable_if<is_pod_elem<T>::value, void>::type add(
			const std::string& name, const T val) {
		add_imp(name, val);
	}

	template<typename T>
	typename std::enable_if<is_ptr_elem<T>::value, T&>::type add(
			const std::string& name) {
		return add_ptr_imp<T>(name);
	}
	template<typename T>
	typename std::enable_if<is_ptr_elem<T>::value, T&>::type add(
			const std::string& name, T&& o) {
		return add_ptr_imp<T>(name, std::move(o));
	}

	template<typename T>
	typename std::enable_if<is_pod_elem<T>::value, T>::type get(
			const size_t idx, const T def = T()) const {
		if (check_idx(idx)) {
			T res;
			elems_[idx].data_.get<T>(res, def);
			return res;
		}
		return def;
	}

	template<typename T>
	typename std::enable_if<is_pod_elem<T>::value, T>::type get(
			const std::string& name, const T def = T()) const {
		auto pos = find_imp(name);
		if (pos != -1) {
			T res;
			elems_[pos].data_.get<T>(res, def);
			return res;
		}
		return def;
	}

	template<typename T>
	typename std::enable_if<is_ptr_elem<T>::value, T&>::type get(
			const size_t idx) const {
		T *res = nullptr;
		if (check_idx(idx)) {
			elems_[idx].data_.get<T*>(res, nullptr);
		}
		return *res;
	}

	template<typename T>
	class ptr_iterator {
		int idx_;
		const MinimalData& parent_;
	public:
		ptr_iterator(int idx, const MinimalData& parent) :
				idx_(idx), parent_(parent) {
		}
		explicit operator bool() const {
			return idx_ > -1;
		}
		T* operator->() {
			return parent_.elems_[idx_].data_.template get<T*>();
		}
		T& operator*() {
			return *parent_.elems_[idx_].data_.template get<T*>();
		}

		const T* operator->() const {
			return parent_.elems_[idx_].data_.template get<T*>();
		}
		const T& operator*() const {
			return *parent_.elems_[idx_].data_.template get<T*>();
		}
	};

	template<typename T, typename K>
	typename std::enable_if<is_ptr_elem<T>::value, ptr_iterator<T>>::type find(
			const K& name) const {
		auto pos = find_imp(name);
		if (pos > -1 && elem_traits<T*>::id != type_of(pos)) {
			pos = -1;
		}
		return ptr_iterator<T>(pos, *this);
	}

	MinimalDataType type_of(size_t idx) const {
		MinimalDataType res = MinimalDataType::EMPTY;
		if (check_idx(idx)) {
			res = elems_[idx].data_.get_type();
		}
		return res;
	}

	template<typename T>
	static typename std::enable_if<is_ptr_elem<T>::value, MinimalDataType>::type type_of() {
		return elem_traits<T*>::id;
	}

	size_t size() const {
		return elems_.size();
	}

	void clear() {
		elems_.clear();
		free_tokens_.clear();
		name_map_.reset(nullptr);
	}

	std::string token_to_string(const TokenID id) const {
		std::string res;
		if (!id.empty()) {
			res = free_tokens_[id.to_free()];
		}
		return res;
	}

private:
	template<typename T>
	void add_imp(const std::string& name, const T val) {
		if (!name.empty()) {
			free_tokens_.emplace_back(std::move(name));
			elems_.emplace_back(free_tokens_.size() - 1, val);
			add_index(str_hasher_(name), elems_.size() - 1);
		}
	}

	template<typename T>
	T& add_ptr_imp(const std::string& name) {
		T* val = new T();
		add_imp(name, val);
		return *val;
	}

	template<typename T>
	T& add_ptr_imp(const std::string& name, T&& o) {
		T* val = new T(std::move(o));
		add_imp(name, val);
		return *val;
	}

	bool check_idx(size_t idx) const {
		return idx < elems_.size();
	}

	void add_index(const size_t hash, size_t pos) {
		if (name_map_) {
			name_map_->emplace(hash, pos);
		}
	}

	void build_index() const {
		name_map_.reset(new NameMap_t((elems_.size() * 3) / 2));
		for (size_t i = 0; i < elems_.size(); ++i) {
			const Elem& elem = elems_[i];
			if (!elem.id_.empty()) {
				name_map_->emplace(
						str_hasher_(free_tokens_[elem.id_.to_free()]), i);
			}
		}
	}

	int find_imp(const std::string& name) const {
		if (elems_.size() > 50 && !name_map_) {
			build_index();
		}
		return find_imp(str_hasher_(name),
				[this, &name] (const Elem& d) {return ( d.id_.is_free() ? free_tokens_[d.id_.to_free()] : token_to_string(d.id_) ) == name;});
	}

	template<typename F>
	int find_imp(const size_t hash, F f) const {
		size_t idx = 0;
		int res = -1;
		if (name_map_) {
			auto range = name_map_->equal_range(hash);
			for (auto it = range.first; it != range.second; ++it) {
				if (f(elems_[it->second])) {
					res = it->second;
					break;
				}
			}
		} else {
			for (auto it = elems_.cbegin(); it != elems_.cend(); ++it, ++idx) {
				if (f(*it)) {
					res = idx;
					break;
				}
			}
		}
		return res;
	}
};

#endif
